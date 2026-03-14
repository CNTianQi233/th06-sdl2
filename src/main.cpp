#include "sdl2_compat.hpp"
#include <SDL.h>
#include <stdio.h>

#include "AnmManager.hpp"
#include "Chain.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GameWindow.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "i18n.hpp"
#include "utils.hpp"

using namespace th06;

int main(int argc, char *argv[])
{
    i32 renderResult = 0;

    if (utils::CheckForRunningGameInstance())
    {
        g_GameErrorContext.Flush();

        return 1;
    }

    if (g_Supervisor.LoadConfig(TH_CONFIG_FILE) != ZUN_SUCCESS)
    {
        g_GameErrorContext.Flush();
        return -1;
    }

    if (GameWindow::InitD3dInterface())
    {
        g_GameErrorContext.Flush();
        return 1;
    }

restart:
    GameWindow::CreateGameWindow(NULL);

    if (GameWindow::InitD3dRendering())
    {
        g_GameErrorContext.Flush();
        return 1;
    }

    g_SoundPlayer.InitializeDSound((HWND)g_GameWindow.sdlWindow);
    Controller::GetJoystickCaps();
    Controller::ResetKeyboard();

    g_AnmManager = new AnmManager();

    if (Supervisor::RegisterChain() != ZUN_SUCCESS)
    {
        goto stop;
    }
    if (!g_Supervisor.cfg.windowed)
    {
        SDL_ShowCursor(SDL_DISABLE);
    }

    g_GameWindow.curFrame = 0;

    while (!g_GameWindow.isAppClosing)
    {
        GameWindow_ProcessEvents();
        renderResult = g_GameWindow.Render();
        if (renderResult != 0)
        {
            goto stop;
        }
    }

stop:
    // Manually invoke Supervisor cleanup before chain release, because
    // g_Chain.Release() only deletes heap-allocated chain elements without
    // calling their deletedCallback.  This ensures MidiOutput, PBG3 archives,
    // text buffers and the SDL controller are properly released.
    Supervisor::DeletedCallback(&g_Supervisor);

    g_Chain.Release();
    g_SoundPlayer.Release();

    delete g_AnmManager;
    g_AnmManager = NULL;

    SDL_DestroyWindow(g_GameWindow.sdlWindow);
    g_GameWindow.sdlWindow = NULL;

    if (renderResult == 2)
    {
        g_GameErrorContext.ResetContext();

        GameErrorContext::Log(&g_GameErrorContext, TH_ERR_OPTION_CHANGED_RESTART);

        if (!g_Supervisor.cfg.windowed)
        {
            SDL_ShowCursor(SDL_ENABLE);
        }
        goto restart;
    }

    FileSystem::WriteDataToFile(TH_CONFIG_FILE, &g_Supervisor.cfg, sizeof(g_Supervisor.cfg));

    SDL_ShowCursor(SDL_ENABLE);
    g_GameErrorContext.Flush();
    SDL_Quit();
    return 0;
}
