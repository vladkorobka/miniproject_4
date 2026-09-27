#include "feeder.h"

#include <stdbool.h>

#include "freertos/task.h"
#include "esp_timer.h"

#include "config.h"
#include "feeder_types.h"
#include "leds.h"
#include "buzzer.h"
#include "servo.h"
#include "motor.h"
#include "buttons.h"
#include "settings.h"

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "feeder";

static esp_timer_handle_t auto_timer = NULL;

static void auto_timer_callback(void *arg)
{
    feeder_event_t evt = { .type = EVT_AUTO_TIMER, .value = 0 };
    xQueueSend(event_queue, &evt, 0);
}

void auto_timer_start(uint32_t interval_sec)
{
    if (auto_timer != NULL) {
        return; /* вже запущений */
    }

    esp_timer_create_args_t args = {
        .callback = &auto_timer_callback,
        .name = "auto_feed_timer",
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &auto_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(auto_timer,
                                             (uint64_t)interval_sec * 1000000ULL));
}

void auto_timer_stop(void)
{
    if (auto_timer == NULL) {
        return;
    }
    ESP_ERROR_CHECK(esp_timer_stop(auto_timer));
    ESP_ERROR_CHECK(esp_timer_delete(auto_timer));
    auto_timer = NULL;
}

void feed_cycle(uint8_t portion_size)
{
    led_set(LED_FEEDING, true);
    ESP_LOGI(TAG, "цикл насипання: порція=%u, мотор=%d мс",
             portion_size, portion_size * MOTOR_MS_PER_PORTION);

    /* 1. Попереджувальний звуковий сигнал */
    buzzer_sweep(BUZZ_FREQ_START, BUZZ_FREQ_END, BUZZ_SWEEP_MS);

    /* 2. Відкрити заслінку приймача */
    servo_set_angle(SERVO_OPEN_DEG);
    vTaskDelay(pdMS_TO_TICKS(SERVO_MOVE_MS));

    /* 3. Крутити диск/мотор пропорційно до порції */
    motor_start();
    vTaskDelay(pdMS_TO_TICKS(portion_size * MOTOR_MS_PER_PORTION));
    motor_stop();

    /* 4. Дати корму часу висипатись з приймача перед закриттям */
    vTaskDelay(pdMS_TO_TICKS(GATE_LINGER_MS));

    /* 5. Закрити заслінку і зняти навантаження з серво */
    servo_set_angle(SERVO_CLOSED_DEG);
    vTaskDelay(pdMS_TO_TICKS(SERVO_MOVE_MS));
    servo_detach();

    led_set(LED_FEEDING, false);
}

void input_task(void *arg)
{
    while (1) {
        if (button_manual_pressed()) {
            feeder_event_t evt = { .type = EVT_BTN_MANUAL, .value = 0 };
            xQueueSend(event_queue, &evt, 0);
        }

        if (button_auto_pressed()) {
            feeder_event_t evt = { .type = EVT_BTN_AUTO_TOGGLE, .value = 0 };
            xQueueSend(event_queue, &evt, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void feeder_task(void *arg)
{
    feeder_event_t evt;

    /* Стан відновлюється з NVS, щоб зникнення живлення не скидало
     * налаштовану порцію і не вимикало AUTO мовчки. */
    uint8_t portion_size = settings_get_portion();
    bool auto_enabled = settings_get_auto_enabled();

    update_portion_leds(portion_size);
    led_set(LED_AUTO, auto_enabled);
    if (auto_enabled) {
        auto_timer_start(AUTO_INTERVAL_SEC);
    }

    while (1) {
        xQueueReceive(event_queue, &evt, portMAX_DELAY);

        switch (evt.type) {
        case EVT_ENCODER_DELTA:
            /* Черга чиститься після кожного feed_cycle(), тож сюди
             * потрапляють лише оберти, зроблені в режимі очікування. */
            portion_size = clamp_u8((int)portion_size + evt.value, PORTION_MIN, PORTION_MAX);
            update_portion_leds(portion_size);
            settings_set_portion(portion_size);
            ESP_LOGI(TAG, "енкодер: delta=%d -> порція=%u", (int)evt.value, portion_size);
            break;

        case EVT_BTN_MANUAL:
            ESP_LOGI(TAG, "кнопка 1 (ручне насипання)");
            feed_cycle(portion_size);
            xQueueReset(event_queue); /* відкинути все, що накопичилось під час циклу */
            break;

        case EVT_BTN_AUTO_TOGGLE:
            auto_enabled = !auto_enabled;
            ESP_LOGI(TAG, "кнопка 2: AUTO=%d", (int)auto_enabled);
            led_set(LED_AUTO, auto_enabled);
            settings_set_auto_enabled(auto_enabled);
            if (auto_enabled) {
                auto_timer_start(AUTO_INTERVAL_SEC);
            } else {
                auto_timer_stop();
            }
            break;

        case EVT_AUTO_TIMER:
            /* Подія могла бути покладена в чергу за мить до вимкнення AUTO -
             * тоді її треба проігнорувати, а не годувати всупереч режиму. */
            if (!auto_enabled) {
                break;
            }
            feed_cycle(portion_size);
            xQueueReset(event_queue);
            break;

        default:
            break;
        }
    }
}
