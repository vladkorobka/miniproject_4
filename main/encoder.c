#include "encoder.h"

#include <stdint.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"

#include "config.h"
#include "feeder_types.h"
#include "feeder.h"

/* Квадратурний декодер на таблиці переходів.
 *
 * Стан = (CLK << 1) | DT, індекс = (попередній стан << 2) | новий стан.
 * Валідний перехід дає +1 або -1 залежно від напрямку; неможливий
 * (змінились обидва біти одразу - наслідок дребезгу чи пропущеного
 * переривання) дає 0 і просто ігнорується.
 *
 * Головна перевага перед читанням DT на фронті CLK: дребезг контакту
 * породжує симетричні переходи туди-назад, які в сумі дають нуль, тому
 * окремий антидребезг за часом не потрібен взагалі. Крок назовні
 * віддається лише після повного детенту. */
static const int8_t quad_table[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0,
};

static volatile uint8_t prev_state = 0;
static volatile int8_t  accum = 0;

static inline uint8_t IRAM_ATTR encoder_read_state(void)
{
    return (uint8_t)((gpio_get_level(ENCODER_CLK_PIN) << 1) |
                      gpio_get_level(ENCODER_DT_PIN));
}

static void IRAM_ATTR encoder_isr_handler(void *arg)
{
    uint8_t state = encoder_read_state();
    if (state == prev_state) {
        return; /* переривання від фронту, який уже враховано */
    }

    accum += quad_table[(prev_state << 2) | state];
    prev_state = state;

    int8_t delta;
    if (accum >= ENCODER_STEPS_PER_DETENT) {
        delta = 1;
    } else if (accum <= -ENCODER_STEPS_PER_DETENT) {
        delta = -1;
    } else {
        return; /* детент ще не завершено */
    }
    accum = 0;

    feeder_event_t evt = { .type = EVT_ENCODER_DELTA, .value = delta };

    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(event_queue, &evt, &woken);
    if (woken) {
        portYIELD_FROM_ISR();
    }
}

void encoder_init(void)
{
    /* Обидві лінії - входи з переривання на будь-якому фронті: декодеру
     * потрібні всі чотири переходи циклу, а не лише спад CLK. */
    gpio_config_t conf = {
        .pin_bit_mask = (1ULL << ENCODER_CLK_PIN) | (1ULL << ENCODER_DT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&conf));

    /* Стартуємо з фактичного положення, інакше перший же рух дав би
     * фальшивий перехід із стану 00. */
    prev_state = encoder_read_state();
    accum = 0;

    /* Єдиний виклик install_isr_service на весь проєкт - тут, оскільки
     * енкодер поки єдине джерело переривань GPIO. ESP_ERR_INVALID_STATE
     * означає "вже встановлено" і не є помилкою. */
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(ENCODER_CLK_PIN, encoder_isr_handler, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(ENCODER_DT_PIN,  encoder_isr_handler, NULL));
}
