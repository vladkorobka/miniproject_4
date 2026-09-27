#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rtc_time.h"

/* Регістри 0x00..0x06: секунди, хвилини, години, день тижня, дата, місяць, рік. */
#define DS1307_TIME_REG_COUNT   7

/* Початок RAM DS1307 (56 байт), що живе, поки модуль має живлення. */
#define DS1307_RAM_MARKER_REG   0x08
#define DS1307_MARKER_LEN       4

/* Маркер "час встановлено прошивкою, і живлення відтоді не пропадало".
 * Після подачі живлення без батарейки регістри DS1307 не визначені,
 * тож одного біта CH для цього недостатньо. */
extern const uint8_t ds1307_marker[DS1307_MARKER_LEN];

uint8_t bcd_encode(uint8_t value);   /* 0..99 */
uint8_t bcd_decode(uint8_t bcd);
bool    bcd_is_valid(uint8_t bcd);   /* обидві тетради 0..9 */

/* 24-годинний режим, CH = 0 (осцилятор запущено), день тижня = 1 (не використовується). */
void ds1307_encode(const rtc_datetime_t *t, uint8_t regs[DS1307_TIME_REG_COUNT]);

/* false (і *out не змінюється), якщо CH = 1, 12-годинний режим,
 * некоректний BCD або неіснуюча дата. */
bool ds1307_decode(const uint8_t regs[DS1307_TIME_REG_COUNT], rtc_datetime_t *out);

bool ds1307_marker_matches(const uint8_t ram[DS1307_MARKER_LEN]);

typedef enum {
    RTC_START_ABSENT,          /* чип не відповідає */
    RTC_START_SET_FROM_BUILD,  /* перший старт нової прошивки: записати час збірки */
    RTC_START_INVALID,         /* живлення RTC пропадало: --:-- до наступної прошивки */
    RTC_START_VALID,           /* час іде з моменту встановлення */
} rtc_start_t;

/* Алгоритм старту (спек, розділ 5.3). regs_ok - ds1307_decode() повернула true. */
rtc_start_t rtc_startup_decide(bool present, bool new_firmware, bool marker_ok, bool regs_ok);

/* Чи зберігати BUILD_STAMP у NVS після старту. Мітка "витрачається" на першому
 * ж старті нової прошивки, навіть якщо RTC не відповів або запис не вдався:
 * RTC, підключений пізніше, інакше отримав би застарілий час збірки і показав
 * би його як валідний. Краще --:-- до наступної прошивки. */
bool rtc_startup_consumes_stamp(bool new_firmware, rtc_start_t decision);
