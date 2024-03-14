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

target_include_directories(ChatNotifier PRIVATE
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

target_link_libraries(ChatNotifier PRIVATE
  fmt::fmt
  glbinding::glbinding
  glbinding::glbinding-aux
  glfw
  websocketpp::websocketpp
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
