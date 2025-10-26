cmake_minimum_required(VERSION 3.16)
project(MRO_TotalBridge_CPP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

if (COMMAND qt_standard_project_setup)
    qt_standard_project_setup()
endif()

set(UI_HEADERS
    include/ui/main_window.hpp
    include/ui/module_config_dialog.hpp
)

set(APP_SOURCES
    src/main.cpp
    src/utils/json.cpp
    src/utils/logger.cpp
    src/utils/settings.cpp
    src/network/socket_utils.cpp
    src/core/image_stream_bridge.cpp
    src/core/gimbal_control.cpp
    src/core/udp_relay.cpp
    src/ui/main_window.cpp
    src/ui/module_config_dialog.cpp
    ${UI_HEADERS}
)

if (COMMAND qt_add_executable)
    qt_add_executable(unified_bridge ${APP_SOURCES})
else()
    add_executable(unified_bridge ${APP_SOURCES})
endif()

target_include_directories(unified_bridge PRIVATE include)
target_link_libraries(unified_bridge PRIVATE Qt6::Widgets)

if (MSVC)
    target_compile_definitions(unified_bridge PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(unified_bridge PRIVATE ws2_32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(unified_bridge PRIVATE Threads::Threads)
endif()

if (COMMAND qt_finalize_executable)
    qt_finalize_executable(unified_bridge)
endif()
