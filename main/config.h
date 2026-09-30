#pragma once

#include "driver/gpio.h"
#include "feeder_types.h"

#define ENCODER_CLK_PIN     GPIO_NUM_4
#define ENCODER_DT_PIN      GPIO_NUM_5
#define ENCODER_SW_PIN      GPIO_NUM_6
#define ENCODER_STEPS_PER_DETENT   4

#define BTN_MANUAL_PIN      GPIO_NUM_7
#define BTN_BACK_PIN        GPIO_NUM_8

#define LED_PORTION_1_PIN   GPIO_NUM_9
#define LED_PORTION_2_PIN   GPIO_NUM_10
#define LED_PORTION_3_PIN   GPIO_NUM_11
#define LED_PORTION_4_PIN   GPIO_NUM_12

#define LED_AUTO_PIN        GPIO_NUM_13
#define LED_FEEDING_PIN     GPIO_NUM_14

#define BUZZER_PIN          GPIO_NUM_15

#define SERVO_PIN           GPIO_NUM_16

#define MOTOR_PIN           GPIO_NUM_17

#define I2C_SDA_PIN         GPIO_NUM_18
#define I2C_SCL_PIN         GPIO_NUM_3
#define I2C_FREQ_HZ         100000
#define I2C_TIMEOUT_MS      50

#define RTC_ADDR            0x68
#define OLED_ADDR           0x3C

#define RTC_BUILD_OFFSET_SEC    30

#define DISPLAY_PERIOD_MS       200
#define DISPLAY_RTC_PERIOD_MS   1000

#define MOTOR_MS_PER_PORTION    1500
#define GATE_LINGER_MS          3500
#define SERVO_MOVE_MS           300
#define SERVO_OPEN_DEG          90
#define SERVO_CLOSED_DEG        0

#define AUTO_INTERVAL_SEC       30
