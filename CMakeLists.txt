cmake_minimum_required(VERSION 3.30)
project(ChatNotifier)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # We don't want to use any compiler-specific extensions
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/modules")

# Preprocessor define TWITCH_CLIENT_SECRET from env
if (DEFINED ENV{TWITCH_CLIENT_SECRET})
  add_compile_definitions(TWITCH_CLIENT_SECRET="$ENV{TWITCH_CLIENT_SECRET}")
endif ()

# If win32 and not clang, force MSVC_RUNTIME_LIBRARY to static
if (WIN32 AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  # Also allow multiple definitions
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /FORCE:MULTIPLE")
endif ()

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "Building within the source tree is unsupported")
endif ()

if ("$ENV{IMGUI_DIR}" STREQUAL "" OR "$ENV{MINIAUDIO_DIR}" STREQUAL "")
  message(STATUS "`IMGUI_DIR` or `MINIAUDIO_DIR` not set, setting NON_NIX_BUILD")
  set(NON_NIX_BUILD ON)

  # If clang compiler, use libc++ rather than libstdc++ (if available)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-stdlib=libc++" HAS_LIBCXX)
    if (HAS_LIBCXX)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif ()
  endif ()

  include(FetchDeps)
else ()
  set(IMGUI_DIR $ENV{IMGUI_DIR})

  add_library(miniaudio INTERFACE)
  target_include_directories(miniaudio INTERFACE $ENV{MINIAUDIO_DIR})
endif ()

include(CMakeDependentOption)

# This doesn't work on MinGW, and seems to have issues with hardened kernels?
cmake_dependent_option(USE_SANITIZERS "Use sanitizers if supported by target" OFF "NOT WIN32" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#find_package(glaze REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Python 3.12 COMPONENTS Development.Module Development.Embed REQUIRED)
find_package(nanobind REQUIRED)
find_package(libhv REQUIRED)
find_package(libsndfile REQUIRED)

if (NON_NIX_BUILD)
else()
  find_package(PkgConfig REQUIRED)
  find_package(OpenSSL REQUIRED)
endif()

add_subdirectory(Source)
