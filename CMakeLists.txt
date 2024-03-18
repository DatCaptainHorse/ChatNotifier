cmake_minimum_required(VERSION 3.28)
project(ChatNotifier)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # We don't want to use any compiler-specific extensions
set(CMAKE_CXX_SCAN_FOR_MODULES ON) # We use C++20, might as well use modules

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/modules")

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "Building within the source tree is unsupported")
endif ()

if ("$ENV{IMGUI_DIR}" STREQUAL "" OR "$ENV{MINIAUDIO_DIR}" STREQUAL "")
  message(STATUS "`IMGUI_DIR` or `MINIAUDIO_DIR` not set, setting NON_NIX_BUILD")
  set(NON_NIX_BUILD ON)
  include(FetchDeps)
else ()
  set(IMGUI_DIR $ENV{IMGUI_DIR})

  add_library(miniaudio INTERFACE)
  target_include_directories(miniaudio INTERFACE $ENV{MINIAUDIO_DIR})
endif ()

include(CMakeDependentOption)

# This doesn't work on MinGW, and seems to have issues with hardened kernels?
cmake_dependent_option(USE_SANITIZERS "Use sanitizers if supported by target" OFF "NOT WIN32" OFF)

# If "mold" linker is available, use it
find_program(MOLD_LINKER mold)
if (MOLD_LINKER)
  set(CMAKE_LINKER_TYPE ${MOLD_LINKER})
  message(STATUS "Using mold linker")
endif ()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(fmt REQUIRED)
find_package(glfw3 REQUIRED)

if (NON_NIX_BUILD)
  find_package(glbinding REQUIRED)
  find_package(ogg REQUIRED)
  find_package(opus REQUIRED)
  find_package(opusfile REQUIRED)
else()
  find_package(PkgConfig REQUIRED)
  find_package(gl3w REQUIRED)
  find_package(websocketpp REQUIRED)
  pkg_check_modules(asio REQUIRED IMPORTED_TARGET asio)
  pkg_check_modules(ogg REQUIRED IMPORTED_TARGET ogg)
  pkg_check_modules(opus REQUIRED IMPORTED_TARGET opus)
  pkg_check_modules(opusfile REQUIRED IMPORTED_TARGET opusfile)
endif()

add_subdirectory(Source)
