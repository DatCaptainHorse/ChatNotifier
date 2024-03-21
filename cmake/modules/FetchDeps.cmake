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
#message(STATUS "Fetching glaze")
#FetchContent_Declare(
#  glaze
#  GIT_REPOSITORY "https://github.com/stephenberry/glaze.git"
#  GIT_TAG "v2.3.1"
#  GIT_SHALLOW TRUE
#  OVERRIDE_FIND_PACKAGE
#)
#FetchContent_MakeAvailable(glaze)

# Fetch spdlog
message(STATUS "Fetching spdlog")
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
  GIT_TAG "v1.13.0"
  GIT_SHALLOW TRUE
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(spdlog)

# Fetch onnxruntime for piper (using release zip because they don't have library targets)
message(STATUS "Fetching onnxruntime")
# If not win32
if (NOT WIN32)
  FetchContent_Declare(
    onnxruntime
    URL "https://github.com/microsoft/onnxruntime/releases/download/v1.17.1/onnxruntime-linux-x64-1.17.1.tgz"
    URL_HASH MD5=405e3ef137247c5229207da596ac45c0
  )
else ()
  FetchContent_Declare(
    onnxruntime
    URL "https://github.com/microsoft/onnxruntime/releases/download/v1.17.1/onnxruntime-win-x64-1.17.1.zip"
    URL_HASH MD5=26138ecd0cb6d4abe00c4070a27e5234
  )
endif ()
FetchContent_MakeAvailable(onnxruntime)
# Add onnxruntime to the project
add_library(onnxruntime INTERFACE)
target_include_directories(onnxruntime INTERFACE ${onnxruntime_SOURCE_DIR}/include)
# If not win32
if (NOT WIN32)
  target_link_libraries(onnxruntime INTERFACE ${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so)
else ()
  target_link_libraries(onnxruntime INTERFACE ${onnxruntime_SOURCE_DIR}/lib/onnxruntime.lib)
endif ()

# Fetch espeak-ng for piper
message(STATUS "Fetching espeak-ng")
FetchContent_Declare(
  espeak-ng
  GIT_REPOSITORY "https://github.com/rhasspy/espeak-ng.git"
  GIT_COMMIT "8593723f10cfd9befd50de447f14bf0a9d2a14a4"
)
# No extras so just make our own library
FetchContent_Populate(espeak-ng)
# Sub-library ucd
add_library(ucd STATIC
  ${espeak-ng_SOURCE_DIR}/src/ucd-tools/src/case.c
  ${espeak-ng_SOURCE_DIR}/src/ucd-tools/src/categories.c
  ${espeak-ng_SOURCE_DIR}/src/ucd-tools/src/ctype.c
  ${espeak-ng_SOURCE_DIR}/src/ucd-tools/src/proplist.c
  ${espeak-ng_SOURCE_DIR}/src/ucd-tools/src/scripts.c
  ${espeak-ng_SOURCE_DIR}/src/ucd-tools/src/tostring.c
)
target_include_directories(ucd PUBLIC ${espeak-ng_SOURCE_DIR}/src/ucd-tools/src/include)
# Create config.h with "#define PACKAGE_VERSION" to avoid errors
file(WRITE ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/config.h "#define PACKAGE_VERSION \"1.51.1\"")
add_library(espeaklib STATIC
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/common.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/mnemonics.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/error.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/ieee80.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/compiledata.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/compiledict.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/dictionary.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/encoding.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/intonation.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/langopts.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/numbers.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/phoneme.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/phonemelist.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/readclause.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/setlengths.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/soundicon.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/spect.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/ssml.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/synthdata.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/synthesize.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/tr_languages.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/translate.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/translateword.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/voices.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/wavegen.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/speech.c
  ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/espeak_api.c
)
target_include_directories(espeaklib PUBLIC ${espeak-ng_SOURCE_DIR}/src/include ${espeak-ng_SOURCE_DIR}/src/libespeak-ng/include)
target_link_libraries(espeaklib PRIVATE ucd)
target_compile_definitions(espeaklib PRIVATE "LIBESPEAK_NG_EXPORT=1")

# Fetch piper-phonemize for piper
message(STATUS "Fetching piper-phonemize")
FetchContent_Declare(
  piper-phonemize
  GIT_REPOSITORY "https://github.com/rhasspy/piper-phonemize.git"
  GIT_TAG "2023.11.14-4"
)
# Customize library..
FetchContent_Populate(piper-phonemize)
add_library(piper_phonemize STATIC
  ${piper-phonemize_SOURCE_DIR}/src/phonemize.cpp
  ${piper-phonemize_SOURCE_DIR}/src/phoneme_ids.cpp
  ${piper-phonemize_SOURCE_DIR}/src/shared.cpp
  ${piper-phonemize_SOURCE_DIR}/src/tashkeel.cpp
)
target_include_directories(piper_phonemize PUBLIC ${piper-phonemize_SOURCE_DIR}/src)
target_link_libraries(piper_phonemize PUBLIC onnxruntime espeaklib)

# Fetch piper
message(STATUS "Fetching piper")
# Patch..
set(PIPER_PATCH git apply "${PROJECT_SOURCE_DIR}/cmake/fix-piper.patch")
FetchContent_Declare(
  piper
  GIT_REPOSITORY "https://github.com/rhasspy/piper.git"
  GIT_TAG "2023.11.14-2"
  PATCH_COMMAND ${PIPER_PATCH}
  UPDATE_DISCONNECTED 1
)
# There's no library, we need to create one ourselves
FetchContent_MakeAvailable(piper)
add_library(piperlib STATIC ${piper_SOURCE_DIR}/src/cpp/piper.cpp)
target_include_directories(piperlib PUBLIC ${piper_SOURCE_DIR}/src/cpp ${piper-phonemize_SOURCE_DIR}/src ${onnxruntime_SOURCE_DIR}/include)
target_link_libraries(piperlib PRIVATE piper_phonemize spdlog)
