#include "rtc_time.h"

static bool is_leap(uint16_t year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

uint8_t rtc_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (month < 1 || month > 12) {
        return 0;
    }
    if (month == 2 && is_leap(year)) {
        return 29;
    }
    return days[month - 1];
}

bool rtc_datetime_is_valid(const rtc_datetime_t *t)
{
    return t->year >= 2000 && t->year <= 2099 &&
           t->month >= 1 && t->month <= 12 &&
           t->day >= 1 && t->day <= rtc_days_in_month(t->year, t->month) &&
           t->hour < 24 && t->minute < 60 && t->second < 60;
}

void rtc_datetime_add_seconds(rtc_datetime_t *t, uint32_t seconds)
{
    uint32_t total = t->second + seconds;
    t->second = (uint8_t)(total % 60);

    total = t->minute + total / 60;
    t->minute = (uint8_t)(total % 60);

    total = t->hour + total / 60;
    t->hour = (uint8_t)(total % 24);

    for (uint32_t days = total / 24; days > 0; days--) {
        if (t->day < rtc_days_in_month(t->year, t->month)) {
            t->day++;
            continue;
        }
        t->day = 1;
        if (t->month < 12) {
            t->month++;
        } else {
            t->month = 1;
            t->year++;
        }
    }
}
