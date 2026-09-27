#include "ds1307_codec.h"

#include <string.h>

#define REG_SEC_CH      0x80  /* Clock Halt: 1 = осцилятор зупинено */
#define REG_HOUR_12H    0x40  /* 1 = 12-годинний режим */

const uint8_t ds1307_marker[DS1307_MARKER_LEN] = { 'F', 'D', 'R', '4' };

uint8_t bcd_encode(uint8_t value)
{
    return (uint8_t)(((value / 10) << 4) | (value % 10));
}

uint8_t bcd_decode(uint8_t bcd)
{
    return (uint8_t)((bcd >> 4) * 10 + (bcd & 0x0F));
}

bool bcd_is_valid(uint8_t bcd)
{
    return (bcd >> 4) <= 9 && (bcd & 0x0F) <= 9;
}

void ds1307_encode(const rtc_datetime_t *t, uint8_t regs[DS1307_TIME_REG_COUNT])
{
    regs[0] = bcd_encode(t->second);                    /* CH = 0 */
    regs[1] = bcd_encode(t->minute);
    regs[2] = bcd_encode(t->hour);                      /* біт 6 = 0 -> 24h */
    regs[3] = 1;
    regs[4] = bcd_encode(t->day);
    regs[5] = bcd_encode(t->month);
    regs[6] = bcd_encode((uint8_t)(t->year - 2000));
}

bool ds1307_decode(const uint8_t regs[DS1307_TIME_REG_COUNT], rtc_datetime_t *out)
{
    if ((regs[0] & REG_SEC_CH) || (regs[2] & REG_HOUR_12H)) {
        return false;
    }

    uint8_t sec = regs[0] & 0x7F;
    uint8_t hour = regs[2] & 0x3F;
    if (!bcd_is_valid(sec) || !bcd_is_valid(regs[1]) || !bcd_is_valid(hour) ||
        !bcd_is_valid(regs[4]) || !bcd_is_valid(regs[5]) || !bcd_is_valid(regs[6])) {
        return false;
    }

    rtc_datetime_t t = {
        .year = (uint16_t)(2000 + bcd_decode(regs[6])),
        .month = bcd_decode(regs[5]),
        .day = bcd_decode(regs[4]),
        .hour = bcd_decode(hour),
        .minute = bcd_decode(regs[1]),
        .second = bcd_decode(sec),
    };
    if (!rtc_datetime_is_valid(&t)) {
        return false;
    }
    *out = t;
    return true;
}

bool ds1307_marker_matches(const uint8_t ram[DS1307_MARKER_LEN])
{
    return memcmp(ram, ds1307_marker, DS1307_MARKER_LEN) == 0;
}

rtc_start_t rtc_startup_decide(bool present, bool new_firmware, bool marker_ok, bool regs_ok)
{
    if (!present) {
        return RTC_START_ABSENT;
    }
    if (new_firmware) {
        return RTC_START_SET_FROM_BUILD;
    }
    if (marker_ok && regs_ok) {
        return RTC_START_VALID;
    }
    return RTC_START_INVALID;
}
