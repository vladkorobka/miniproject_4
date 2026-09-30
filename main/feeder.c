#include "feeder.h"

#include <stdbool.h>

#include "freertos/task.h"
#include "esp_timer.h"

#include "config.h"
#include "feeder_types.h"
#include "leds.h"
#include "buzzer.h"
#include "melody.h"
#include "servo.h"
#include "motor.h"
#include "buttons.h"
#include "settings.h"
#include "ui_text.h"
#include "menu.h"
#include "ui_state.h"

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "feeder";

static esp_timer_handle_t auto_timer = NULL;
static int64_t auto_interval_us = 0;

static int64_t next_deadline_us = 0;
static portMUX_TYPE deadline_lock = portMUX_INITIALIZER_UNLOCKED;

static void auto_timer_callback(void *arg)
{
    taskENTER_CRITICAL(&deadline_lock);
    if (next_deadline_us != 0) {
        next_deadline_us += auto_interval_us;
    }
    taskEXIT_CRITICAL(&deadline_lock);

    feeder_event_t evt = { .type = EVT_AUTO_TIMER, .value = 0 };
    xQueueSend(event_queue, &evt, 0);
}

void auto_timer_start(uint32_t interval_sec)
{
    if (auto_timer == NULL) {
        esp_timer_create_args_t args = {
            .callback = &auto_timer_callback,
            .name = "auto_feed_timer",
        };
        ESP_ERROR_CHECK(esp_timer_create(&args, &auto_timer));
    }
    if (esp_timer_is_active(auto_timer)) {
        return;
    }

    auto_interval_us = (int64_t)interval_sec * 1000000;
    taskENTER_CRITICAL(&deadline_lock);
    next_deadline_us = esp_timer_get_time() + auto_interval_us;
    taskEXIT_CRITICAL(&deadline_lock);
    ESP_ERROR_CHECK(esp_timer_start_periodic(auto_timer, (uint64_t)auto_interval_us));
}

void auto_timer_stop(void)
{
    if (auto_timer == NULL || !esp_timer_is_active(auto_timer)) {
        return;
    }
    ESP_ERROR_CHECK(esp_timer_stop(auto_timer));
    taskENTER_CRITICAL(&deadline_lock);
    next_deadline_us = 0;
    taskEXIT_CRITICAL(&deadline_lock);
}

bool auto_timer_remaining_sec(int32_t *sec)
{
    taskENTER_CRITICAL(&deadline_lock);
    int64_t deadline = next_deadline_us;
    taskEXIT_CRITICAL(&deadline_lock);

    if (deadline == 0) {
        return false;
    }
    *sec = ui_countdown_sec(deadline, esp_timer_get_time());
    return true;
}

void feed_cycle(uint8_t portion_size)
{
    led_set(LED_FEEDING, true);
    ESP_LOGI(TAG, "цикл насипання: порція=%u, мотор=%d мс",
             portion_size, portion_size * MOTOR_MS_PER_PORTION);

    buzzer_play(feed_melody, feed_melody_len);
    buzzer_sweep(MEOW_HZ_START, MEOW_HZ_PEAK, MEOW_UP_MS);
    buzzer_sweep(MEOW_HZ_PEAK, MEOW_HZ_END, MEOW_DOWN_MS);

    servo_set_angle(SERVO_OPEN_DEG);
    vTaskDelay(pdMS_TO_TICKS(SERVO_MOVE_MS));

    motor_start();
    vTaskDelay(pdMS_TO_TICKS(portion_size * MOTOR_MS_PER_PORTION));
    motor_stop();

    vTaskDelay(pdMS_TO_TICKS(GATE_LINGER_MS));

    servo_set_angle(SERVO_CLOSED_DEG);
    vTaskDelay(pdMS_TO_TICKS(SERVO_MOVE_MS));
    servo_detach();

    led_set(LED_FEEDING, false);
}

static void send_event(event_type_t type)
{
    feeder_event_t evt = { .type = type, .value = 0 };
    xQueueSend(event_queue, &evt, 0);
}

void input_task(void *arg)
{
    while (1) {
        if (button_manual_pressed()) {
            send_event(EVT_BTN_MANUAL);
        }
        if (button_enc_click_pressed()) {
            send_event(EVT_ENC_CLICK);
        }
        if (button_back_pressed()) {
            send_event(EVT_BTN_BACK);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void run_feed(ui_state_t *st)
{
    st->feeding = true;
    ui_state_publish(st);

    update_portion_leds(st->portion);
    feed_cycle(st->portion);

    st->feeding = false;
    update_portion_leds(st->screen == SCREEN_MENU_PORTION ? st->preview : st->portion);
    xQueueReset(event_queue);
}

void feeder_task(void *arg)
{
    feeder_event_t evt;
    ui_state_t st;

    ui_state_get(&st);

    update_portion_leds(st.portion);
    led_set(LED_AUTO, st.auto_enabled);
    if (st.auto_enabled) {
        auto_timer_start(AUTO_INTERVAL_SEC);
    }

    while (1) {
        xQueueReceive(event_queue, &evt, portMAX_DELAY);

        menu_action_t action = menu_handle(&st, evt.type, evt.value);

        switch (action.type) {
        case ACT_NONE:
            break;

        case ACT_SHOW_LEDS:
            update_portion_leds(action.arg);
            break;

        case ACT_COMMIT_PORTION:
            update_portion_leds(action.arg);
            settings_set_portion(action.arg);
            ESP_LOGI(TAG, "порція підтверджена: %u", action.arg);
            break;

        case ACT_SET_AUTO:
            led_set(LED_AUTO, action.arg != 0);
            settings_set_auto_enabled(action.arg != 0);
            if (action.arg) {
                auto_timer_start(AUTO_INTERVAL_SEC);
            } else {
                auto_timer_stop();
            }
            ESP_LOGI(TAG, "AUTO=%u", action.arg);
            break;

        case ACT_FEED:
            ESP_LOGI(TAG, "насипання (%s)", evt.type == EVT_AUTO_TIMER ? "таймер" : "кнопка");
            run_feed(&st);
            break;
        }

        ui_state_publish(&st);
    }
}
