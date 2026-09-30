#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
} rtc_datetime_t;

uint8_t rtc_days_in_month(uint16_t year, uint8_t month);
bool rtc_datetime_is_valid(const rtc_datetime_t *t);
void rtc_datetime_add_seconds(rtc_datetime_t *t, uint32_t seconds);
