cmake_minimum_required(VERSION 3.20)
project(ChatNotifier)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "Building within the source tree is unsupported")
endif ()

if ("$ENV{IMGUI_DIR}" STREQUAL "")
  message(WARNING "`IMGUI_DIR` not set, assuming non-nix build, fetching dependencies..")

  set(NON_NIX_BUILD ON)
  include(FetchContent)

  # Fetch imgui
  # Needs special patch to have transparent framebuffers
  set(IMGUI_PATCH git apply "${CMAKE_SOURCE_DIR}/nix/pkgs/imgui/0001-Add-transparency-flag-to-viewports.patch")
  FetchContent_Declare(
    imgui
    GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
    GIT_TAG "docking"
    PATCH_COMMAND ${IMGUI_PATCH}
    UPDATE_DISCONNECTED 1
  )
  FetchContent_MakeAvailable(imgui)
  set(ENV{IMGUI_DIR} ${imgui_SOURCE_DIR})

  # Fetch fmtlib
  FetchContent_Declare(
    fmt
    GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
    GIT_TAG "10.2.1"
    OVERRIDE_FIND_PACKAGE
  )
  FetchContent_MakeAvailable(fmt)

  # Fetch glbinding
  FetchContent_Declare(
    glbinding
    GIT_REPOSITORY "https://github.com/cginternals/glbinding.git"
    GIT_TAG "v3.3.0"
    OVERRIDE_FIND_PACKAGE
  )
  FetchContent_MakeAvailable(glbinding)

  # Fetch glfw
  FetchContent_Declare(
    glfw3
    GIT_REPOSITORY "https://github.com/glfw/glfw.git"
    GIT_TAG "3.4"
    OVERRIDE_FIND_PACKAGE
  )
  # Force GLFW_BUILD_WAYLAND to OFF and GLFW_BUILD_X11 to ON under Linux
  if (UNIX AND NOT APPLE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
  endif ()
  FetchContent_MakeAvailable(glfw3)

  # Fetch websocketpp
  # Needs patch as well
  set(WEBSOCKETPP_PATCH git apply "${CMAKE_SOURCE_DIR}/cmake/patches/0001-Fix-cpp20-build.patch")
  FetchContent_Declare(
    websocketpp
    GIT_REPOSITORY "https://github.com/zaphoyd/websocketpp.git"
    GIT_TAG "0.8.2"
    OVERRIDE_FIND_PACKAGE
    PATCH_COMMAND ${WEBSOCKETPP_PATCH}
    UPDATE_DISCONNECTED 1
  )
  FetchContent_GetProperties(websocketpp)
  if(NOT websocketpp_POPULATED)
    FetchContent_Populate(websocketpp)
    add_subdirectory(${websocketpp_SOURCE_DIR} ${websocketpp_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
  # add interface library with all websocketpp dependencies
  add_library(websocketpp INTERFACE)
  target_include_directories(websocketpp INTERFACE ${websocketpp_SOURCE_DIR})
endif ()

set(IMGUI_DIR $ENV{IMGUI_DIR})

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

# If not NON_NIX_BUILD, find websocketpp
if (NOT NON_NIX_BUILD)
  find_package(websocketpp REQUIRED)
endif ()

add_subdirectory(Source)
