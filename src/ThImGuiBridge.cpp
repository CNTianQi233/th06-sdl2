// ImGui bridge implementation for SDL2 + OpenGL.
// Conditionally compiled: requires TH06_IMGUI_ENABLED and the ImGui SDL2+OpenGL2 backends.

#include "ThImGuiBridge.h"

#ifdef TH06_IMGUI_ENABLED

#include "ThpracGui.hpp"
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl2.h>

extern "C" {

void ThImGui_Init(void* window, void* glContext)
{
    (void)glContext;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    const ImWchar* chineseRanges = io.Fonts->GetGlyphRangesChineseSimplifiedCommon();
    ImFont* font = io.Fonts->AddFontFromFileTTF("fonts/msyh.ttc", 16.0f, nullptr, chineseRanges);
    if (!font)
        io.Fonts->AddFontDefault();

    ImGui_ImplSDL2_InitForOpenGL((SDL_Window*)window, glContext);
    ImGui_ImplOpenGL2_Init();
    th06::ThpracGui::Init();
}

void ThImGui_CreateObjects(void)
{
    ImGui_ImplOpenGL2_CreateDeviceObjects();
}

void ThImGui_InvalidateObjects(void)
{
    ImGui_ImplOpenGL2_DestroyDeviceObjects();
}

void ThImGui_Render(void)
{
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    th06::ThpracGui::Render();
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

} // extern "C"

#else // !TH06_IMGUI_ENABLED

extern "C" {

void ThImGui_Init(void* window, void* glContext)
{
    (void)window;
    (void)glContext;
}

void ThImGui_CreateObjects(void)
{
}

void ThImGui_InvalidateObjects(void)
{
}

void ThImGui_Render(void)
{
}

} // extern "C"

#endif // TH06_IMGUI_ENABLED
