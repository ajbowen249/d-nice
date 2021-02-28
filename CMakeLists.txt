cmake_minimum_required(VERSION 3.7)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

project(d_nice)

add_executable(d_nice
    src/duk_module_duktape.cpp
    src/duktape.cpp
    src/main.cpp
)

target_include_directories(d_nice
    PRIVATE inc
)
