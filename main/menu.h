#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "feeder_types.h"

typedef enum {
    SCREEN_MAIN,
    SCREEN_MENU_ROOT,
    SCREEN_MENU_PORTION,
    SCREEN_MENU_AUTO,
} ui_screen_t;

#define MENU_ROOT_PORTION   0
#define MENU_ROOT_AUTO      1
#define MENU_AUTO_ENABLED   0
#define MENU_AUTO_DISABLED  1

typedef struct {
    ui_screen_t screen;
    uint8_t cursor;
    uint8_t portion;
    uint8_t preview;
    bool    auto_enabled;
    bool    feeding;
} ui_state_t;

typedef enum {
    ACT_NONE,
    ACT_SHOW_LEDS,
    ACT_COMMIT_PORTION,
    ACT_SET_AUTO,
    ACT_FEED,
} menu_action_type_t;

typedef struct {
    menu_action_type_t type;
    uint8_t arg;
} menu_action_t;

void menu_init(ui_state_t *st, uint8_t portion, bool auto_enabled);

menu_action_t menu_handle(ui_state_t *st, event_type_t evt, int8_t value);
