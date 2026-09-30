#include "buttons.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"

static bool manual_last_state = false;
static bool back_last_state = false;
static bool enc_click_last_state = false;

static bool debounce_check(gpio_num_t pin, bool *last_state)
{
    bool level = (gpio_get_level(pin) == 0);

    if (level == *last_state) {
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(20));
    level = (gpio_get_level(pin) == 0);

    if (level == *last_state) {
        return false;
    }

    *last_state = level;

    return level;
}

void buttons_init(void)
{
    gpio_config_t conf = {
        .pin_bit_mask = (1ULL << BTN_MANUAL_PIN) | (1ULL << BTN_BACK_PIN) |
                        (1ULL << ENCODER_SW_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&conf));
}

bool button_manual_pressed(void)
{
    return debounce_check(BTN_MANUAL_PIN, &manual_last_state);
}

bool button_back_pressed(void)
{
    return debounce_check(BTN_BACK_PIN, &back_last_state);
}

bool button_enc_click_pressed(void)
{
    return debounce_check(ENCODER_SW_PIN, &enc_click_last_state);
}
