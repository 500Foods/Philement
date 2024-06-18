#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

int render_test(int attempt) {
    printf("\nAttempt %d:\n", attempt);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    printf("SDL_Init successful\n");

    SDL_Window *window = SDL_CreateWindow("SDL2 2D Accel Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    printf("Window created successfully\n");

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    printf("Renderer created successfully\n");

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    printf("Rendered red background\n");

    printf("Press any key to close the window...\n");

    bool quit = false;
    SDL_Event event;
    Uint32 startTime = SDL_GetTicks();
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN) {
                quit = true;
            }
        }

        // Only render every 16ms (approx. 60 FPS)
        if (SDL_GetTicks() - startTime >= 16) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);
            startTime = SDL_GetTicks();
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("Resources freed\n");

    return 0;
}

int main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    for (int i = 0; i < 5; i++) {
        render_test(i + 1);
        SDL_Delay(1000);  // Wait for 1 second between attempts
    }

    return 0;
}
