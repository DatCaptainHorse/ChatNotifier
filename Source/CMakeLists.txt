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
  gl3w
  glfw

  $<$<BOOL:${NON_NIX_BUILD}>:ogg>
  $<$<BOOL:${NON_NIX_BUILD}>:opus>
  $<$<BOOL:${NON_NIX_BUILD}>:OpusFile::opusfile>

  ${ogg_LIBRARIES}
  ${opus_LIBRARIES}
  ${opusfile_LIBRARIES}

  miniaudio
)

if (WIN32)
  target_link_libraries(ChatNotifier PRIVATE
    ws2_32
  )
endif ()

install(TARGETS ChatNotifier)

# Make sure we bring Assets folder with the executable, assuming NON_NIX_BUILD
if (NON_NIX_BUILD)
  add_custom_command(TARGET ChatNotifier POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Assets
    $<TARGET_FILE_DIR:ChatNotifier>/Assets
  )
endif ()
