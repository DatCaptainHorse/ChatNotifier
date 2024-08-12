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
  GIT_TAG "v1.5.2"
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

# Fetch libnyquist
message(STATUS "Fetching libnyquist")
FetchContent_Declare(
  libnyquist
  GIT_REPOSITORY "https://github.com/ddiakopoulos/libnyquist.git"
  GIT_COMMIT "767efd97cdd7a281d193296586e708490eb6e54f"
  OVERRIDE_FIND_PACKAGE
)
# Disable unnecessary features
set(LIBNYQUIST_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libnyquist)

# Fetch openal-soft
message(STATUS "Fetching openal-soft")
FetchContent_Declare(
  openal-soft
  GIT_REPOSITORY "https://github.com/kcat/openal-soft.git"
  GIT_COMMIT "1318bea2e0f0af9430335708e65ae2ff920d98c6"
  OVERRIDE_FIND_PACKAGE
)
# We don't care about the examples
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_NO_CONFIG_UTIL ON CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_EAX OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(openal-soft)

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
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
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
set(BUILD_FOR_MT ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libhv)

# Fetch glaze
message(STATUS "Fetching glaze")
FetchContent_Declare(
  glaze
  GIT_REPOSITORY "https://github.com/stephenberry/glaze.git"
  GIT_TAG "main"
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(glaze)

# Fetch nanobind
message(STATUS "Fetching nanobind")
FetchContent_Declare(
  nanobind
  GIT_REPOSITORY "https://github.com/wjakob/nanobind.git"
  GIT_TAG "v2.1.0"
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(nanobind)
