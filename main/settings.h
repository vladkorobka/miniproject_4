#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void settings_init(void);
uint8_t settings_get_portion(void);
bool    settings_get_auto_enabled(void);
void settings_set_portion(uint8_t portion);
void settings_set_auto_enabled(bool enabled);
bool settings_get_build_stamp(char *out, size_t out_len);
void settings_set_build_stamp(const char *stamp);
