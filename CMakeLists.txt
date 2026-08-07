cmake_minimum_required(VERSION 3.16)
project(snap_cpp_sdk CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/x64/v1.0.0)
elseif(APPLE)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/macos/v1.0.0)
else()
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/linux/x64/v1.0.0)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_e2e.cpp")
    add_executable(test_e2e test_e2e.cpp)
    target_link_libraries(test_e2e PRIVATE snap_cpp)
    
    if(WIN32)
        add_custom_command(TARGET test_e2e POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/x64/v1.0.0/snap_cpp.dll"
            "$<TARGET_FILE_DIR:test_e2e>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/x64/v1.0.0/onnxruntime.dll"
            "$<TARGET_FILE_DIR:test_e2e>"
        )
    endif()
endif()
