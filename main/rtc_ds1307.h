#pragma once

#include <stdbool.h>

#include "rtc_time.h"

/* Алгоритм старту зі спеку 5.3: за потреби встановлює час зі збірки.
 * Викликати один раз після settings_init() та i2c_bus_init(). */
void ds1307_rtc_init(void);

/* Поточний час. false - час невалідний зі старту або читання не вдалося. */
bool ds1307_rtc_read(rtc_datetime_t *out);
