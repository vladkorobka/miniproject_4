#include "ui_text.h"

#include <stdio.h>
#include <string.h>

static void blank(char out[UI_LINE_LEN + 1])
{
    memset(out, ' ', UI_LINE_LEN);
    out[UI_LINE_LEN] = '\0';
}

void ui_fmt_status(char out[UI_LINE_LEN + 1], const rtc_datetime_t *time,
                   uint8_t portion, int32_t countdown_sec)
{
    char buf[16];

    blank(out);

    if (time != NULL) {
        snprintf(buf, sizeof buf, "%02u:%02u", (unsigned)time->hour, (unsigned)time->minute);
    } else {
        strcpy(buf, "--:--");
    }
    memcpy(out, buf, 5);

    out[UI_STATUS_PORTION_COL] = (char)('0' + portion);

    if (countdown_sec >= 0) {
        if (countdown_sec > UI_COUNTDOWN_MAX) {
            countdown_sec = UI_COUNTDOWN_MAX;
        }
        int n = snprintf(buf, sizeof buf, "%lds", (long)countdown_sec);
        memcpy(out + UI_LINE_LEN - n, buf, (size_t)n);
    }
}

void ui_fmt_portion_scale(char out[UI_LINE_LEN + 1])
{
    blank(out);
    for (uint8_t level = 1; level <= 4; level++) {
        out[ui_portion_digit_col(level)] = (char)('0' + level);
    }
}

int ui_portion_digit_col(uint8_t level)
{
    return 4 * level - 2;
}

void ui_fmt_auto_option(char out[UI_LINE_LEN + 1], bool option_enabled, bool is_current)
{
    const char *label = option_enabled ? "Enabled" : "Disabled";

    blank(out);
    out[0] = is_current ? '*' : ' ';
    memcpy(out + 2, label, strlen(label));
}

int32_t ui_countdown_sec(int64_t deadline_us, int64_t now_us)
{
    int64_t left = deadline_us - now_us;
    if (left <= 0) {
        return 0;
    }
    int64_t sec = (left + 999999) / 1000000;
    return sec > INT32_MAX ? INT32_MAX : (int32_t)sec;
}
