#pragma once
// Adapted from thprac_gui_impl_win32.h for native th06 integration.
// Original: thprac/thprac/src/thprac/thprac_gui_impl_win32.h

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <imgui.h>

IMGUI_IMPL_API bool  ImGui_ImplWin32_Init(void* hwnd);
IMGUI_IMPL_API void  ImGui_ImplWin32_Shutdown();
IMGUI_IMPL_API void  ImGui_ImplWin32_NewFrame();
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Key state helpers used by ThpracGui
IMGUI_IMPL_API int  ImGui_ImplWin32_GetKey(int vk);
IMGUI_IMPL_API int  ImGui_ImplWin32_GetKeyFrame(int vk);
// Update key state from WndProc (call inside WndProcHandler path)
IMGUI_IMPL_API void ImGui_ImplWin32_UpdateKeyState(UINT msg, WPARAM wParam, LPARAM lParam);
