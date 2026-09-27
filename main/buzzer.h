#pragma once

void buzzer_init(void);

/* Плавно змінює частоту від freq_start до freq_end за duration_ms
 * (ефект сирени, попереджувальний сигнал перед насипанням). */
void buzzer_sweep(int freq_start, int freq_end, int duration_ms);
