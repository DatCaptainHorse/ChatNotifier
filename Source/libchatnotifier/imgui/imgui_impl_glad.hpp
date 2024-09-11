#pragma once

#include <imgui.h>

#ifndef IMGUI_DISABLE

// Follow "Getting Started" link and check examples/ folder to learn about using backends!
IMGUI_IMPL_API bool     ImGui_ImplGlad_Init(const char* glsl_version = nullptr);
IMGUI_IMPL_API void     ImGui_ImplGlad_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplGlad_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplGlad_RenderDrawData(ImDrawData* draw_data);

// (Optional) Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool     ImGui_ImplGlad_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplGlad_DestroyFontsTexture();
IMGUI_IMPL_API bool     ImGui_ImplGlad_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplGlad_DestroyDeviceObjects();

#endif // #ifndef IMGUI_DISABLE
