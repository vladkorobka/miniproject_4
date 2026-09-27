#include <string.h>

#include "unity.h"
#include "ds1307_codec.h"

void setUp(void) {}
void tearDown(void) {}

static void test_bcd_roundtrip(void)
{
    for (uint8_t v = 0; v < 100; v++) {
        uint8_t b = bcd_encode(v);
        TEST_ASSERT_TRUE(bcd_is_valid(b));
        TEST_ASSERT_EQUAL_UINT8(v, bcd_decode(b));
    }
    TEST_ASSERT_EQUAL_HEX8(0x59, bcd_encode(59));
}

static void test_bcd_invalid_nibbles(void)
{
    TEST_ASSERT_FALSE(bcd_is_valid(0x1A));
    TEST_ASSERT_FALSE(bcd_is_valid(0xA1));
    TEST_ASSERT_FALSE(bcd_is_valid(0xFF));
}

static void test_encode_known_time(void)
{
    rtc_datetime_t t = { .year = 2026, .month = 9, .day = 27, .hour = 14, .minute = 37, .second = 5 };
    uint8_t regs[DS1307_TIME_REG_COUNT];
    const uint8_t expected[DS1307_TIME_REG_COUNT] = { 0x05, 0x37, 0x14, 0x01, 0x27, 0x09, 0x26 };

    ds1307_encode(&t, regs);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, regs, DS1307_TIME_REG_COUNT);
}

static void test_decode_known_time(void)
{
    const uint8_t regs[DS1307_TIME_REG_COUNT] = { 0x05, 0x37, 0x14, 0x07, 0x27, 0x09, 0x26 };
    rtc_datetime_t t;

    TEST_ASSERT_TRUE(ds1307_decode(regs, &t));
    TEST_ASSERT_EQUAL_UINT16(2026, t.year);
    TEST_ASSERT_EQUAL_UINT8(9, t.month);
    TEST_ASSERT_EQUAL_UINT8(27, t.day);
    TEST_ASSERT_EQUAL_UINT8(14, t.hour);
    TEST_ASSERT_EQUAL_UINT8(37, t.minute);
    TEST_ASSERT_EQUAL_UINT8(5, t.second);
}

static void test_decode_rejects_clock_halt(void)
{
    const uint8_t regs[DS1307_TIME_REG_COUNT] = { 0x80 | 0x05, 0x37, 0x14, 0x01, 0x27, 0x09, 0x26 };
    rtc_datetime_t t;
    TEST_ASSERT_FALSE(ds1307_decode(regs, &t));
}

static void test_decode_rejects_12h_mode(void)
{
    const uint8_t regs[DS1307_TIME_REG_COUNT] = { 0x05, 0x37, 0x40 | 0x02, 0x01, 0x27, 0x09, 0x26 };
    rtc_datetime_t t;
    TEST_ASSERT_FALSE(ds1307_decode(regs, &t));
}

static void test_decode_rejects_all_ff(void)
{
    uint8_t regs[DS1307_TIME_REG_COUNT];
    rtc_datetime_t t;
    memset(regs, 0xFF, sizeof regs);
    TEST_ASSERT_FALSE(ds1307_decode(regs, &t));
}

static void test_decode_rejects_all_zero(void)
{
    const uint8_t regs[DS1307_TIME_REG_COUNT] = { 0 };   /* день і місяць 0 */
    rtc_datetime_t t;
    TEST_ASSERT_FALSE(ds1307_decode(regs, &t));
}

static void test_decode_rejects_impossible_date(void)
{
    const uint8_t regs[DS1307_TIME_REG_COUNT] = { 0x00, 0x00, 0x00, 0x01, 0x30, 0x02, 0x26 };  /* 30 лютого */
    rtc_datetime_t t;
    TEST_ASSERT_FALSE(ds1307_decode(regs, &t));
}

static void test_decode_leaves_out_untouched_on_failure(void)
{
    uint8_t regs[DS1307_TIME_REG_COUNT];
    rtc_datetime_t t = { .year = 2030, .month = 1, .day = 1, .hour = 1, .minute = 1, .second = 1 };
    memset(regs, 0xFF, sizeof regs);
    ds1307_decode(regs, &t);
    TEST_ASSERT_EQUAL_UINT16(2030, t.year);
}

static void test_marker_matches(void)
{
    uint8_t ram[DS1307_MARKER_LEN];
    memcpy(ram, ds1307_marker, DS1307_MARKER_LEN);
    TEST_ASSERT_TRUE(ds1307_marker_matches(ram));
    ram[3] ^= 0x01;
    TEST_ASSERT_FALSE(ds1307_marker_matches(ram));
}

static void test_decide_absent_wins_over_new_firmware(void)
{
    TEST_ASSERT_EQUAL(RTC_START_ABSENT, rtc_startup_decide(false, true, false, false));
    TEST_ASSERT_EQUAL(RTC_START_ABSENT, rtc_startup_decide(false, false, true, true));
}

static void test_decide_new_firmware_sets_time(void)
{
    TEST_ASSERT_EQUAL(RTC_START_SET_FROM_BUILD, rtc_startup_decide(true, true, false, false));
    TEST_ASSERT_EQUAL(RTC_START_SET_FROM_BUILD, rtc_startup_decide(true, true, true, true));
}

static void test_decide_valid_needs_marker_and_regs(void)
{
    TEST_ASSERT_EQUAL(RTC_START_VALID, rtc_startup_decide(true, false, true, true));
    TEST_ASSERT_EQUAL(RTC_START_INVALID, rtc_startup_decide(true, false, false, true));
    TEST_ASSERT_EQUAL(RTC_START_INVALID, rtc_startup_decide(true, false, true, false));
}

static void test_stamp_consumed_on_first_boot_even_without_rtc(void)
{
    /* RTC, підключений пізніше, не повинен отримати застарілий час збірки. */
    TEST_ASSERT_TRUE(rtc_startup_consumes_stamp(true, RTC_START_ABSENT));
    TEST_ASSERT_TRUE(rtc_startup_consumes_stamp(true, RTC_START_SET_FROM_BUILD));
    TEST_ASSERT_FALSE(rtc_startup_consumes_stamp(false, RTC_START_VALID));
    TEST_ASSERT_FALSE(rtc_startup_consumes_stamp(false, RTC_START_INVALID));
    TEST_ASSERT_FALSE(rtc_startup_consumes_stamp(false, RTC_START_ABSENT));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bcd_roundtrip);
    RUN_TEST(test_bcd_invalid_nibbles);
    RUN_TEST(test_encode_known_time);
    RUN_TEST(test_decode_known_time);
    RUN_TEST(test_decode_rejects_clock_halt);
    RUN_TEST(test_decode_rejects_12h_mode);
    RUN_TEST(test_decode_rejects_all_ff);
    RUN_TEST(test_decode_rejects_all_zero);
    RUN_TEST(test_decode_rejects_impossible_date);
    RUN_TEST(test_decode_leaves_out_untouched_on_failure);
    RUN_TEST(test_marker_matches);
    RUN_TEST(test_decide_absent_wins_over_new_firmware);
    RUN_TEST(test_decide_new_firmware_sets_time);
    RUN_TEST(test_decide_valid_needs_marker_and_regs);
    RUN_TEST(test_stamp_consumed_on_first_boot_even_without_rtc);
    return UNITY_END();
}
