#include "servo.h"

#include <stdint.h>

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

#include "config.h"

#define SERVO_TIMER      LEDC_TIMER_0
#define SERVO_CHANNEL    LEDC_CHANNEL_0
#define SERVO_MODE       LEDC_LOW_SPEED_MODE
#define SERVO_RES        LEDC_TIMER_13_BIT
#define SERVO_RES_BITS   13
#define SERVO_FREQ_HZ    50

/* Стандартні межі імпульсу SG90. Якщо заслінка не до кінця
 * закривається/відкривається на реальному сервоприводі - підправ ці
 * значення емпірично (деякі клони відхиляються на ±100-200 мкс). */
#define SERVO_MIN_US     500    /* 0 градусів */
#define SERVO_MAX_US     2500   /* 180 градусів */
#define SERVO_PERIOD_US  20000  /* 20 мс = 50 Гц */

void servo_init(void)
{
    ledc_timer_config_t timer_conf = {
        .speed_mode      = SERVO_MODE,
        .timer_num       = SERVO_TIMER,
        .duty_resolution = SERVO_RES,
        .freq_hz         = SERVO_FREQ_HZ,
        /* Джерело такту задане явно: на ESP32-S3 воно спільне для всіх
         * low-speed таймерів LEDC, тому LEDC_AUTO_CLK на серво і зумері
         * могло б вибрати різні джерела і другий таймер не налаштувався б. */
        .clk_cfg         = LEDC_USE_APB_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t channel_conf = {
        .gpio_num   = SERVO_PIN,
        .speed_mode = SERVO_MODE,
        .channel    = SERVO_CHANNEL,
        .timer_sel  = SERVO_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));

    /* Після подачі живлення фізичний кут заслінки невідомий (той, у якому
     * серво лишилось при вимкненні). Приводимо її у відомий закритий стан,
     * інакше корм може сипатись одразу після старту. */
    servo_set_angle(SERVO_CLOSED_DEG);
    vTaskDelay(pdMS_TO_TICKS(SERVO_MOVE_MS));
    servo_detach();
}

void servo_set_angle(int angle_deg)
{
    if (angle_deg < 0) angle_deg = 0;
    if (angle_deg > 180) angle_deg = 180;

    int pulse_us = SERVO_MIN_US + (SERVO_MAX_US - SERVO_MIN_US) * angle_deg / 180;
    int max_duty = (1 << SERVO_RES_BITS) - 1;
    int duty = (int)((int64_t)pulse_us * max_duty / SERVO_PERIOD_US);

    ESP_ERROR_CHECK(ledc_set_duty(SERVO_MODE, SERVO_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(SERVO_MODE, SERVO_CHANNEL));
}

void servo_detach(void)
{
    ESP_ERROR_CHECK(ledc_set_duty(SERVO_MODE, SERVO_CHANNEL, 0));
    ESP_ERROR_CHECK(ledc_update_duty(SERVO_MODE, SERVO_CHANNEL));
}
