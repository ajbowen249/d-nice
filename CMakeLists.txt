cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

project(d_nice)

add_executable(d_nice
    src/DNS.cpp
    src/duk_module_duktape.cpp
    src/duktape.cpp
    src/Logging.cpp
    src/main.cpp
    src/OS.cpp
    src/ProcessQueue.cpp
)

target_include_directories(d_nice
    PRIVATE inc
    PRIVATE build/inc
)

add_custom_command(TARGET d_nice
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -P "${PROJECT_SOURCE_DIR}/BuildResources.cmake"
)
