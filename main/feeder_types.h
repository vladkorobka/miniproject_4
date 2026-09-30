#pragma once

#include <stdint.h>

#define PORTION_MIN 1
#define PORTION_MAX 4

typedef enum {
    EVT_ENCODER_DELTA,
    EVT_BTN_MANUAL,
    EVT_AUTO_TIMER,
    EVT_ENC_CLICK,
    EVT_BTN_BACK,
} event_type_t;

typedef struct {
    event_type_t type;
    int8_t value;
} feeder_event_t;

typedef enum {
    LED_AUTO,
    LED_FEEDING,
} led_id_t;
