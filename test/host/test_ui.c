#include <string.h>

#include "unity.h"
#include "ui.h"
#include "ui_text.h"

void setUp(void) {}
void tearDown(void) {}

static const rtc_datetime_t T1437 = { .year = 2026, .month = 9, .day = 27, .hour = 14, .minute = 37, .second = 5 };

/* ---------- ui_text ---------- */

static void test_status_full(void)
{
    char s[UI_LINE_LEN + 1];
    ui_fmt_status(s, &T1437, 3, 27);
    TEST_ASSERT_EQUAL_STRING("14:37   3    27s", s);
}

static void test_status_invalid_time(void)
{
    char s[UI_LINE_LEN + 1];
    ui_fmt_status(s, NULL, 3, 27);
    TEST_ASSERT_EQUAL_STRING("--:--   3    27s", s);
}

static void test_status_auto_off_leaves_right_empty(void)
{
    char s[UI_LINE_LEN + 1];
    ui_fmt_status(s, &T1437, 1, -1);
    TEST_ASSERT_EQUAL_STRING("14:37   1       ", s);
}

static void test_status_zero_countdown(void)
{
    char s[UI_LINE_LEN + 1];
    ui_fmt_status(s, &T1437, 4, 0);
    TEST_ASSERT_EQUAL_STRING("14:37   4     0s", s);
}

static void test_status_clamps_huge_countdown(void)
{
    char s[UI_LINE_LEN + 1];
    ui_fmt_status(s, &T1437, 2, 123456);
    TEST_ASSERT_EQUAL_STRING("14:37   2 99999s", s);
    TEST_ASSERT_EQUAL_size_t(UI_LINE_LEN, strlen(s));
}

static void test_status_pads_single_digit_time(void)
{
    const rtc_datetime_t t = { .year = 2026, .month = 1, .day = 1, .hour = 7, .minute = 5, .second = 0 };
    char s[UI_LINE_LEN + 1];
    ui_fmt_status(s, &t, 1, -1);
    TEST_ASSERT_EQUAL_STRING("07:05   1       ", s);
}

static void test_portion_scale(void)
{
    char s[UI_LINE_LEN + 1];
    ui_fmt_portion_scale(s);
    TEST_ASSERT_EQUAL_STRING("  1   2   3   4 ", s);
    for (uint8_t level = 1; level <= 4; level++) {
        TEST_ASSERT_EQUAL_CHAR('0' + level, s[ui_portion_digit_col(level)]);
    }
}

static void test_auto_options(void)
{
    char s[UI_LINE_LEN + 1];
    ui_fmt_auto_option(s, true, true);
    TEST_ASSERT_EQUAL_STRING("* Enabled       ", s);
    ui_fmt_auto_option(s, false, false);
    TEST_ASSERT_EQUAL_STRING("  Disabled      ", s);
    ui_fmt_auto_option(s, false, true);
    TEST_ASSERT_EQUAL_STRING("* Disabled      ", s);
}

static void test_countdown_sec_rounds_up(void)
{
    TEST_ASSERT_EQUAL_INT32(27, ui_countdown_sec(26200000, 0));
    TEST_ASSERT_EQUAL_INT32(27, ui_countdown_sec(27000000, 0));
    TEST_ASSERT_EQUAL_INT32(1, ui_countdown_sec(1000001, 1000000));
}

static void test_countdown_sec_past_deadline_is_zero(void)
{
    TEST_ASSERT_EQUAL_INT32(0, ui_countdown_sec(5000000, 5000000));
    TEST_ASSERT_EQUAL_INT32(0, ui_countdown_sec(5000000, 9000000));
}

/* ---------- ui_render: порівняння з еталонним кадром ---------- */

static fb_t actual, expected;

static ui_view_t view(ui_screen_t screen, uint8_t cursor)
{
    ui_view_t v;
    memset(&v, 0, sizeof v);
    v.st.screen = screen;
    v.st.cursor = cursor;
    v.st.portion = 3;
    v.st.preview = 3;
    v.time_valid = true;
    v.time = T1437;
    v.countdown_sec = 27;
    return v;
}

static void assert_frames_equal(void)
{
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected.buf, actual.buf, sizeof expected.buf);
}

static void test_render_main(void)
{
    ui_view_t v = view(SCREEN_MAIN, 0);
    ui_render(&actual, &v);
    fb_clear(&expected);
    fb_draw_text(&expected, 0, UI_ROW_Y(1), "14:37   3    27s");
    assert_frames_equal();
}

static void test_render_main_invalid_time(void)
{
    ui_view_t v = view(SCREEN_MAIN, 0);
    v.time_valid = false;
    ui_render(&actual, &v);
    fb_clear(&expected);
    fb_draw_text(&expected, 0, UI_ROW_Y(1), "--:--   3    27s");
    assert_frames_equal();
}

static void test_render_root_cursor_on_auto(void)
{
    ui_view_t v = view(SCREEN_MENU_ROOT, MENU_ROOT_AUTO);
    ui_render(&actual, &v);
    fb_clear(&expected);
    fb_draw_text(&expected, 0, UI_ROW_Y(0), "MENU");
    fb_draw_text(&expected, 0, UI_ROW_Y(1), " Portion");
    fb_draw_text(&expected, 0, UI_ROW_Y(2), " Auto Mode");
    fb_invert_rect(&expected, 0, UI_ROW_Y(2), FB_WIDTH, FB_CHAR_H);
    assert_frames_equal();
}

static void test_render_portion_highlights_preview(void)
{
    ui_view_t v = view(SCREEN_MENU_PORTION, 0);
    v.st.preview = 4;
    ui_render(&actual, &v);
    fb_clear(&expected);
    fb_draw_text(&expected, 0, UI_ROW_Y(0), "Portion");
    fb_draw_text(&expected, 0, UI_ROW_Y(2), "  1   2   3   4 ");
    fb_invert_rect(&expected, 13 * FB_CHAR_W, UI_ROW_Y(2), 3 * FB_CHAR_W, FB_CHAR_H);
    assert_frames_equal();
}

static void test_render_auto_marks_current_and_cursor(void)
{
    ui_view_t v = view(SCREEN_MENU_AUTO, MENU_AUTO_ENABLED);
    v.st.auto_enabled = false;   /* курсор на Enabled, але фактично вимкнено */
    ui_render(&actual, &v);
    fb_clear(&expected);
    fb_draw_text(&expected, 0, UI_ROW_Y(0), "Auto Mode");
    fb_draw_text(&expected, 0, UI_ROW_Y(1), "  Enabled       ");
    fb_draw_text(&expected, 0, UI_ROW_Y(2), "* Disabled      ");
    fb_invert_rect(&expected, 0, UI_ROW_Y(1), FB_WIDTH, FB_CHAR_H);
    assert_frames_equal();
}

static void test_render_feeding_overrides_screen(void)
{
    ui_view_t v = view(SCREEN_MENU_PORTION, 0);
    v.st.feeding = true;
    ui_render(&actual, &v);
    fb_clear(&expected);
    fb_draw_text(&expected, 0, UI_ROW_Y(0), "14:37   3    27s");
    fb_draw_text(&expected, 0, UI_ROW_Y(2), "   FEEDING...");
    assert_frames_equal();
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_status_full);
    RUN_TEST(test_status_invalid_time);
    RUN_TEST(test_status_auto_off_leaves_right_empty);
    RUN_TEST(test_status_zero_countdown);
    RUN_TEST(test_status_clamps_huge_countdown);
    RUN_TEST(test_status_pads_single_digit_time);
    RUN_TEST(test_portion_scale);
    RUN_TEST(test_auto_options);
    RUN_TEST(test_countdown_sec_rounds_up);
    RUN_TEST(test_countdown_sec_past_deadline_is_zero);
    RUN_TEST(test_render_main);
    RUN_TEST(test_render_main_invalid_time);
    RUN_TEST(test_render_root_cursor_on_auto);
    RUN_TEST(test_render_portion_highlights_preview);
    RUN_TEST(test_render_auto_marks_current_and_cursor);
    RUN_TEST(test_render_feeding_overrides_screen);
    return UNITY_END();
}
