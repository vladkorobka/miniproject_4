#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Календарний час без часового поясу - локальний, як його зберігає DS1307. */
typedef struct {
    uint16_t year;    /* 2000..2099 - діапазон DS1307 */
    uint8_t  month;   /* 1..12 */
    uint8_t  day;     /* 1..31 */
    uint8_t  hour;    /* 0..23 */
    uint8_t  minute;  /* 0..59 */
    uint8_t  second;  /* 0..59 */
} rtc_datetime_t;

/* Кількість днів у місяці з урахуванням високосного року; 0 для month поза 1..12. */
uint8_t rtc_days_in_month(uint16_t year, uint8_t month);

bool rtc_datetime_is_valid(const rtc_datetime_t *t);

/* Додає секунди з переносом через хвилину, годину, добу, місяць і рік. */
void rtc_datetime_add_seconds(rtc_datetime_t *t, uint32_t seconds);
