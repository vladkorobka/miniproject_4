#include "menu.h"

static menu_action_t act(menu_action_type_t type, uint8_t arg)
{
    menu_action_t a = { .type = type, .arg = arg };
    return a;
}

static uint8_t clamp_step(uint8_t value, int8_t delta, uint8_t lo, uint8_t hi)
{
    int r = (int)value + delta;
    if (r < lo) r = lo;
    if (r > hi) r = hi;
    return (uint8_t)r;
}

void menu_init(ui_state_t *st, uint8_t portion, bool auto_enabled)
{
    st->screen = SCREEN_MAIN;
    st->cursor = 0;
    st->portion = portion;
    st->preview = portion;
    st->auto_enabled = auto_enabled;
    st->feeding = false;
}

static menu_action_t handle_main(ui_state_t *st, event_type_t evt)
{
    if (evt == EVT_ENC_CLICK) {
        st->screen = SCREEN_MENU_ROOT;
        st->cursor = MENU_ROOT_PORTION;
    }
    return act(ACT_NONE, 0);
}

static menu_action_t handle_root(ui_state_t *st, event_type_t evt, int8_t value)
{
    switch (evt) {
    case EVT_ENCODER_DELTA:
        st->cursor = clamp_step(st->cursor, value, MENU_ROOT_PORTION, MENU_ROOT_AUTO);
        break;
    case EVT_ENC_CLICK:
        if (st->cursor == MENU_ROOT_PORTION) {
            st->screen = SCREEN_MENU_PORTION;
            st->preview = st->portion;
        } else {
            st->screen = SCREEN_MENU_AUTO;
            st->cursor = st->auto_enabled ? MENU_AUTO_ENABLED : MENU_AUTO_DISABLED;
        }
        break;
    case EVT_BTN_BACK:
        st->screen = SCREEN_MAIN;
        st->cursor = 0;
        break;
    default:
        break;
    }
    return act(ACT_NONE, 0);
}

static menu_action_t handle_portion(ui_state_t *st, event_type_t evt, int8_t value)
{
    switch (evt) {
    case EVT_ENCODER_DELTA:
        st->preview = clamp_step(st->preview, value, PORTION_MIN, PORTION_MAX);
        return act(ACT_SHOW_LEDS, st->preview);
    case EVT_ENC_CLICK:
        st->portion = st->preview;
        st->screen = SCREEN_MENU_ROOT;
        st->cursor = MENU_ROOT_PORTION;
        return act(ACT_COMMIT_PORTION, st->portion);
    case EVT_BTN_BACK:
        st->preview = st->portion;
        st->screen = SCREEN_MENU_ROOT;
        st->cursor = MENU_ROOT_PORTION;
        return act(ACT_SHOW_LEDS, st->portion);
    default:
        return act(ACT_NONE, 0);
    }
}

static menu_action_t handle_auto(ui_state_t *st, event_type_t evt, int8_t value)
{
    switch (evt) {
    case EVT_ENCODER_DELTA:
        st->cursor = clamp_step(st->cursor, value, MENU_AUTO_ENABLED, MENU_AUTO_DISABLED);
        return act(ACT_NONE, 0);
    case EVT_ENC_CLICK: {
        bool want = (st->cursor == MENU_AUTO_ENABLED);
        st->screen = SCREEN_MENU_ROOT;
        st->cursor = MENU_ROOT_AUTO;
        if (want == st->auto_enabled) {
            return act(ACT_NONE, 0);
        }
        st->auto_enabled = want;
        return act(ACT_SET_AUTO, want ? 1 : 0);
    }
    case EVT_BTN_BACK:
        st->screen = SCREEN_MENU_ROOT;
        st->cursor = MENU_ROOT_AUTO;
        return act(ACT_NONE, 0);
    default:
        return act(ACT_NONE, 0);
    }
}

menu_action_t menu_handle(ui_state_t *st, event_type_t evt, int8_t value)
{
    if (evt == EVT_BTN_MANUAL) {
        return act(ACT_FEED, 0);
    }
    if (evt == EVT_AUTO_TIMER) {
        return st->auto_enabled ? act(ACT_FEED, 0) : act(ACT_NONE, 0);
    }

    switch (st->screen) {
    case SCREEN_MAIN:         return handle_main(st, evt);
    case SCREEN_MENU_ROOT:    return handle_root(st, evt, value);
    case SCREEN_MENU_PORTION: return handle_portion(st, evt, value);
    case SCREEN_MENU_AUTO:    return handle_auto(st, evt, value);
    }
    return act(ACT_NONE, 0);
}
