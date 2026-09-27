#include "unity.h"
#include "melody.h"

void setUp(void) {}
void tearDown(void) {}

static void test_melody_is_not_empty(void)
{
    TEST_ASSERT_GREATER_THAN(0, feed_melody_len);
}

static void test_melody_fits_time_budget(void)
{
    /* Мелодія затримує кожне годування - тримаємо її короткою. */
    int total = feed_melody_duration_ms();
    TEST_ASSERT_GREATER_OR_EQUAL(1000, total);
    TEST_ASSERT_LESS_OR_EQUAL(2500, total);
}

static void test_notes_are_playable_by_piezo(void)
{
    for (size_t i = 0; i < feed_melody_len; i++) {
        int hz = feed_melody[i].hz;
        if (hz == REST) {
            continue;
        }
        TEST_ASSERT_INT_WITHIN_MESSAGE((NOTE_HZ_MAX - NOTE_HZ_MIN) / 2,
                                       (NOTE_HZ_MAX + NOTE_HZ_MIN) / 2, hz,
                                       "частота поза діапазоном зумера/LEDC");
    }
    TEST_ASSERT_GREATER_OR_EQUAL(NOTE_HZ_MIN, MEOW_HZ_START);
    TEST_ASSERT_LESS_OR_EQUAL(NOTE_HZ_MAX, MEOW_HZ_PEAK);
    TEST_ASSERT_GREATER_OR_EQUAL(NOTE_HZ_MIN, MEOW_HZ_END);
}

static void test_durations_are_whole_ticks(void)
{
    /* Тік FreeRTOS = 10 мс: інша тривалість тихо округлилась би. */
    for (size_t i = 0; i < feed_melody_len; i++) {
        TEST_ASSERT_GREATER_THAN(0, feed_melody[i].ms);
        TEST_ASSERT_EQUAL_INT(0, feed_melody[i].ms % 10);
    }
    TEST_ASSERT_EQUAL_INT(0, NOTE_GAP_MS % 10);
    TEST_ASSERT_EQUAL_INT(0, MEOW_UP_MS % 10);
    TEST_ASSERT_EQUAL_INT(0, MEOW_DOWN_MS % 10);
}

static void test_meow_goes_up_then_down(void)
{
    TEST_ASSERT_LESS_THAN(MEOW_HZ_PEAK, MEOW_HZ_START);
    TEST_ASSERT_LESS_THAN(MEOW_HZ_PEAK, MEOW_HZ_END);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_melody_is_not_empty);
    RUN_TEST(test_melody_fits_time_budget);
    RUN_TEST(test_notes_are_playable_by_piezo);
    RUN_TEST(test_durations_are_whole_ticks);
    RUN_TEST(test_meow_goes_up_then_down);
    return UNITY_END();
}
