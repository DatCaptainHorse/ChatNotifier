if (NOT NON_NIX_BUILD)
  message(WARNING "Fetching dependencies without `NON_NIX_BUILD` set.")
endif ()

include(FetchContent)

# Fetch libogg
message(STATUS "Fetching ogg")
FetchContent_Declare(
  ogg
  GIT_REPOSITORY "https://github.com/xiph/ogg.git"
  GIT_TAG "v1.3.5"
  FIND_PACKAGE_ARGS
)
# INSTALL_DOCS to OFF, since we don't care about the docs
set(INSTALL_DOCS OFF CACHE BOOL "" FORCE)

# Fetch libopus
message(STATUS "Fetching opus")
FetchContent_Declare(
  opus
  GIT_REPOSITORY "https://github.com/xiph/opus.git"
  GIT_TAG "v1.5.2"
  FIND_PACKAGE_ARGS
)

# Fetch libvorbis
message(STATUS "Fetching vorbis")
FetchContent_Declare(
  vorbis
  GIT_REPOSITORY "https://github.com/xiph/vorbis.git"
  GIT_TAG "v1.3.7"
  FIND_PACKAGE_ARGS
)

# Fetch libflac
message(STATUS "Fetching flac")
FetchContent_Declare(
  flac
  GIT_REPOSITORY "https://github.com/xiph/flac.git"
  GIT_TAG "1.4.3"
  FIND_PACKAGE_ARGS
)
# We don't need manpages, testing, etc..
set(INSTALL_MANPAGES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_REGTEST OFF CACHE BOOL "" FORCE)
set(BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_CPACK OFF CACHE BOOL "" FORCE)

# Fetch libsndfile with patch
message(STATUS "Fetching libsndfile")
set(LIBSNDFILE_PATCH git apply "${PROJECT_SOURCE_DIR}/cmake/0002-Fix-sndfile-opus.patch")
FetchContent_Declare(
  libsndfile
  GIT_REPOSITORY "https://github.com/libsndfile/libsndfile.git"
  GIT_COMMIT "58c05b87162264200b1aa7790be260fd74c9deee"
  PATCH_COMMAND ${LIBSNDFILE_PATCH}
  UPDATE_DISCONNECTED 1
  FIND_PACKAGE_ARGS
)
# We don't care about the examples
set(BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ENABLE_CPACK OFF CACHE BOOL "" FORCE)

# Fetch openal-soft
message(STATUS "Fetching openal-soft")
FetchContent_Declare(
  openal-soft
  GIT_REPOSITORY "https://github.com/kcat/openal-soft.git"
  GIT_COMMIT "e39038317623c6282c3fd5685fcceaf3cc3137c7"
  FIND_PACKAGE_ARGS
)
# We don't care about the examples
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_NO_CONFIG_UTIL ON CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_EAX OFF CACHE BOOL "" FORCE)
# Make sure it's statically built
set(LIBTYPE STATIC CACHE STRING "" FORCE)

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

# Fetch glfw
message(STATUS "Fetching GLFW")
FetchContent_Declare(
  glfw3
  GIT_REPOSITORY "https://github.com/glfw/glfw.git"
  GIT_TAG "3.4"
  FIND_PACKAGE_ARGS
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

# Fetch libhv
message(STATUS "Fetching libhv")
FetchContent_Declare(
  libhv
  GIT_REPOSITORY "https://github.com/ithewei/libhv.git"
  GIT_TAG "v1.3.2"
  FIND_PACKAGE_ARGS
)
# BUILD_SHARED, BUILD_EXAMPLES to OFF, we don't need them
set(BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# We do want to build for MT and with OpenSSL if not under Windows
set(BUILD_FOR_MT ON CACHE BOOL "" FORCE)
if (NOT WIN32)
  set(WITH_OPENSSL ON CACHE BOOL "" FORCE)
endif ()

# Fetch glaze
#message(STATUS "Fetching glaze")
#FetchContent_Declare(
#  glaze
#  GIT_REPOSITORY "https://github.com/stephenberry/glaze.git"
#  GIT_TAG "v3.2.5"
#  OVERRIDE_FIND_PACKAGE
#)
#FetchContent_MakeAvailable(glaze)


# Make available
FetchContent_MakeAvailable(ogg opus vorbis flac libsndfile openal-soft imgui glfw3 libhv)
set(IMGUI_DIR ${imgui_SOURCE_DIR})
add_library(Vorbis::vorbisenc ALIAS vorbisenc)
