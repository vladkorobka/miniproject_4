#include "ui.h"

#include "ui_text.h"

static void draw_row(fb_t *fb, int row, const char *text)
{
    fb_draw_text(fb, 0, UI_ROW_Y(row), text);
}

static void invert_row(fb_t *fb, int row)
{
    fb_invert_rect(fb, 0, UI_ROW_Y(row), FB_WIDTH, FB_CHAR_H);
}

void ui_render(fb_t *fb, const ui_view_t *v)
{
    const ui_state_t *st = &v->st;
    char line[UI_LINE_LEN + 1];

    fb_clear(fb);
    ui_fmt_status(line, v->time_valid ? &v->time : NULL, st->portion, v->countdown_sec);

    /* Під час насипання екран меню ховається, але стан меню зберігається
     * і повертається після циклу (спек 4.5). */
    if (st->feeding) {
        draw_row(fb, 0, line);
        draw_row(fb, 2, "   FEEDING...");
        return;
    }

    switch (st->screen) {
    case SCREEN_MAIN:
        draw_row(fb, 1, line);
        break;

    case SCREEN_MENU_ROOT:
        draw_row(fb, 0, "MENU");
        draw_row(fb, 1, " Portion");
        draw_row(fb, 2, " Auto Mode");
        invert_row(fb, 1 + st->cursor);
        break;

    case SCREEN_MENU_PORTION: {
        draw_row(fb, 0, "Portion");
        ui_fmt_portion_scale(line);
        draw_row(fb, 2, line);
        int col = ui_portion_digit_col(st->preview);
        fb_invert_rect(fb, (col - 1) * FB_CHAR_W, UI_ROW_Y(2), 3 * FB_CHAR_W, FB_CHAR_H);
        break;
    }

    case SCREEN_MENU_AUTO:
        draw_row(fb, 0, "Auto Mode");
        ui_fmt_auto_option(line, true, st->auto_enabled);
        draw_row(fb, 1, line);
        ui_fmt_auto_option(line, false, !st->auto_enabled);
        draw_row(fb, 2, line);
        invert_row(fb, 1 + st->cursor);
        break;
    }
}
