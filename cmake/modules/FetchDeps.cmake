if (NOT NON_NIX_BUILD)
  message(WARNING "Fetching dependencies without `NON_NIX_BUILD` set.")
endif ()

include(FetchContent)

# Fetch imgui
# Needs special patch to have transparent framebuffers
message(STATUS "Fetching ImGui")
set(IMGUI_PATCH git apply "${PROJECT_SOURCE_DIR}/nix/pkgs/imgui/0001-Add-transparency-flag-to-viewports.patch")
FetchContent_Declare(
  imgui
  GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
  GIT_TAG "v1.90.4-docking"
  PATCH_COMMAND ${IMGUI_PATCH}
  UPDATE_DISCONNECTED 1
)
FetchContent_MakeAvailable(imgui)
set(IMGUI_DIR ${imgui_SOURCE_DIR})

# Fetch fmtlib
message(STATUS "Fetching fmtlib")
FetchContent_Declare(
  fmt
  GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
  GIT_TAG "10.2.1"
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(fmt)

# Fetch glbinding
message(STATUS "Fetching glbinding")
FetchContent_Declare(
  glbinding
  GIT_REPOSITORY "https://github.com/cginternals/glbinding.git"
  GIT_TAG "v3.3.0"
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(glbinding)

# Fetch glfw
message(STATUS "Fetching GLFW")
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
message(STATUS "Fetching websocket++")
set(WEBSOCKETPP_PATCH git apply "${PROJECT_SOURCE_DIR}/cmake/patches/0001-Fix-cpp20-build.patch")
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
