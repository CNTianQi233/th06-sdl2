#pragma once
// Adapted from thprac_gui_impl_dx8.h for native th06 integration.
// Original: thprac/thprac/src/thprac/thprac_gui_impl_dx8.h

#include <imgui.h>

struct IDirect3DDevice8;

IMGUI_IMPL_API bool ImGui_ImplDX8_Init(IDirect3DDevice8* device);
IMGUI_IMPL_API void ImGui_ImplDX8_Shutdown();
IMGUI_IMPL_API void ImGui_ImplDX8_NewFrame();
IMGUI_IMPL_API void ImGui_ImplDX8_RenderDrawData(ImDrawData* draw_data);
IMGUI_IMPL_API bool ImGui_ImplDX8_CreateDeviceObjects();
IMGUI_IMPL_API void ImGui_ImplDX8_InvalidateDeviceObjects();
