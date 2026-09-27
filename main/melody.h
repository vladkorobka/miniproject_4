#pragma once

#include <stddef.h>

#include "notes.h"

/* Мелодія на старті насипання: фанфара "та-да-да-ДАМ! та-ДААМ!". */
extern const note_t feed_melody[];
extern const size_t feed_melody_len;

/* Після фанфари - "мяу": ковзання частоти вгору, потім униз (buzzer_sweep). */
#define MEOW_HZ_START   700
#define MEOW_HZ_PEAK    1400
#define MEOW_HZ_END     650
#define MEOW_UP_MS      150
#define MEOW_DOWN_MS    300

/* Повна тривалість сигналу (ноти + паузи між ними + "мяу"), мс. */
int feed_melody_duration_ms(void);
