#pragma once

#include <stdbool.h>
#include "rtc_time.h"

void ds1307_rtc_init(void);
bool ds1307_rtc_read(rtc_datetime_t *out);
