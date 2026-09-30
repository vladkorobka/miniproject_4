#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "feeder_types.h"

extern QueueHandle_t event_queue;

void feed_cycle(uint8_t portion_size);

void input_task(void *arg);

void feeder_task(void *arg);

void auto_timer_start(uint32_t interval_sec);
void auto_timer_stop(void);

bool auto_timer_remaining_sec(int32_t *sec);
