#pragma once

#include <stddef.h>
#include "notes.h"

void buzzer_init(void);
void buzzer_sweep(int freq_start, int freq_end, int duration_ms);
void buzzer_play(const note_t *notes, size_t count);
