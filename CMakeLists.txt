cmake_minimum_required(VERSION 3.20)
project(ChatNotifier)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/modules")

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "Building within the source tree is unsupported")
endif ()

if ("$ENV{IMGUI_DIR}" STREQUAL "")
  message(STATUS "`IMGUI_DIR` not set, setting NON_NIX_BUILD")
  set(NON_NIX_BUILD ON)
  include(FetchDeps)
else ()
  set(IMGUI_DIR $ENV{IMGUI_DIR})
endif ()

include(CMakeDependentOption)

# This doesn't work on MinGW, and seems to have issues with hardened kernels?
cmake_dependent_option(USE_SANITIZERS "Use sanitizers if supported by target" OFF "NOT WIN32" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(PkgConfig REQUIRED)

pkg_check_modules(asio REQUIRED IMPORTED_TARGET asio)
find_package(fmt REQUIRED)
find_package(glbinding REQUIRED)
find_package(glfw3 REQUIRED)
find_package(websocketpp REQUIRED)

add_subdirectory(Source)
