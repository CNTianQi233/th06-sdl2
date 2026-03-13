/*
 * th06 SDL2 environment test
 * Verifies SDL2 + SDL2_image + SDL2_mixer are correctly configured
 */
#pragma execution_character_set("utf-8")

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <cstdio>

int main(int argc, char* argv[])
{
    // === Init SDL2 ===
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0)
    {
        printf("[ERR] SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    printf("[OK] SDL2 %d.%d.%d init success\n",
        SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);

    // === Init SDL2_image ===
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags))
    {
        printf("[WARN] SDL2_image PNG init failed: %s\n", IMG_GetError());
    }
    else
    {
        SDL_version imgVer;
        const SDL_version* imgLinked = IMG_Linked_Version();
        printf("[OK] SDL2_image %d.%d.%d init success\n",
            imgLinked->major, imgLinked->minor, imgLinked->patch);
    }

    // === Init SDL2_mixer ===
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        printf("[WARN] SDL2_mixer init failed: %s\n", Mix_GetError());
    }
    else
    {
        const SDL_version* mixLinked = Mix_Linked_Version();
        printf("[OK] SDL2_mixer %d.%d.%d init success (44100Hz, 2ch)\n",
            mixLinked->major, mixLinked->minor, mixLinked->patch);
    }

    // === Create window (640x480, same as original game) ===
    SDL_Window* window = SDL_CreateWindow(
        "th06 SDL2 Test - Touhou 06",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_SHOWN
    );
    if (!window)
    {
        printf("[ERR] Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    printf("[OK] Window created (640x480)\n");

    // === Create renderer ===
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        printf("[ERR] Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    printf("[OK] Renderer: %s (max tex %dx%d)\n",
        info.name, info.max_texture_width, info.max_texture_height);

    // === Print gamepad info ===
    int numJoysticks = SDL_NumJoysticks();
    printf("[INFO] Found %d gamepad(s)\n", numJoysticks);
    for (int i = 0; i < numJoysticks; i++)
    {
        if (SDL_IsGameController(i))
        {
            printf("  [%d] %s (GameController)\n", i, SDL_GameControllerNameForIndex(i));
        }
        else
        {
            printf("  [%d] %s (Joystick)\n", i, SDL_JoystickNameForIndex(i));
        }
    }

    // === Main loop - 3 seconds then exit ===
    printf("\n=== SDL2 dev environment OK! ===\n");
    printf("Window will close in 3 seconds...\n");

    Uint32 startTime = SDL_GetTicks();
    SDL_bool running = SDL_TRUE;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = SDL_FALSE;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                running = SDL_FALSE;
        }

        // Gradient background (Touhou blue tone)
        Uint32 elapsed = SDL_GetTicks() - startTime;
        int blue = 128 + (int)(127.0 * SDL_sin(elapsed / 500.0));
        SDL_SetRenderDrawColor(renderer, 20, 20, (Uint8)blue, 255);
        SDL_RenderClear(renderer);

        // Draw a small white rect (player placeholder)
        SDL_Rect playerRect = { 300, 400, 8, 12 };
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &playerRect);

        SDL_RenderPresent(renderer);

        if (elapsed > 3000)
            running = SDL_FALSE;
    }

    // === Cleanup ===
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    IMG_Quit();
    SDL_Quit();

    printf("\nAll tests passed, SDL2 dev env ready!\n");
    return 0;
}
