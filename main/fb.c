#include "fb.h"

#include <string.h>

#include "font8x8_basic.h"

static bool in_bounds(int x, int y)
{
    return x >= 0 && x < FB_WIDTH && y >= 0 && y < FB_HEIGHT;
}

void fb_clear(fb_t *fb)
{
    memset(fb->buf, 0, sizeof fb->buf);
}

void fb_set_pixel(fb_t *fb, int x, int y, bool on)
{
    if (!in_bounds(x, y)) {
        return;
    }
    uint8_t *byte = &fb->buf[(y / 8) * FB_WIDTH + x];
    uint8_t mask = (uint8_t)(1u << (y % 8));
    if (on) {
        *byte |= mask;
    } else {
        *byte &= (uint8_t)~mask;
    }
}

bool fb_get_pixel(const fb_t *fb, int x, int y)
{
    if (!in_bounds(x, y)) {
        return false;
    }
    return (fb->buf[(y / 8) * FB_WIDTH + x] >> (y % 8)) & 1;
}

void fb_draw_char(fb_t *fb, int x, int y, char c)
{
    unsigned char ch = (unsigned char)c;
    if (ch < 0x20 || ch > 0x7E) {
        ch = '?';
    }
    for (int row = 0; row < 8; row++) {
        uint8_t bits = font8x8_basic[ch][row];
        for (int col = 0; col < 8; col++) {
            bool on = (bits >> col) & 1;
            fb_set_pixel(fb, x + col, y + 2 * row, on);
            fb_set_pixel(fb, x + col, y + 2 * row + 1, on);
        }
    }
}

void fb_draw_text(fb_t *fb, int x, int y, const char *s)
{
    for (; *s != '\0' && x < FB_WIDTH; s++, x += FB_CHAR_W) {
        fb_draw_char(fb, x, y, *s);
    }
}

void fb_invert_rect(fb_t *fb, int x, int y, int w, int h)
{
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            fb_set_pixel(fb, xx, yy, !fb_get_pixel(fb, xx, yy));
        }
    }
}
