if (NOT NON_NIX_BUILD)
  message(WARNING "Fetching dependencies without `NON_NIX_BUILD` set.")
endif ()

include(FetchContent)

# Fetch libogg
message(STATUS "Fetching libogg")
FetchContent_Declare(
  ogg
  GIT_REPOSITORY "https://github.com/xiph/ogg.git"
  GIT_TAG "v1.3.5"
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(ogg)

# Fetch libopus
message(STATUS "Fetching libopus")
FetchContent_Declare(
  opus
  GIT_REPOSITORY "https://github.com/xiph/opus.git"
  GIT_TAG "v1.5.1"
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(opus)

# Fetch opusfile
message(STATUS "Fetching opusfile")
FetchContent_Declare(
  opusfile
  GIT_REPOSITORY "https://github.com/xiph/opusfile.git"
  GIT_COMMIT "9d718345ce03b2fad5d7d28e0bcd1cc69ab2b166"
  OVERRIDE_FIND_PACKAGE
)
# Force OP_DISABLE_DOCS ON
set(OP_DISABLE_DOCS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(opusfile)

# Fetch miniaudio
message(STATUS "Fetching miniaudio")
FetchContent_Declare(
  miniaudio
  GIT_REPOSITORY "https://github.com/mackron/miniaudio.git"
  GIT_TAG "0.11.21"
  OVERRIDE_FIND_PACKAGE
)
# Miniaudio has no CMakeLists, just a single header (miniaudio.h), need to turn it into an interface library
FetchContent_MakeAvailable(miniaudio)
add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE ${miniaudio_SOURCE_DIR})

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
# While fmt has C++20 modules, the CMake script they have for it is broken..
FetchContent_MakeAvailable(fmt)

# Fetch glfw
message(STATUS "Fetching GLFW")
FetchContent_Declare(
  glfw3
  GIT_REPOSITORY "https://github.com/glfw/glfw.git"
  GIT_TAG "3.4"
  OVERRIDE_FIND_PACKAGE
)
# Make sure GLFW_BUILD_WAYLAND is OFF and GLFW_BUILD_X11 is ON
# glfw 3.4 has lacking support for wayland
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
