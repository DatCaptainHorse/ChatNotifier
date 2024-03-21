add_executable(ChatNotifier
  main.cpp

  gl3w/GL/gl3w.c

  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)

# C++20 module files
target_sources(ChatNotifier PRIVATE FILE_SET CXX_MODULES FILES
  common.cppm
  assets.cppm
  commands.cppm
  opus_decoder.cppm
  audio.cppm
  twitch.cppm
  notification.cppm
  gui.cppm
  config.cppm
)

target_include_directories(ChatNotifier PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/gl3w
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
  ${ogg_INCLUDE_DIRS}
  ${opus_INCLUDE_DIRS}
  ${opusfile_INCLUDE_DIRS}
)

target_link_libraries(ChatNotifier PRIVATE
#  glaze::glaze
  miniaudio
  glfw
  hv_static

  $<$<BOOL:${NON_NIX_BUILD}>:ogg>
  $<$<BOOL:${NON_NIX_BUILD}>:opus>
  $<$<BOOL:${NON_NIX_BUILD}>:OpusFile::opusfile>

  ${ogg_LIBRARIES}
  ${opus_LIBRARIES}
  ${opusfile_LIBRARIES}
)

if (WIN32)
  target_link_libraries(ChatNotifier PRIVATE
    ws2_32
  )
endif ()

add_custom_command(TARGET ChatNotifier POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/Assets
  $<TARGET_FILE_DIR:ChatNotifier>/Assets
)

install(TARGETS ChatNotifier)

if (CMAKE_INSTALL_BINDIR)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/Assets DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()

# Enable warnings as errors for the target, unless WIN32 is set
set_target_properties(ChatNotifier PROPERTIES
  $<$<NOT:$<BOOL:${WIN32}>>:COMPILE_WARNING_AS_ERROR ON>
)

# If WIN32, set _CRT_SECURE_NO_WARNINGS
target_compile_definitions(ChatNotifier PRIVATE
  $<$<BOOL:${WIN32}>:_CRT_SECURE_NO_WARNINGS>
)
