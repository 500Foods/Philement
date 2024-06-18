#ifndef DISPLAY_H
#define DISPLAY_H

#include "lvgl.h"

lv_display_t *display_init(void);
void display_deinit(void);
void create_splash_screen(void);

#endif // DISPLAY_H
