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

/* Пункти кореневого меню і меню Auto Mode (значення поля cursor). */
#define MENU_ROOT_PORTION   0
#define MENU_ROOT_AUTO      1
#define MENU_AUTO_ENABLED   0
#define MENU_AUTO_DISABLED  1

/* Увесь стан, який бачить користувач. Власник - feeder_task;
 * display_task отримує копію через ui_state_get(). */
typedef struct {
    ui_screen_t screen;
    uint8_t cursor;         /* курсор у MENU_ROOT / MENU_AUTO */
    uint8_t portion;        /* підтверджена порція */
    uint8_t preview;        /* рівень попереднього перегляду в MENU_PORTION */
    bool    auto_enabled;
    bool    feeding;        /* виставляє feeder_task навколо feed_cycle() */
} ui_state_t;

typedef enum {
    ACT_NONE,
    ACT_SHOW_LEDS,       /* arg = рівень для шкали LED (перегляд або відкат) */
    ACT_COMMIT_PORTION,  /* arg = нова підтверджена порція: LED + NVS */
    ACT_SET_AUTO,        /* arg = 0/1: LED AUTO, NVS, старт/стоп таймера */
    ACT_FEED,            /* годувати підтвердженою порцією */
} menu_action_type_t;

typedef struct {
    menu_action_type_t type;
    uint8_t arg;
} menu_action_t;

void menu_init(ui_state_t *st, uint8_t portion, bool auto_enabled);

/* Чиста функція переходу: оновлює *st і повертає побічний ефект,
 * який має виконати feeder_task. Заліза не торкається. */
menu_action_t menu_handle(ui_state_t *st, event_type_t evt, int8_t value);
