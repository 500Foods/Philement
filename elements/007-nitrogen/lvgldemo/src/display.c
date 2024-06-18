#include "display.h"
#include "lv_conf.h"
#include "lvgl.h"
#include <SDL2/SDL.h>
#include <stdio.h>

#define DISP_HOR_RES 800
#define DISP_VER_RES 600

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;


static void sdl_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    printf("Flush called for area: (%d,%d) to (%d,%d)\n", area->x1, area->y1, area->x2, area->y2);

    SDL_Rect rect;
    rect.x = area->x1;
    rect.y = area->y1;
    rect.w = lv_area_get_width(area);
    rect.h = lv_area_get_height(area);

    SDL_UpdateTexture(texture, &rect, px_map, rect.w * sizeof(uint32_t));
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    lv_display_flush_ready(disp);
}

lv_display_t *display_init(void)
{
    printf("Initializing display...\n");

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return NULL;
    }

    window = SDL_CreateWindow("LVGL Demo",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              DISP_HOR_RES, DISP_VER_RES, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return NULL;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return NULL;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                DISP_HOR_RES, DISP_VER_RES);
    if (texture == NULL) {
        printf("SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return NULL;
    }

    // Initial render to show a black screen
//    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
//    SDL_RenderClear(renderer);
//    SDL_RenderPresent(renderer);

    lv_display_t *disp = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
    lv_display_set_flush_cb(disp, sdl_display_flush);

    static lv_color_t buf1[DISP_HOR_RES * DISP_VER_RES];
static lv_color_t buf2[DISP_HOR_RES * DISP_VER_RES];
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    printf("Display initialization complete\n");
    return disp;
}


void display_deinit(void)
{
    printf("Deinitializing display...\n");
    if (texture) {
        SDL_DestroyTexture(texture);
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
    printf("Display deinitialization complete\n");
}

void create_splash_screen(void)
{
    printf("Creating splash screen\n");

    lv_obj_t *scr = lv_scr_act();

    // Set the screen's background color to black
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // Get the screen dimensions
    lv_coord_t screen_width = lv_obj_get_width(scr);
    lv_coord_t screen_height = lv_obj_get_height(scr);

    // Create gradient styles for top and bottom
    static lv_style_t gradient_style_top;
    static lv_style_t gradient_style_bottom;
    lv_style_init(&gradient_style_top);
    lv_style_init(&gradient_style_bottom);

    // Set the gradient colors and directions
    lv_style_set_bg_color(&gradient_style_top, lv_color_make(255, 0, 0));  // Red at the top
    lv_style_set_bg_grad_color(&gradient_style_top, lv_color_black());     // Black at the bottom
    lv_style_set_bg_grad_dir(&gradient_style_top, LV_GRAD_DIR_VER);        // Vertical gradient

    lv_style_set_bg_color(&gradient_style_bottom, lv_color_black());       // Black at the top
    lv_style_set_bg_grad_color(&gradient_style_bottom, lv_color_make(255, 0, 0)); // Red at the bottom
    lv_style_set_bg_grad_dir(&gradient_style_bottom, LV_GRAD_DIR_VER);     // Vertical gradient

    // Remove any border and rounding for both styles
    lv_style_set_border_width(&gradient_style_top, 0);
    lv_style_set_radius(&gradient_style_top, 0);
    lv_style_set_border_width(&gradient_style_bottom, 0);
    lv_style_set_radius(&gradient_style_bottom, 0);

    // Create the four gradient rectangles
    lv_obj_t *rect_tl = lv_obj_create(scr);
    lv_obj_set_size(rect_tl, screen_width / 2, screen_height / 2);
    lv_obj_set_pos(rect_tl, 0, 0);
    lv_obj_add_style(rect_tl, &gradient_style_top, 0);

    lv_obj_t *rect_tr = lv_obj_create(scr);
    lv_obj_set_size(rect_tr, screen_width / 2, screen_height / 2);
    lv_obj_set_pos(rect_tr, screen_width / 2, 0);
    lv_obj_add_style(rect_tr, &gradient_style_top, 0);

    lv_obj_t *rect_bl = lv_obj_create(scr);
    lv_obj_set_size(rect_bl, screen_width / 2, screen_height / 2);
    lv_obj_set_pos(rect_bl, 0, screen_height / 2);
    lv_obj_add_style(rect_bl, &gradient_style_bottom, 0);

    lv_obj_t *rect_br = lv_obj_create(scr);
    lv_obj_set_size(rect_br, screen_width / 2, screen_height / 2);
    lv_obj_set_pos(rect_br, screen_width / 2, screen_height / 2);
    lv_obj_add_style(rect_br, &gradient_style_bottom, 0);

    // Create title label
    lv_obj_t *title_label = lv_label_create(scr);
    lv_label_set_text(title_label, "Nitrogen LVGL Demo");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, -20);  // 20 pixels above center

    // Create subtitle label
    lv_obj_t *subtitle_label = lv_label_create(scr);
    lv_label_set_text(subtitle_label, "Part of the Philement Project");
    lv_obj_set_style_text_font(subtitle_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(subtitle_label, lv_color_white(), 0);
    lv_obj_align(subtitle_label, LV_ALIGN_CENTER, 0, 20);  // 20 pixels below center

    printf("Splash screen creation complete\n");
}
