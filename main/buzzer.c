#include "buzzer.h"

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

#include "config.h"

#define BUZZER_TIMER     LEDC_TIMER_1
#define BUZZER_CHANNEL   LEDC_CHANNEL_1
#define BUZZER_MODE      LEDC_LOW_SPEED_MODE
#define BUZZER_RES       LEDC_TIMER_10_BIT
#define BUZZER_RES_BITS  10

void buzzer_init(void)
{
    ledc_timer_config_t timer_conf = {
        .speed_mode      = BUZZER_MODE,
        .timer_num       = BUZZER_TIMER,
        .duty_resolution = BUZZER_RES,
        .freq_hz         = NOTE_C6,
        .clk_cfg         = LEDC_USE_APB_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t channel_conf = {
        .gpio_num   = BUZZER_PIN,
        .speed_mode = BUZZER_MODE,
        .channel    = BUZZER_CHANNEL,
        .timer_sel  = BUZZER_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));
}

static void tone_on(void)
{
    int half_duty = (1 << BUZZER_RES_BITS) / 2;
    ESP_ERROR_CHECK(ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, half_duty));
    ESP_ERROR_CHECK(ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL));
}

static void tone_off(void)
{
    ESP_ERROR_CHECK(ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, 0));
    ESP_ERROR_CHECK(ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL));
}

void buzzer_sweep(int freq_start, int freq_end, int duration_ms)
{
    const int step_ms = 10;

    int steps = duration_ms / step_ms;
    if (steps < 1) {
        steps = 1;
    }

    ESP_ERROR_CHECK(ledc_set_freq(BUZZER_MODE, BUZZER_TIMER, freq_start));
    tone_on();

    for (int i = 0; i <= steps; i++) {
        int freq = freq_start + (freq_end - freq_start) * i / steps;
        ESP_ERROR_CHECK(ledc_set_freq(BUZZER_MODE, BUZZER_TIMER, freq));
        vTaskDelay(pdMS_TO_TICKS(step_ms));
    }

    tone_off();
}

void buzzer_play(const note_t *notes, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        if (notes[i].hz != REST) {
            ESP_ERROR_CHECK(ledc_set_freq(BUZZER_MODE, BUZZER_TIMER, notes[i].hz));
            tone_on();
        }
        vTaskDelay(pdMS_TO_TICKS(notes[i].ms));
        tone_off();
        vTaskDelay(pdMS_TO_TICKS(NOTE_GAP_MS));
    }
}
