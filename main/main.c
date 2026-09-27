#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "feeder_types.h"
#include "feeder.h"
#include "encoder.h"
#include "buttons.h"
#include "leds.h"
#include "buzzer.h"
#include "servo.h"
#include "motor.h"
#include "settings.h"
#include "i2c_bus.h"
#include "rtc_ds1307.h"
#include "menu.h"
#include "ui_state.h"
#include "display.h"

#include "esp_err.h"

/* Визначення спільної черги подій (оголошена як extern у feeder.h). */
QueueHandle_t event_queue;

void app_main(void)
{
    event_queue = xQueueCreate(10, sizeof(feeder_event_t));
    ESP_ERROR_CHECK(event_queue != NULL ? ESP_OK : ESP_ERR_NO_MEM);

    /* NVS - першим: з нього відновлюється стан і мітка збірки для RTC. */
    settings_init();

    leds_init();
    buttons_init();
    encoder_init();
    buzzer_init();
    servo_init();
    motor_init();

    /* I2C: скан у лог для діагностики, потім алгоритм старту RTC.
     * Після app_main шиною користується лише display_task. */
    i2c_bus_init();
    i2c_bus_scan();
    ds1307_rtc_init();

    /* Початковий стан меню з NVS - до старту задач, що його читають. */
    ui_state_t initial;
    menu_init(&initial, settings_get_portion(), settings_get_auto_enabled());
    ui_state_init(&initial);

    xTaskCreate(input_task, "input_task", 4096, NULL, 5, NULL);
    xTaskCreate(feeder_task, "feeder_task", 4096, NULL, 5, NULL);
    xTaskCreate(display_task, "display_task", 4096, NULL, 4, NULL);
}
