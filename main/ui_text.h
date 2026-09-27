#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rtc_time.h"

#define UI_LINE_LEN             16      /* символів у рядку: 128 px / 8 px */
#define UI_STATUS_PORTION_COL   8       /* колонка цифри порції в рядку стану */
#define UI_COUNTDOWN_MAX        99999   /* "99999s" - 6 символів праворуч */

/* Рядок стану: "HH:MM" або "--:--" | порція | "<N>s" вирівняно праворуч.
 * time == NULL -> "--:--"; countdown_sec < 0 -> права частина порожня. */
void ui_fmt_status(char out[UI_LINE_LEN + 1], const rtc_datetime_t *time,
                   uint8_t portion, int32_t countdown_sec);

/* Шкала меню Portion "  1   2   3   4 " і колонка цифри рівня в ній. */
void ui_fmt_portion_scale(char out[UI_LINE_LEN + 1]);
int  ui_portion_digit_col(uint8_t level);

/* "* Enabled" / "  Disabled" (доповнено пробілами): '*' - фактичний поточний режим. */
void ui_fmt_auto_option(char out[UI_LINE_LEN + 1], bool option_enabled, bool is_current);

/* Секунд до дедлайну з округленням вгору; 0, якщо дедлайн уже настав. */
int32_t ui_countdown_sec(int64_t deadline_us, int64_t now_us);
