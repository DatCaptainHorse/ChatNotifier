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
# INSTALL_DOCS to OFF, since we don't care about the docs
set(INSTALL_DOCS OFF CACHE BOOL "" FORCE)
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
# Disable unnecessary features
set(OP_DISABLE_DOCS ON CACHE BOOL "" FORCE)
set(OP_DISABLE_HTTP ON CACHE BOOL "" FORCE)
set(OP_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(opusfile)

# Fetch miniaudio
message(STATUS "Fetching miniaudio")
FetchContent_Declare(
  miniaudio
  GIT_REPOSITORY "https://github.com/mackron/miniaudio.git"
  GIT_TAG "0.11.21"
  OVERRIDE_FIND_PACKAGE
)
# Miniaudio has no CMakeLists, just a single header (miniaudio.h)
# though we want to use the extras/miniaudio_split/miniaudio.h/.c files to fix some issues..
FetchContent_MakeAvailable(miniaudio)
add_library(miniaudio STATIC ${miniaudio_SOURCE_DIR}/extras/miniaudio_split/miniaudio.c)
target_include_directories(miniaudio PUBLIC ${miniaudio_SOURCE_DIR}/extras/miniaudio_split)

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
# We don't care about docs..
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw3)

# Fetch libhv
message(STATUS "Fetching libhv")
FetchContent_Declare(
  libhv
  GIT_REPOSITORY "https://github.com/ithewei/libhv.git"
  GIT_TAG "v1.3.2"
  OVERRIDE_FIND_PACKAGE
)
# BUILD_SHARED, BUILD_EXAMPLES to OFF, we don't need them
set(BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libhv)

# Fetch glaze
message(STATUS "Fetching glaze")
FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG "v2.3.1"
  GIT_SHALLOW TRUE
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(glaze)
