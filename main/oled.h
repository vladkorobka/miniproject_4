#pragma once

#include <stdbool.h>

#include "fb.h"

/* SSD1306 128x64 через esp_lcd на спільній шині I2C.
 * false - дисплей не відповідає; годівничка тоді працює без екрана. */
bool oled_init(void);

/* Надсилає весь кадр. false - помилка I2C (кадр просто пропускається). */
bool oled_flush(const fb_t *fb);
