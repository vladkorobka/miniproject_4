#pragma once

#include <stddef.h>
#include "notes.h"

extern const note_t feed_melody[];
extern const size_t feed_melody_len;

#define MEOW_HZ_START   700
#define MEOW_HZ_PEAK    1400
#define MEOW_HZ_END     650
#define MEOW_UP_MS      150
#define MEOW_DOWN_MS    300

int feed_melody_duration_ms(void);
