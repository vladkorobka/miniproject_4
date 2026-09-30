#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "fb.h"
#include "menu.h"
#include "rtc_time.h"

typedef struct {
    ui_state_t     st;
    bool           time_valid;
    rtc_datetime_t time;
    int32_t        countdown_sec;
} ui_view_t;

#define UI_ROW_Y(row)   ((row) * FB_CHAR_H)

void ui_render(fb_t *fb, const ui_view_t *v);
