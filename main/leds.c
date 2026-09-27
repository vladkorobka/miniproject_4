#include "leds.h"

#include "driver/gpio.h"
#include "esp_err.h"

#include "config.h"

static const gpio_num_t portion_pins[4] = {
    LED_PORTION_1_PIN,
    LED_PORTION_2_PIN,
    LED_PORTION_3_PIN,
    LED_PORTION_4_PIN,
};

void leds_init(void)
{
    uint64_t mask = 0;
    for (int i = 0; i < 4; i++) {
        mask |= (1ULL << portion_pins[i]);
    }
    mask |= (1ULL << LED_AUTO_PIN);
    mask |= (1ULL << LED_FEEDING_PIN);

    gpio_config_t conf = {
        .pin_bit_mask = mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&conf));

    update_portion_leds(0);
    led_set(LED_AUTO, false);
    led_set(LED_FEEDING, false);
}

void update_portion_leds(uint8_t portion)
{
    for (int i = 0; i < 4; i++) {
        gpio_set_level(portion_pins[i], (i < portion) ? 1 : 0);
    }
}

void led_set(led_id_t id, bool on)
{
    gpio_num_t pin = (id == LED_AUTO) ? LED_AUTO_PIN : LED_FEEDING_PIN;
    gpio_set_level(pin, on ? 1 : 0);
}
