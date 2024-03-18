add_executable(ChatNotifier
  main.cpp

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
)

target_include_directories(ChatNotifier PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
  ${ogg_INCLUDE_DIRS}
  ${opus_INCLUDE_DIRS}
  ${opusfile_INCLUDE_DIRS}
)

# Specifically include websocketpp before the rest of the includes
target_include_directories(ChatNotifier BEFORE PRIVATE
  ${websocketpp_SOURCE_DIR}
)

target_link_libraries(ChatNotifier PRIVATE
  fmt::fmt-header-only
  miniaudio
  glfw

  $<$<BOOL:${NON_NIX_BUILD}>:glbinding::glbinding>
  $<$<NOT:$<BOOL:${NON_NIX_BUILD}>>:gl3w>

  $<$<BOOL:${NON_NIX_BUILD}>:asio>
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
