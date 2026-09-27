#pragma once

#include <stdint.h>

typedef enum {
    EVT_ENCODER_DELTA,    /* поворот енкодера, value = +1 або -1 */
    EVT_BTN_MANUAL,       /* натиснута кнопка ручного насипання */
    EVT_BTN_AUTO_TOGGLE,  /* натиснута кнопка перемикання AUTO */
    EVT_AUTO_TIMER,       /* спрацював таймер авто-режиму */
} event_type_t;

typedef struct {
    event_type_t type;
    int8_t value;
} feeder_event_t;

typedef enum {
    LED_AUTO,     /* червоний: AUTO on/off */
    LED_FEEDING,  /* зелений: іде насипання */
} led_id_t;
