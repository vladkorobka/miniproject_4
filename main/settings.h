#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Енергонезалежне зберігання налаштувань у NVS: розмір порції і стан
 * AUTO переживають перезавантаження та зникнення живлення. */
void settings_init(void);

uint8_t settings_get_portion(void);
bool    settings_get_auto_enabled(void);

/* Запис відбувається лише якщо значення реально змінилось,
 * щоб не зношувати flash на кожному кліку енкодера. */
void settings_set_portion(uint8_t portion);
void settings_set_auto_enabled(bool enabled);

/* Мітка збірки (BUILD_STAMP), при якій востаннє встановлювався час RTC.
 * false, якщо мітки ще немає або NVS недоступний. */
bool settings_get_build_stamp(char *out, size_t out_len);
void settings_set_build_stamp(const char *stamp);
