#include "GameWindow.hpp"
#include "AnmManager.hpp"
#include "GameErrorContext.hpp"
#include "ScreenEffect.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "diffbuild.hpp"
#include "i18n.hpp"
#include "sdl2_renderer.hpp"
#include <cmath>

namespace th06
{
DIFFABLE_STATIC(GameWindow, g_GameWindow)
DIFFABLE_STATIC(i32, g_TickCountToEffectiveFramerate)
DIFFABLE_STATIC(f64, g_LastFrameTime)

#define FRAME_TIME (1000. / 60.)

#pragma var_order(res, viewport, slowdown, local_34, delta, curtime)
RenderResult GameWindow::Render()
{
    i32 res;
    f64 slowdown;
    D3DVIEWPORT8 viewport;
    f64 delta;
    u32 curtime;
    f64 local_34;

    if (this->lastActiveAppValue == 0)
    {
        return RENDER_RESULT_KEEP_RUNNING;
    }

    if (this->curFrame == 0)
    {
    LOOP_USING_GOTO_BECAUSE_WHY_NOT:
        if (g_Supervisor.cfg.frameskipConfig <= this->curFrame)
        {
            if (g_Supervisor.IsUnknown())
            {
                viewport.X = 0;
                viewport.Y = 0;
                viewport.Width = 640;
                viewport.Height = 480;
                viewport.MinZ = 0.0;
                viewport.MaxZ = 1.0;
                g_Renderer.SetViewport(viewport.X, viewport.Y, viewport.Width, viewport.Height);
                g_Renderer.Clear(g_Stage.skyFog.color, 1, 1);
                g_Renderer.SetViewport(g_Supervisor.viewport.X, g_Supervisor.viewport.Y,
                                       g_Supervisor.viewport.Width, g_Supervisor.viewport.Height);
            }
            g_Renderer.BeginScene();
            g_Chain.RunDrawChain();
            g_Renderer.EndScene();
            g_Renderer.SetTexture(0);
        }

        g_Supervisor.viewport.X = 0;
        g_Supervisor.viewport.Y = 0;
        g_Supervisor.viewport.Width = 640;
        g_Supervisor.viewport.Height = 480;
        g_Renderer.SetViewport(0, 0, 640, 480);
        res = g_Chain.RunCalcChain();
        g_SoundPlayer.PlaySounds();
        if (res == 0)
        {
            return RENDER_RESULT_EXIT_SUCCESS;
        }
        if (res == -1)
        {
            return RENDER_RESULT_EXIT_ERROR;
        }
        this->curFrame++;
    }

    if (g_Supervisor.cfg.windowed != false || g_Supervisor.ShouldRunAt60Fps())
    {
        if (this->curFrame != 0)
        {
            g_Supervisor.framerateMultiplier = 1.0;
            slowdown = timeGetTime();
            if (slowdown < g_LastFrameTime)
            {
                g_LastFrameTime = slowdown;
            }
            local_34 = fabs(slowdown - g_LastFrameTime);
            if (local_34 >= FRAME_TIME)
            {
                do
                {
                    g_LastFrameTime += FRAME_TIME;
                    local_34 -= FRAME_TIME;
                } while (local_34 >= FRAME_TIME);

                if (g_Supervisor.cfg.frameskipConfig < this->curFrame)
                    goto I_HAVE_NO_CLUE_WHY_BUT_I_MUST_JUMP_HERE;
                goto LOOP_USING_GOTO_BECAUSE_WHY_NOT;
            }
        }
    }

    if (g_Supervisor.cfg.windowed == false && !g_Supervisor.ShouldRunAt60Fps())
    {

        if (g_Supervisor.cfg.frameskipConfig >= this->curFrame)
        {
            Present();
            goto LOOP_USING_GOTO_BECAUSE_WHY_NOT;
        }

    I_HAVE_NO_CLUE_WHY_BUT_I_MUST_JUMP_HERE:
        Present();
        if (g_Supervisor.framerateMultiplier == 0.f)
        {
            if (2 <= g_TickCountToEffectiveFramerate)
            {
                curtime = timeGetTime();
                if (curtime < g_Supervisor.lastFrameTime)
                {
                    g_Supervisor.lastFrameTime = curtime;
                }
                delta = curtime - g_Supervisor.lastFrameTime;
                delta = (delta * 60.) / 2. / 1000.;
                delta /= (g_Supervisor.cfg.frameskipConfig + 1);
                if (delta >= .865)
                {
                    delta = 1.0;
                }
                else if (delta >= .6)
                {
                    delta = 0.8;
                }
                else
                {
                    delta = 0.5;
                }
                g_Supervisor.effectiveFramerateMultiplier = delta;
                g_Supervisor.lastFrameTime = curtime;
                g_TickCountToEffectiveFramerate = 0;
            }
        }
        else
        {
            g_Supervisor.effectiveFramerateMultiplier = g_Supervisor.framerateMultiplier;
        }
        this->curFrame = 0;
        g_TickCountToEffectiveFramerate = g_TickCountToEffectiveFramerate + 1;
    }
    return RENDER_RESULT_KEEP_RUNNING;
}

void GameWindow::Present()
{
    i32 unused;

    g_Renderer.EndFrame();

    if (g_GameWindow.screenWidth != 0)
    {
        static u32 s_lastPresentTime = 0;
        u32 curTime = timeGetTime();
        if (s_lastPresentTime != 0)
        {
            u32 elapsed = curTime - s_lastPresentTime;
            if (elapsed < 16)
            {
                SDL_Delay(16 - elapsed);
            }
        }
        s_lastPresentTime = timeGetTime();
    }
    g_AnmManager->TakeScreenshotIfRequested();
    if (g_Supervisor.unk198 != 0)
    {
        g_Supervisor.unk198--;
    }

    // Begin next frame (binds FBO)
    g_Renderer.BeginFrame();
    return;
}

i32 GameWindow::InitD3dInterface(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0)
    {
        GameErrorContext::Fatal(&g_GameErrorContext, TH_ERR_D3D_ERR_COULD_NOT_CREATE_OBJ);
        return 1;
    }
    return 0;
}

void GameWindow::CreateGameWindow(void *unused)
{
    u32 windowFlags;
    i32 width;
    i32 height;

    g_GameWindow.lastActiveAppValue = 0;
    g_GameWindow.isAppActive = 0;

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    if (g_Supervisor.cfg.windowed == 0)
    {
        windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_SHOWN;
        g_GameWindow.sdlWindow = SDL_CreateWindow(
            "Touhou 06", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT, windowFlags);
    }
    else
    {
        windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
        g_GameWindow.sdlWindow = SDL_CreateWindow(
            "Touhou 06", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT, windowFlags);
    }

    g_Supervisor.hwndGameWindow = (HWND)g_GameWindow.sdlWindow;
}

void GameWindow_ProcessEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            g_GameWindow.isAppClosing = 1;
            break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
            {
                g_GameWindow.lastActiveAppValue = 1;
                g_GameWindow.isAppActive = 0;
            }
            else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
            {
                g_GameWindow.lastActiveAppValue = 0;
                g_GameWindow.isAppActive = 1;
            }
            break;
        default:
            break;
        }
    }
}

#pragma var_order(using_d3d_hal, display_mode, present_params, camera_distance, half_height, half_width, aspect_ratio, \
                  field_of_view_y, up, at, eye, should_run_at_60_fps)
i32 GameWindow::InitD3dRendering(void)
{
    D3DXVECTOR3 eye;
    D3DXVECTOR3 at;
    D3DXVECTOR3 up;
    float half_width;
    float half_height;
    float aspect_ratio;
    float field_of_view_y;
    float camera_distance;

    if (!g_Supervisor.cfg.windowed)
    {
        SDL_DisplayMode dm;
        SDL_GetCurrentDisplayMode(0, &dm);
        g_GameWindow.screenWidth = dm.w;
        g_GameWindow.screenHeight = dm.h;
    }
    else
    {
        g_GameWindow.screenWidth = 0;
        g_GameWindow.screenHeight = 0;
    }

    SDL_GLContext glCtx = SDL_GL_CreateContext(g_GameWindow.sdlWindow);
    if (glCtx == NULL)
    {
        GameErrorContext::Fatal(&g_GameErrorContext, TH_ERR_D3D_INIT_FAILED);
        return 1;
    }

    SDL_GL_SetSwapInterval(g_Supervisor.cfg.windowed ? 1 : 0);

    g_Renderer.Init(g_GameWindow.sdlWindow, glCtx, GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT);

    g_Supervisor.lockableBackbuffer = 1;
    g_Supervisor.hasD3dHardwareVertexProcessing = 1;
    g_Supervisor.colorMode16Bits = 1;

    half_width = (float)GAME_WINDOW_WIDTH / 2.0;
    half_height = (float)GAME_WINDOW_HEIGHT / 2.0;
    aspect_ratio = (float)GAME_WINDOW_WIDTH / (float)GAME_WINDOW_HEIGHT;
    field_of_view_y = 0.52359879f;
    camera_distance = half_height / tanf(field_of_view_y / 2.0f);
    up.x = 0.0;
    up.y = 1.0;
    up.z = 0.0;
    at.x = half_width;
    at.y = -half_height;
    at.z = 0.0;
    eye.x = half_width;
    eye.y = -half_height;
    eye.z = -camera_distance;
    D3DXMatrixLookAtLH(&g_Supervisor.viewMatrix, &eye, &at, &up);
    D3DXMatrixPerspectiveFovLH(&g_Supervisor.projectionMatrix, field_of_view_y, aspect_ratio, 100.0, 10000.0);

    g_Renderer.viewMatrix = g_Supervisor.viewMatrix;
    g_Renderer.projectionMatrix = g_Supervisor.projectionMatrix;

    g_Supervisor.viewport.X = 0;
    g_Supervisor.viewport.Y = 0;
    g_Supervisor.viewport.Width = GAME_WINDOW_WIDTH;
    g_Supervisor.viewport.Height = GAME_WINDOW_HEIGHT;
    g_Supervisor.viewport.MinZ = 0.0f;
    g_Supervisor.viewport.MaxZ = 1.0f;

    InitD3dDevice();
    ScreenEffect::SetViewport(0);
    g_GameWindow.isAppClosing = 0;
    g_Supervisor.lastFrameTime = 0;
    g_Supervisor.framerateMultiplier = 0.0;
    return 0;
}

#pragma var_order(fogVal, fogDensity, anm1, anm2, anm3, anm4)
void GameWindow::InitD3dDevice(void)
{
    if (((g_Supervisor.cfg.opts >> GCOS_TURN_OFF_DEPTH_TEST) & 1) == 0)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);
    }

    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (((g_Supervisor.cfg.opts >> GCOS_SUPPRESS_USE_OF_GOROUD_SHADING) & 1) == 0)
    {
        glShadeModel(GL_SMOOTH);
    }
    else
    {
        glShadeModel(GL_FLAT);
    }

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GEQUAL, 4.0f / 255.0f);

    if (((g_Supervisor.cfg.opts >> GCOS_DONT_USE_FOG) & 1) == 0)
    {
        glEnable(GL_FOG);
        g_Renderer.fogEnabled = 1;
    }
    else
    {
        glDisable(GL_FOG);
        g_Renderer.fogEnabled = 0;
    }
    glFogi(GL_FOG_MODE, GL_LINEAR);
    f32 fogColor[4] = {0.627f, 0.627f, 0.627f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_START, 1000.0f);
    glFogf(GL_FOG_END, 5000.0f);
    glFogf(GL_FOG_DENSITY, 1.0f);

    g_Renderer.fogColor = 0xffa0a0a0;
    g_Renderer.fogStart = 1000.0f;
    g_Renderer.fogEnd = 5000.0f;

    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (g_AnmManager != NULL)
    {
        g_AnmManager->currentBlendMode = 0xff;
        g_AnmManager->currentColorOp = 0xff;
        g_AnmManager->currentVertexShader = 0xff;
        g_AnmManager->currentTexture = NULL;
    }
    g_Stage.skyFogNeedsSetup = 1;

    return;
}
}; // namespace th06
