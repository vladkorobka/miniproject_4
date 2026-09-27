#pragma once

#include <stddef.h>

#include "notes.h"

void buzzer_init(void);

/* Плавно змінює частоту від freq_start до freq_end за duration_ms
 * (ковзання тону, наприклад "мяу" наприкінці мелодії). */
void buzzer_sweep(int freq_start, int freq_end, int duration_ms);

/* Блокуюче програє ноти по черзі з паузою NOTE_GAP_MS між ними. */
void buzzer_play(const note_t *notes, size_t count);
