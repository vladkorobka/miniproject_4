#pragma once

#include "driver/gpio.h"
#include "feeder_types.h"   /* PORTION_MIN / PORTION_MAX */

/* ---------- Енкодер KY-040 ---------- */
#define ENCODER_CLK_PIN     GPIO_NUM_4
#define ENCODER_DT_PIN      GPIO_NUM_5
#define ENCODER_SW_PIN      GPIO_NUM_6   /* зарезервовано, поки не використовується */

/* Скільки квадратурних переходів припадає на один клік (детент).
 * У KY-040 це повний цикл = 4. Якщо один клік дає два кроки порції -
 * поставте 2, якщо навпаки треба два кліки на крок - 8. */
#define ENCODER_STEPS_PER_DETENT   4

/* ---------- Кнопки ---------- */
#define BTN_MANUAL_PIN      GPIO_NUM_7
#define BTN_AUTO_PIN        GPIO_NUM_8

/* ---------- LED порції (шкала-градусник, 1 клік енкодера = 1 LED) ---------- */
#define LED_PORTION_1_PIN   GPIO_NUM_9
#define LED_PORTION_2_PIN   GPIO_NUM_10
#define LED_PORTION_3_PIN   GPIO_NUM_11
#define LED_PORTION_4_PIN   GPIO_NUM_12

/* ---------- LED режимів ---------- */
#define LED_AUTO_PIN        GPIO_NUM_13  /* червоний: AUTO on/off */
#define LED_FEEDING_PIN     GPIO_NUM_14  /* зелений: іде насипання */

/* ---------- Зумер (пасивний, PWM через LEDC) ---------- */
#define BUZZER_PIN          GPIO_NUM_15

/* ---------- Сервопривід SG90 (PWM через LEDC, 50 Гц) ---------- */
#define SERVO_PIN           GPIO_NUM_16

/* ---------- DC-мотор (через транзистор, простий on/off) ---------- */
#define MOTOR_PIN           GPIO_NUM_17

/* ---------- Шина I2C: RTC DS1307 + OLED SSD1306 ----------
 * GPIO46 не використовується: strapping-пін режиму завантаження,
 * підтяжка до 1 заважає входу в режим прошивки. */
#define I2C_SDA_PIN         GPIO_NUM_18
#define I2C_SCL_PIN         GPIO_NUM_3
#define I2C_FREQ_HZ         100000
#define I2C_TIMEOUT_MS      50

#define RTC_ADDR            0x68
#define OLED_ADDR           0x3C

/* Час між збіркою і стартом плати (прошивка + завантаження), додається
 * до часу збірки при встановленні RTC. */
#define RTC_BUILD_OFFSET_SEC    30

/* ---------- Екран ---------- */
#define DISPLAY_PERIOD_MS       200    /* період перемальовування */
#define DISPLAY_RTC_PERIOD_MS   1000   /* як часто читати RTC */

/* ---------- Часові константи циклу насипання ---------- */
#define MOTOR_MS_PER_PORTION    1500   /* калібрується емпірично під диск */
#define GATE_LINGER_MS          3500   /* час, щоб корм висипався з приймача */
#define SERVO_MOVE_MS           300    /* час фізичного руху серво */
#define SERVO_OPEN_DEG          90
#define SERVO_CLOSED_DEG        0

/* ---------- Зумер: параметри sweep-сигналу ---------- */
#define BUZZ_FREQ_START         200
#define BUZZ_FREQ_END           800
#define BUZZ_SWEEP_MS           500

/* ---------- Авто-режим ---------- */
#define AUTO_INTERVAL_SEC 10
