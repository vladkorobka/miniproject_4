#pragma once

#include <stdbool.h>
#include <stdint.h>

#define FB_WIDTH    128
#define FB_HEIGHT   64
#define FB_CHAR_W   8
#define FB_CHAR_H   16

typedef struct {
    uint8_t buf[FB_WIDTH * FB_HEIGHT / 8];
} fb_t;

void fb_clear(fb_t *fb);
void fb_set_pixel(fb_t *fb, int x, int y, bool on);
bool fb_get_pixel(const fb_t *fb, int x, int y);

void fb_draw_char(fb_t *fb, int x, int y, char c);
void fb_draw_text(fb_t *fb, int x, int y, const char *s);

void fb_invert_rect(fb_t *fb, int x, int y, int w, int h);
