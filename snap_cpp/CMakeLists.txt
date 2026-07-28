cmake_minimum_required(VERSION 3.15)
project(snap_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC: UTF-8 source files
if(MSVC)
    add_compile_options(/utf-8)
endif()

# ──────────────────────────────────────────────────────
# Options
# ──────────────────────────────────────────────────────
option(USE_GPU "Build with GPU (CUDA/TensorRT) ONNX Runtime support" OFF)

# ──────────────────────────────────────────────────────
# Dependencies
# ──────────────────────────────────────────────────────

include(FetchContent)

# 1. nlohmann/json (header-only, auto-download)
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

# 2. ONNX Runtime C++ SDK (prebuilt binary — download directly, no add_subdirectory)
if(WIN32)
    if(USE_GPU)
        set(ORT_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.18.1/onnxruntime-win-x64-gpu-1.18.1.zip")
        set(ORT_ARCHIVE "${CMAKE_BINARY_DIR}/_ort_download/ort.zip")
    else()
        set(ORT_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.18.1/onnxruntime-win-x64-1.18.1.zip")
        set(ORT_ARCHIVE "${CMAKE_BINARY_DIR}/_ort_download/ort.zip")
    endif()
    set(ORT_LIB_NAME "onnxruntime")
    set(ORT_SO_NAME "onnxruntime.dll")
elseif(APPLE)
    set(ORT_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.18.1/onnxruntime-osx-universal2-1.18.1.tgz")
    set(ORT_ARCHIVE "${CMAKE_BINARY_DIR}/_ort_download/ort.tgz")
    set(ORT_LIB_NAME "onnxruntime")
    set(ORT_SO_NAME "libonnxruntime.dylib")
else()
    set(ORT_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.18.1/onnxruntime-linux-x64-1.18.1.tgz")
    set(ORT_ARCHIVE "${CMAKE_BINARY_DIR}/_ort_download/ort.tgz")
    set(ORT_LIB_NAME "onnxruntime")
    set(ORT_SO_NAME "libonnxruntime.so")
endif()

set(ORT_EXTRACT_DIR "${CMAKE_BINARY_DIR}/_ort_sdk")

# ORT_SDK_DIR can be passed from CI (pre-downloaded) to skip network access during configure.
# If not provided, cmake downloads it automatically via file(DOWNLOAD).
if(ORT_SDK_DIR)
    # Use pre-downloaded SDK (CI path)
    set(ORT_EXTRACT_DIR "${ORT_SDK_DIR}")
    message(STATUS "[snap_cpp] Using pre-downloaded ONNX Runtime SDK at: ${ORT_SDK_DIR}")
else()
    message(STATUS "[snap_cpp] Downloading ONNX Runtime SDK from: ${ORT_URL}")

    # Download only if archive is not already present (acts as a simple cache)
    if(NOT EXISTS "${ORT_ARCHIVE}")
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_ort_download")
        file(DOWNLOAD "${ORT_URL}" "${ORT_ARCHIVE}"
            SHOW_PROGRESS
            STATUS _ort_dl_status
        )
        list(GET _ort_dl_status 0 _ort_dl_code)
        if(NOT _ort_dl_code EQUAL 0)
            list(GET _ort_dl_status 1 _ort_dl_msg)
            message(FATAL_ERROR "[snap_cpp] ONNX Runtime SDK download failed: ${_ort_dl_msg}")
        endif()
    endif()

    # Extract only if include/onnxruntime_cxx_api.h is not already present
    if(NOT EXISTS "${ORT_EXTRACT_DIR}/include/onnxruntime_cxx_api.h")
        file(MAKE_DIRECTORY "${ORT_EXTRACT_DIR}")
        file(ARCHIVE_EXTRACT INPUT "${ORT_ARCHIVE}"
            DESTINATION "${ORT_EXTRACT_DIR}"
        )
        # ORT archives contain a single top-level directory; find and flatten it
        file(GLOB _ort_subdirs LIST_DIRECTORIES true "${ORT_EXTRACT_DIR}/onnxruntime-*")
        if(_ort_subdirs)
            list(GET _ort_subdirs 0 _ort_root)
            file(RENAME "${_ort_root}/include" "${ORT_EXTRACT_DIR}/include")
            file(RENAME "${_ort_root}/lib"     "${ORT_EXTRACT_DIR}/lib")
        endif()
    endif()
endif()

set(ORT_INCLUDE_DIR "${ORT_EXTRACT_DIR}/include")
set(ORT_LIB_DIR     "${ORT_EXTRACT_DIR}/lib")

# Verify extraction
if(NOT EXISTS "${ORT_INCLUDE_DIR}/onnxruntime_cxx_api.h")
    message(FATAL_ERROR "[snap_cpp] ONNX Runtime SDK extraction failed — onnxruntime_cxx_api.h not found.")
endif()

message(STATUS "[snap_cpp] ONNX Runtime SDK ready at: ${ORT_EXTRACT_DIR}")

# ──────────────────────────────────────────────────────
# Shared Library (DLL)
# ──────────────────────────────────────────────────────

if(MSVC)
    enable_language(RC)
    set(VERSION_RC_SRC src/snap_version.rc)
else()
    set(VERSION_RC_SRC "")
endif()

add_library(snap_cpp SHARED
    src/tokenizer.cpp
    src/bert_session.cpp
    src/classifier.cpp
    src/phonology.cpp
    src/phonology_ja.cpp
    src/phonology_en.cpp
    src/num2words_ko.cpp
    src/text_normalize_kr.cpp
    src/snap_api.cpp
    src/snap_version.cpp
    ${VERSION_RC_SRC}
)

set_target_properties(snap_cpp PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

target_compile_definitions(snap_cpp PRIVATE SNAP_BUILD_DLL)

target_include_directories(snap_cpp
    PUBLIC  include
    PRIVATE "${ORT_INCLUDE_DIR}"
)

target_link_directories(snap_cpp PRIVATE "${ORT_LIB_DIR}")

if(WIN32)
    # Link to onnxruntime.lib on Windows
    target_link_libraries(snap_cpp PRIVATE onnxruntime nlohmann_json::nlohmann_json)
else()
    # Link to libonnxruntime.so / .dylib on Unix-like OS
    target_link_libraries(snap_cpp PRIVATE ${ORT_LIB_NAME} nlohmann_json::nlohmann_json)
endif()

# Copy ONNX Runtime shared library to output target directory (For easy execution/debugging)
if(WIN32)
    if(USE_GPU)
        set(COPY_CMD ${CMAKE_COMMAND} -E copy_if_different
            "${ORT_LIB_DIR}/onnxruntime.dll"
            "${ORT_LIB_DIR}/onnxruntime_providers_cuda.dll"
            "${ORT_LIB_DIR}/onnxruntime_providers_shared.dll"
            "${ORT_LIB_DIR}/onnxruntime_providers_tensorrt.dll"
            $<TARGET_FILE_DIR:snap_cpp>
        )
    else()
        set(COPY_CMD ${CMAKE_COMMAND} -E copy_if_different
            "${ORT_LIB_DIR}/onnxruntime.dll"
            $<TARGET_FILE_DIR:snap_cpp>
        )
    endif()
elseif(APPLE)
    set(COPY_CMD ${CMAKE_COMMAND} -E copy_if_different "${ORT_LIB_DIR}/libonnxruntime.dylib" $<TARGET_FILE_DIR:snap_cpp>)
else()
    set(COPY_CMD ${CMAKE_COMMAND} -E copy_if_different "${ORT_LIB_DIR}/libonnxruntime.so" $<TARGET_FILE_DIR:snap_cpp>)
endif()

add_custom_command(TARGET snap_cpp POST_BUILD
    COMMAND ${COPY_CMD}
    COMMENT "[snap_cpp] Copying ONNX Runtime shared library and providers to target directory..."
)

# ──────────────────────────────────────────────────────
# Test executable
# ──────────────────────────────────────────────────────

add_executable(test_e2e test_e2e.cpp)
target_link_libraries(test_e2e PRIVATE snap_cpp)
target_include_directories(test_e2e PRIVATE
    include
    "${ORT_INCLUDE_DIR}"
)

