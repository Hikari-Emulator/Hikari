cmake_minimum_required(VERSION 3.25)

project(Hikari LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

add_library(hikari_warnings INTERFACE)
target_compile_options(hikari_warnings INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Werror
)

find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK is required but was not found.")
endif()

find_package(glfw3 REQUIRED)
if(NOT glfw3_FOUND AND NOT TARGET glfw)
    message(FATAL_ERROR "GLFW 3 (glfw3) is required but was not found.")
endif()

add_subdirectory(src/core)
add_subdirectory(src/capture)
add_subdirectory(src/rsb)
add_subdirectory(src/validate)
add_subdirectory(tools/capdump)
add_subdirectory(tests/unit)
add_subdirectory(tests/golden)
