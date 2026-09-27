#include "unity.h"
#include "rtc_time.h"

void setUp(void) {}
void tearDown(void) {}

static rtc_datetime_t dt(uint16_t y, uint8_t mo, uint8_t d, uint8_t h, uint8_t mi, uint8_t s)
{
    rtc_datetime_t t = { .year = y, .month = mo, .day = d, .hour = h, .minute = mi, .second = s };
    return t;
}

static void assert_dt(rtc_datetime_t exp, rtc_datetime_t act)
{
    TEST_ASSERT_EQUAL_UINT16(exp.year, act.year);
    TEST_ASSERT_EQUAL_UINT8(exp.month, act.month);
    TEST_ASSERT_EQUAL_UINT8(exp.day, act.day);
    TEST_ASSERT_EQUAL_UINT8(exp.hour, act.hour);
    TEST_ASSERT_EQUAL_UINT8(exp.minute, act.minute);
    TEST_ASSERT_EQUAL_UINT8(exp.second, act.second);
}

static void test_days_in_month(void)
{
    TEST_ASSERT_EQUAL_UINT8(31, rtc_days_in_month(2026, 1));
    TEST_ASSERT_EQUAL_UINT8(28, rtc_days_in_month(2026, 2));
    TEST_ASSERT_EQUAL_UINT8(29, rtc_days_in_month(2028, 2));
    TEST_ASSERT_EQUAL_UINT8(28, rtc_days_in_month(2100, 2));
    TEST_ASSERT_EQUAL_UINT8(30, rtc_days_in_month(2026, 4));
    TEST_ASSERT_EQUAL_UINT8(0, rtc_days_in_month(2026, 0));
    TEST_ASSERT_EQUAL_UINT8(0, rtc_days_in_month(2026, 13));
}

static void test_is_valid(void)
{
    rtc_datetime_t ok = dt(2026, 9, 27, 14, 37, 5);
    TEST_ASSERT_TRUE(rtc_datetime_is_valid(&ok));

    rtc_datetime_t feb29 = dt(2026, 2, 29, 0, 0, 0);
    rtc_datetime_t hour24 = dt(2026, 1, 1, 24, 0, 0);
    rtc_datetime_t min60 = dt(2026, 1, 1, 0, 60, 0);
    rtc_datetime_t y1999 = dt(1999, 1, 1, 0, 0, 0);
    rtc_datetime_t day0 = dt(2026, 1, 0, 0, 0, 0);
    TEST_ASSERT_FALSE(rtc_datetime_is_valid(&feb29));
    TEST_ASSERT_FALSE(rtc_datetime_is_valid(&hour24));
    TEST_ASSERT_FALSE(rtc_datetime_is_valid(&min60));
    TEST_ASSERT_FALSE(rtc_datetime_is_valid(&y1999));
    TEST_ASSERT_FALSE(rtc_datetime_is_valid(&day0));
}

static void test_add_seconds_simple(void)
{
    rtc_datetime_t t = dt(2026, 9, 27, 14, 37, 5);
    rtc_datetime_add_seconds(&t, 30);
    assert_dt(dt(2026, 9, 27, 14, 37, 35), t);
}

static void test_add_seconds_crosses_minute_and_hour(void)
{
    rtc_datetime_t t = dt(2026, 9, 27, 14, 59, 45);
    rtc_datetime_add_seconds(&t, 30);
    assert_dt(dt(2026, 9, 27, 15, 0, 15), t);
}

static void test_add_seconds_crosses_midnight(void)
{
    rtc_datetime_t t = dt(2026, 9, 27, 23, 59, 45);
    rtc_datetime_add_seconds(&t, 30);
    assert_dt(dt(2026, 9, 28, 0, 0, 15), t);
}

static void test_add_seconds_crosses_month_end(void)
{
    rtc_datetime_t t = dt(2026, 4, 30, 23, 59, 50);
    rtc_datetime_add_seconds(&t, 30);
    assert_dt(dt(2026, 5, 1, 0, 0, 20), t);
}

static void test_add_seconds_crosses_year_end(void)
{
    rtc_datetime_t t = dt(2026, 12, 31, 23, 59, 59);
    rtc_datetime_add_seconds(&t, 1);
    assert_dt(dt(2027, 1, 1, 0, 0, 0), t);
}

static void test_add_seconds_leap_day(void)
{
    rtc_datetime_t t = dt(2028, 2, 28, 23, 59, 59);
    rtc_datetime_add_seconds(&t, 1);
    assert_dt(dt(2028, 2, 29, 0, 0, 0), t);
}

static void test_add_seconds_multiple_days(void)
{
    rtc_datetime_t t = dt(2026, 2, 27, 12, 0, 0);
    rtc_datetime_add_seconds(&t, 3u * 24u * 3600u);
    assert_dt(dt(2026, 3, 2, 12, 0, 0), t);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_days_in_month);
    RUN_TEST(test_is_valid);
    RUN_TEST(test_add_seconds_simple);
    RUN_TEST(test_add_seconds_crosses_minute_and_hour);
    RUN_TEST(test_add_seconds_crosses_midnight);
    RUN_TEST(test_add_seconds_crosses_month_end);
    RUN_TEST(test_add_seconds_crosses_year_end);
    RUN_TEST(test_add_seconds_leap_day);
    RUN_TEST(test_add_seconds_multiple_days);
    return UNITY_END();
}
