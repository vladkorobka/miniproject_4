#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rtc_time.h"

#define UI_LINE_LEN             16
#define UI_STATUS_PORTION_COL   8
#define UI_COUNTDOWN_MAX        99999

void ui_fmt_status(char out[UI_LINE_LEN + 1], const rtc_datetime_t *time,
                   uint8_t portion, int32_t countdown_sec);

void ui_fmt_portion_scale(char out[UI_LINE_LEN + 1]);
int  ui_portion_digit_col(uint8_t level);
void ui_fmt_auto_option(char out[UI_LINE_LEN + 1], bool option_enabled, bool is_current);

int32_t ui_countdown_sec(int64_t deadline_us, int64_t now_us);
