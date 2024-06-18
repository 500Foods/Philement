#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include "lvgl.h"
#include "display.h"
#include <SDL2/SDL.h>
#include <signal.h>

#define MAX_INIT_ATTEMPTS 3
#define INIT_RETRY_DELAY 2000 // 2 seconds
#define EXIT_DELAY 5000 // 5 seconds

static bool keep_running = true;
static lv_display_t *disp = NULL;

static void signal_handler(int signum) {
    printf("Caught signal %d\n", signum);
    keep_running = false;
}

static void *lvgl_thread(void *arg)
{
    (void)arg;
    
    printf("LVGL thread started\n");
    while(keep_running) {
        lv_timer_handler();
        SDL_Delay(5);
    }
    printf("LVGL thread ended\n");

    return NULL;
}

void cleanup_resources() {
    printf("Cleaning up resources. Please wait...\n");
    
    if (disp) {
        display_deinit();
    }
    SDL_Quit();
    
    // Wait for resources to be fully released
    SDL_Delay(EXIT_DELAY);
    
    printf("Cleanup complete. It's now safe to restart.\n");
}

int main(void)
{
    printf("Program started\n");

    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Register the cleanup function to be called at program exit
    atexit(cleanup_resources);

    printf("Initializing LVGL\n");
    lv_init();
    printf("LVGL initialized\n");

    int init_attempts = 0;
    while (disp == NULL && init_attempts < MAX_INIT_ATTEMPTS) {
        printf("Initializing display (attempt %d)\n", init_attempts + 1);
        disp = display_init();
        if (disp == NULL) {
            printf("Display initialization failed. Retrying in 2 seconds...\n");
            SDL_Delay(INIT_RETRY_DELAY);
            init_attempts++;
        }
    }

    if (disp == NULL) {
        printf("Failed to initialize display after %d attempts. Exiting.\n", MAX_INIT_ATTEMPTS);
        return 1;
    }
    printf("Display initialized\n");

    printf("Creating LVGL thread\n");
pthread_t lvgl_thread_id;
if (pthread_create(&lvgl_thread_id, NULL, lvgl_thread, NULL) != 0) {
    printf("Failed to create LVGL thread\n");
    return 1;
}
printf("LVGL thread created\n");

// Add a small delay to ensure LVGL thread is running
SDL_Delay(100);

// Create the splash screen
create_splash_screen();

// Force an initial redraw
lv_obj_invalidate(lv_scr_act());
lv_refr_now(disp);

printf("Entering main event loop\n");
    SDL_Event event;
    SDL_bool done = SDL_FALSE;
    while (!done && keep_running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                printf("Quit event received\n");
                done = SDL_TRUE;
            } else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
                    printf("Window exposed event received\n");
                    lv_obj_invalidate(lv_scr_act());
                    lv_refr_now(disp);
                }
            }
        }
        SDL_Delay(10);
    }
    printf("Exited main event loop\n");

    keep_running = false;  // Signal the LVGL thread to exit

    printf("Waiting for LVGL thread to finish\n");
    pthread_join(lvgl_thread_id, NULL);
    printf("LVGL thread joined\n");

    return 0;
}
