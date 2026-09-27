#include "motor.h"

#include "driver/gpio.h"
#include "esp_err.h"

#include "config.h"

void motor_init(void)
{
    gpio_config_t conf = {
        .pin_bit_mask = 1ULL << MOTOR_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&conf));
    gpio_set_level(MOTOR_PIN, 0);
}

void motor_start(void)
{
    gpio_set_level(MOTOR_PIN, 1);
}

void motor_stop(void)
{
    gpio_set_level(MOTOR_PIN, 0);
}
