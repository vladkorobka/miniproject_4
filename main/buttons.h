#pragma once

#include <stdbool.h>

void buttons_init(void);

/* Повертають true рівно один раз на кожне фізичне натискання
 * (програмний дебаунс усередині), а не на весь час утримання. */
bool button_manual_pressed(void);
bool button_auto_pressed(void);
