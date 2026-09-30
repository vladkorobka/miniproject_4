#pragma once

#include <stdbool.h>

#include "fb.h"

bool oled_init(void);
bool oled_flush(const fb_t *fb);
