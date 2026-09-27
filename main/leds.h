#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "feeder_types.h"

void leds_init(void);

/* Шкала-градусник: вмикає LED 1..portion, гасить решту. */
void update_portion_leds(uint8_t portion);

/* Керування окремими режимними LED (AUTO / FEEDING). */
void led_set(led_id_t id, bool on);
