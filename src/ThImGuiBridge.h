#pragma once
// Bridge between game code and ImGui overlay.
// All functions declared as C for cross-compiler compatibility.

#include "sdl2_compat.hpp"

#ifdef __cplusplus
extern "C" {
#endif

// Call once after SDL window + GL context are created.
// window: SDL_Window*, glContext: SDL_GLContext (unused param kept for API compat)
void ThImGui_Init(void* window, void* glContext);

// Call after GL context recreation to rebuild GPU objects.
void ThImGui_CreateObjects(void);

// Call before GL context teardown to free GPU objects.
void ThImGui_InvalidateObjects(void);

// Call every frame after game rendering, before SDL_GL_SwapWindow.
void ThImGui_Render(void);

#ifdef __cplusplus
}
#endif
