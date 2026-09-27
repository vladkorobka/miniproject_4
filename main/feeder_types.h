#pragma once

#include <stdint.h>

/* ---------- Порція (тут, а не в config.h: потрібна чистій логіці без ESP-IDF) ---------- */
#define PORTION_MIN             1
#define PORTION_MAX             4

typedef enum {
    EVT_ENCODER_DELTA,    /* поворот енкодера, value = +1 або -1 */
    EVT_BTN_MANUAL,       /* натиснута кнопка ручного насипання */
    EVT_AUTO_TIMER,      /* спрацював таймер авто-режиму */
    EVT_ENC_CLICK,        /* клік енкодера (GPIO6) */
    EVT_BTN_BACK,         /* кнопка «Назад» (GPIO8) */
} event_type_t;

typedef struct {
    event_type_t type;
    int8_t value;
} feeder_event_t;

typedef enum {
    LED_AUTO,     /* червоний: AUTO on/off */
    LED_FEEDING,  /* зелений: іде насипання */
} led_id_t;
