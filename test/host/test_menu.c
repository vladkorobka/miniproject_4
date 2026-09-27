#include "unity.h"
#include "menu.h"

void setUp(void) {}
void tearDown(void) {}

static ui_state_t st;

static menu_action_t send(event_type_t evt, int8_t value)
{
    return menu_handle(&st, evt, value);
}

static void assert_action(menu_action_type_t type, uint8_t arg, menu_action_t a)
{
    TEST_ASSERT_EQUAL(type, a.type);
    TEST_ASSERT_EQUAL_UINT8(arg, a.arg);
}

/* Головний екран, порція 2, AUTO вимкнено. */
static void start(void)
{
    menu_init(&st, 2, false);
}

static void open_portion(void)
{
    start();
    send(EVT_ENC_CLICK, 0);   /* -> MENU_ROOT, курсор на Portion */
    send(EVT_ENC_CLICK, 0);   /* -> MENU_PORTION */
}

static void open_auto(bool auto_enabled)
{
    menu_init(&st, 2, auto_enabled);
    send(EVT_ENC_CLICK, 0);
    send(EVT_ENCODER_DELTA, +1);   /* курсор на Auto Mode */
    send(EVT_ENC_CLICK, 0);
}

static void test_init_state(void)
{
    start();
    TEST_ASSERT_EQUAL(SCREEN_MAIN, st.screen);
    TEST_ASSERT_EQUAL_UINT8(2, st.portion);
    TEST_ASSERT_EQUAL_UINT8(2, st.preview);
    TEST_ASSERT_FALSE(st.auto_enabled);
    TEST_ASSERT_FALSE(st.feeding);
}

static void test_main_ignores_rotation_and_back(void)
{
    start();
    assert_action(ACT_NONE, 0, send(EVT_ENCODER_DELTA, +1));
    assert_action(ACT_NONE, 0, send(EVT_BTN_BACK, 0));
    TEST_ASSERT_EQUAL(SCREEN_MAIN, st.screen);
    TEST_ASSERT_EQUAL_UINT8(2, st.portion);
}

static void test_main_click_opens_root_on_portion(void)
{
    start();
    assert_action(ACT_NONE, 0, send(EVT_ENC_CLICK, 0));
    TEST_ASSERT_EQUAL(SCREEN_MENU_ROOT, st.screen);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_PORTION, st.cursor);
}

static void test_root_cursor_clamps(void)
{
    start();
    send(EVT_ENC_CLICK, 0);
    send(EVT_ENCODER_DELTA, +1);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_AUTO, st.cursor);
    send(EVT_ENCODER_DELTA, +1);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_AUTO, st.cursor);
    send(EVT_ENCODER_DELTA, -1);
    send(EVT_ENCODER_DELTA, -1);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_PORTION, st.cursor);
}

static void test_root_back_returns_to_main(void)
{
    start();
    send(EVT_ENC_CLICK, 0);
    assert_action(ACT_NONE, 0, send(EVT_BTN_BACK, 0));
    TEST_ASSERT_EQUAL(SCREEN_MAIN, st.screen);
}

static void test_portion_opens_with_current_level(void)
{
    open_portion();
    TEST_ASSERT_EQUAL(SCREEN_MENU_PORTION, st.screen);
    TEST_ASSERT_EQUAL_UINT8(2, st.preview);
}

static void test_portion_rotation_previews_and_clamps(void)
{
    open_portion();
    assert_action(ACT_SHOW_LEDS, 3, send(EVT_ENCODER_DELTA, +1));
    assert_action(ACT_SHOW_LEDS, 4, send(EVT_ENCODER_DELTA, +1));
    assert_action(ACT_SHOW_LEDS, 4, send(EVT_ENCODER_DELTA, +1));
    TEST_ASSERT_EQUAL_UINT8(2, st.portion);   /* ще не підтверджено */
    for (int i = 0; i < 5; i++) {
        send(EVT_ENCODER_DELTA, -1);
    }
    TEST_ASSERT_EQUAL_UINT8(PORTION_MIN, st.preview);
}

static void test_portion_click_commits_and_returns_to_root(void)
{
    open_portion();
    send(EVT_ENCODER_DELTA, +1);
    send(EVT_ENCODER_DELTA, +1);
    assert_action(ACT_COMMIT_PORTION, 4, send(EVT_ENC_CLICK, 0));
    TEST_ASSERT_EQUAL_UINT8(4, st.portion);
    TEST_ASSERT_EQUAL(SCREEN_MENU_ROOT, st.screen);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_PORTION, st.cursor);
}

static void test_portion_back_reverts_preview(void)
{
    open_portion();
    send(EVT_ENCODER_DELTA, +1);
    send(EVT_ENCODER_DELTA, +1);
    assert_action(ACT_SHOW_LEDS, 2, send(EVT_BTN_BACK, 0));
    TEST_ASSERT_EQUAL_UINT8(2, st.portion);
    TEST_ASSERT_EQUAL_UINT8(2, st.preview);
    TEST_ASSERT_EQUAL(SCREEN_MENU_ROOT, st.screen);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_PORTION, st.cursor);
}

static void test_auto_cursor_starts_on_current_mode(void)
{
    open_auto(false);
    TEST_ASSERT_EQUAL(SCREEN_MENU_AUTO, st.screen);
    TEST_ASSERT_EQUAL_UINT8(MENU_AUTO_DISABLED, st.cursor);
    open_auto(true);
    TEST_ASSERT_EQUAL_UINT8(MENU_AUTO_ENABLED, st.cursor);
}

static void test_auto_cursor_clamps(void)
{
    open_auto(false);
    send(EVT_ENCODER_DELTA, +1);
    TEST_ASSERT_EQUAL_UINT8(MENU_AUTO_DISABLED, st.cursor);
    send(EVT_ENCODER_DELTA, -1);
    send(EVT_ENCODER_DELTA, -1);
    TEST_ASSERT_EQUAL_UINT8(MENU_AUTO_ENABLED, st.cursor);
}

static void test_auto_click_enables(void)
{
    open_auto(false);
    send(EVT_ENCODER_DELTA, -1);   /* курсор на Enabled */
    assert_action(ACT_SET_AUTO, 1, send(EVT_ENC_CLICK, 0));
    TEST_ASSERT_TRUE(st.auto_enabled);
    TEST_ASSERT_EQUAL(SCREEN_MENU_ROOT, st.screen);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_AUTO, st.cursor);
}

static void test_auto_click_disables(void)
{
    open_auto(true);
    send(EVT_ENCODER_DELTA, +1);   /* курсор на Disabled */
    assert_action(ACT_SET_AUTO, 0, send(EVT_ENC_CLICK, 0));
    TEST_ASSERT_FALSE(st.auto_enabled);
}

static void test_auto_click_same_mode_is_noop(void)
{
    open_auto(true);
    assert_action(ACT_NONE, 0, send(EVT_ENC_CLICK, 0));
    TEST_ASSERT_TRUE(st.auto_enabled);
    TEST_ASSERT_EQUAL(SCREEN_MENU_ROOT, st.screen);
}

static void test_auto_back_keeps_mode(void)
{
    open_auto(false);
    send(EVT_ENCODER_DELTA, -1);
    assert_action(ACT_NONE, 0, send(EVT_BTN_BACK, 0));
    TEST_ASSERT_FALSE(st.auto_enabled);
    TEST_ASSERT_EQUAL(SCREEN_MENU_ROOT, st.screen);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_AUTO, st.cursor);
}

static void test_manual_feeds_from_any_screen_and_keeps_menu(void)
{
    start();
    assert_action(ACT_FEED, 0, send(EVT_BTN_MANUAL, 0));
    TEST_ASSERT_EQUAL(SCREEN_MAIN, st.screen);

    open_portion();
    send(EVT_ENCODER_DELTA, +1);
    send(EVT_ENCODER_DELTA, +1);
    assert_action(ACT_FEED, 0, send(EVT_BTN_MANUAL, 0));
    TEST_ASSERT_EQUAL(SCREEN_MENU_PORTION, st.screen);
    TEST_ASSERT_EQUAL_UINT8(4, st.preview);   /* непідтверджений вибір зберігся */
    TEST_ASSERT_EQUAL_UINT8(2, st.portion);   /* годуємо підтвердженою */
}

static void test_auto_timer_feeds_when_enabled(void)
{
    menu_init(&st, 2, true);
    assert_action(ACT_FEED, 0, send(EVT_AUTO_TIMER, 0));
}

static void test_auto_timer_ignored_when_disabled(void)
{
    start();
    assert_action(ACT_NONE, 0, send(EVT_AUTO_TIMER, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_state);
    RUN_TEST(test_main_ignores_rotation_and_back);
    RUN_TEST(test_main_click_opens_root_on_portion);
    RUN_TEST(test_root_cursor_clamps);
    RUN_TEST(test_root_back_returns_to_main);
    RUN_TEST(test_portion_opens_with_current_level);
    RUN_TEST(test_portion_rotation_previews_and_clamps);
    RUN_TEST(test_portion_click_commits_and_returns_to_root);
    RUN_TEST(test_portion_back_reverts_preview);
    RUN_TEST(test_auto_cursor_starts_on_current_mode);
    RUN_TEST(test_auto_cursor_clamps);
    RUN_TEST(test_auto_click_enables);
    RUN_TEST(test_auto_click_disables);
    RUN_TEST(test_auto_click_same_mode_is_noop);
    RUN_TEST(test_auto_back_keeps_mode);
    RUN_TEST(test_manual_feeds_from_any_screen_and_keeps_menu);
    RUN_TEST(test_auto_timer_feeds_when_enabled);
    RUN_TEST(test_auto_timer_ignored_when_disabled);
    return UNITY_END();
}
