add_library(chatnotifier SHARED
  gl3w/GL/gl3w.c

  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp

  ${CMAKE_JS_SRC}
)

# C++20 module files
target_sources(chatnotifier PUBLIC FILE_SET CXX_MODULES FILES
  standard.cppm
  common.cppm
  types.cppm
  opengl.cppm
  assets.cppm
  commands.cppm
  audio.cppm
  twitch.cppm
  effect.cppm
  notification.cppm
  gui.cppm
  config.cppm
  runner.cppm
  filesystem.cppm
  scripting_py.cppm
  scripting.cppm
  napi.cppm
)

target_include_directories(chatnotifier PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/gl3w
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends

  ${CMAKE_JS_INC}
)

target_link_libraries(chatnotifier PUBLIC
  SndFile::sndfile
  OpenAL::OpenAL
  glfw
  hv_static
  pybind11::embed

  ${CMAKE_JS_LIB}
)

if (WIN32)
  target_link_libraries(chatnotifier PRIVATE
    ws2_32
  )
endif ()

# Enable warnings as errors for the target, unless WIN32 is set
set_target_properties(chatnotifier PROPERTIES
  $<$<NOT:$<BOOL:${WIN32}>>:COMPILE_WARNING_AS_ERROR ON>
  PREFIX ""
  SUFFIX ".node"
)

# Include Node-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/UI/
  OUTPUT_VARIABLE NODE_ADDON_API_DIR
)
string(REGEX REPLACE "[\r\n\"]" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

target_include_directories(chatnotifier PRIVATE ${NODE_ADDON_API_DIR})

# If WIN32, set _CRT_SECURE_NO_WARNINGS
target_compile_definitions(chatnotifier PRIVATE
  $<$<BOOL:${WIN32}>:_CRT_SECURE_NO_WARNINGS>
  NAPI_VERSION=3
)

if (MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
  execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  add_custom_command(TARGET chatnotifier POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:chatnotifier> ${CMAKE_SOURCE_DIR}/UI/dist-electron/chatnotifier.node
  )
  add_custom_command(TARGET chatnotifier POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:chatnotifier> ${CMAKE_SOURCE_DIR}/UI/src/electron/chatnotifier.node
  )
  add_custom_command(TARGET chatnotifier POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:chatnotifier> ${CMAKE_SOURCE_DIR}/UI/build/chatnotifier.node
  )
endif ()