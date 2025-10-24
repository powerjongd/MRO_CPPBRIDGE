cmake_minimum_required(VERSION 3.16)
project(MRO_TotalBridge_CPP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(unified_bridge
    src/main.cpp
    src/utils/json.cpp
    src/utils/logger.cpp
    src/utils/settings.cpp
    src/network/socket_utils.cpp
    src/core/image_stream_bridge.cpp
    src/core/gimbal_control.cpp
    src/core/udp_relay.cpp
)

target_include_directories(unified_bridge PRIVATE include)

if (MSVC)
    target_compile_definitions(unified_bridge PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(unified_bridge PRIVATE ws2_32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(unified_bridge PRIVATE Threads::Threads)
endif()
