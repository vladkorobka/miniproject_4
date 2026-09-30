#pragma once

#include "menu.h"

void ui_state_init(const ui_state_t *initial);
void ui_state_publish(const ui_state_t *st);
void ui_state_get(ui_state_t *out);
