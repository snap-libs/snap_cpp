cmake_minimum_required(VERSION 3.16)
project(snap_cpp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Fetch nlohmann/json
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# 2. Setup ONNX Runtime
set(ORT_VERSION "1.18.1")
set(ORT_DIR "${CMAKE_CURRENT_BINARY_DIR}/_ort_sdk")

# Function to robustly set up ONNX Runtime SDK
function(setup_onnx_runtime ort_url)
    if(NOT EXISTS "${ORT_DIR}/include/onnxruntime_cxx_api.h")
        message(STATUS "Downloading ONNX Runtime SDK from ${ort_url}...")
        file(DOWNLOAD "${ort_url}" "${CMAKE_CURRENT_BINARY_DIR}/ort_pkg" SHOW_PROGRESS STATUS dl_status)
        list(GET dl_status 0 dl_code)
        if(NOT dl_code EQUAL 0)
            message(FATAL_ERROR "Failed to download ONNX Runtime SDK: ${ort_url}")
        endif()

        file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_BINARY_DIR}/ort_pkg" DESTINATION "${ORT_DIR}")
        file(GLOB EXTRACTED_SUBDIRS LIST_DIRECTORIES true "${ORT_DIR}/onnxruntime-*")
        foreach(subdir IN LISTS EXTRACTED_SUBDIRS)
            if(IS_DIRECTORY "${subdir}")
                if(EXISTS "${subdir}/include")
                    file(COPY "${subdir}/include/" DESTINATION "${ORT_DIR}/include")
                endif()
                if(EXISTS "${subdir}/lib")
                    file(COPY "${subdir}/lib/" DESTINATION "${ORT_DIR}/lib")
                endif()
            endif()
        endforeach()
    endif()
endfunction()

if(WIN32)
    setup_onnx_runtime("https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION}/onnxruntime-win-x64-${ORT_VERSION}.zip")
    set(ORT_LIB "${ORT_DIR}/lib/onnxruntime.lib")
elseif(APPLE)
    setup_onnx_runtime("https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION}/onnxruntime-osx-universal2-${ORT_VERSION}.tgz")
    find_library(ORT_LIB NAMES onnxruntime HINTS "${ORT_DIR}/lib")
else()
    setup_onnx_runtime("https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION}/onnxruntime-linux-x64-${ORT_VERSION}.tgz")
    find_library(ORT_LIB NAMES onnxruntime HINTS "${ORT_DIR}/lib")
endif()

include_directories(${ORT_DIR}/include)
link_directories(${ORT_DIR}/lib)

# 3. Build snap_cpp Shared Library
file(GLOB SNAP_CPP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(snap_cpp SHARED ${SNAP_CPP_SOURCES})
target_compile_definitions(snap_cpp PRIVATE SNAP_BUILD_DLL)
target_include_directories(snap_cpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(WIN32)
    target_link_libraries(snap_cpp PRIVATE nlohmann_json::nlohmann_json ${ORT_LIB})
    add_custom_command(TARGET snap_cpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ORT_DIR}/lib/onnxruntime.dll"
        "$<TARGET_FILE_DIR:snap_cpp>"
    )
else()
    if(ORT_LIB)
        target_link_libraries(snap_cpp PRIVATE nlohmann_json::nlohmann_json ${ORT_LIB})
    else()
        target_link_libraries(snap_cpp PRIVATE nlohmann_json::nlohmann_json onnxruntime)
    endif()
endif()

# 4. Build Examples if present
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_e2e.cpp")
    add_executable(test_e2e test_e2e.cpp)
    target_link_libraries(test_e2e PRIVATE snap_cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/version_check.cpp")
    add_executable(version_check examples/version_check.cpp)
    target_link_libraries(version_check PRIVATE snap_cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cpp_unified_config.cpp")
    add_executable(test_cpp_config test_cpp_unified_config.cpp src/config.cpp)
    target_include_directories(test_cpp_config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(test_cpp_config PRIVATE nlohmann_json::nlohmann_json)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cpp_mass_e2e.cpp")
    add_executable(test_cpp_mass_e2e test_cpp_mass_e2e.cpp)
    target_link_libraries(test_cpp_mass_e2e PRIVATE snap_cpp)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cpp_accuracy.cpp")
    add_executable(test_cpp_accuracy test_cpp_accuracy.cpp)
    target_link_libraries(test_cpp_accuracy PRIVATE nlohmann_json::nlohmann_json snap_cpp)
endif()




