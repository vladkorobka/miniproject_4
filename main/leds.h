#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "feeder_types.h"

void leds_init(void);
void update_portion_leds(uint8_t portion);
void led_set(led_id_t id, bool on);
