#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "feeder_types.h"

/* Спільна черга подій між input_task і feeder_task.
 * Оголошена тут, визначена (виділена) у main.c. */
extern QueueHandle_t event_queue;

/* Лінійний блокуючий цикл видачі корму: звук -> відкрити заслінку ->
 * крутити мотор -> зачекати -> закрити заслінку. */
void feed_cycle(uint8_t portion_size);

/* Задача, що обробляє кнопки (енкодер шле події напряму з ISR). */
void input_task(void *arg);

/* Задача, що володіє станом (portion_size, auto_enabled) і викликає feed_cycle(). */
void feeder_task(void *arg);

/* Періодичний таймер для авто-режиму. */
void auto_timer_start(uint32_t interval_sec);
void auto_timer_stop(void);

/* Секунд до наступної автоподачі (округлено вгору).
 * false - таймер не запущено (AUTO вимкнено). Безпечно з будь-якої задачі. */
bool auto_timer_remaining_sec(int32_t *sec);

static inline uint8_t clamp_u8(int value, int lo, int hi)
{
    if (value < lo) return (uint8_t)lo;
    if (value > hi) return (uint8_t)hi;
    return (uint8_t)value;
}
