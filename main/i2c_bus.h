#pragma once

#include "driver/i2c_master.h"

/* Одна шина на RTC і OLED. Помилка конфігурації - баг прошивки, тому abort. */
void i2c_bus_init(void);
i2c_master_bus_handle_t i2c_bus_get(void);

/* Опитує всі 7-бітні адреси і пише знайдені в лог (діагностика підключення). */
void i2c_bus_scan(void);
