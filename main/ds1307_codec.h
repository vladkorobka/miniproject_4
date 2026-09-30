#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rtc_time.h"

#define DS1307_TIME_REG_COUNT   7

#define DS1307_RAM_MARKER_REG   0x08
#define DS1307_MARKER_LEN       4

extern const uint8_t ds1307_marker[DS1307_MARKER_LEN];

uint8_t bcd_encode(uint8_t value);
uint8_t bcd_decode(uint8_t bcd);
bool    bcd_is_valid(uint8_t bcd);

void ds1307_encode(const rtc_datetime_t *t, uint8_t regs[DS1307_TIME_REG_COUNT]);

bool ds1307_decode(const uint8_t regs[DS1307_TIME_REG_COUNT], rtc_datetime_t *out);

bool ds1307_marker_matches(const uint8_t ram[DS1307_MARKER_LEN]);

typedef enum {
    RTC_START_ABSENT,
    RTC_START_SET_FROM_BUILD,
    RTC_START_INVALID,
    RTC_START_VALID,
} rtc_start_t;

rtc_start_t rtc_startup_decide(bool present, bool new_firmware, bool marker_ok, bool regs_ok);

bool rtc_startup_consumes_stamp(bool new_firmware, rtc_start_t decision);
