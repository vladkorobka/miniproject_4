#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "fb.h"
#include "menu.h"
#include "rtc_time.h"

/* Усе, що потрібно для одного кадру. */
typedef struct {
    ui_state_t     st;
    bool           time_valid;
    rtc_datetime_t time;
    int32_t        countdown_sec;   /* < 0 -> AUTO вимкнено, праворуч порожньо */
} ui_view_t;

#define UI_ROW_Y(row)   ((row) * FB_CHAR_H)

/* Малює кадр з нуля (спек, розділ 4). Заліза не торкається. */
void ui_render(fb_t *fb, const ui_view_t *v);
