#include "display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h"
#include "feeder.h"
#include "oled.h"
#include "rtc_ds1307.h"
#include "ui.h"
#include "ui_state.h"

void display_task(void *arg)
{
    static fb_t fb;
    ui_view_t view = { 0 };
    TickType_t last_rtc_read = 0;
    bool rtc_read_once = false;

    if (!oled_init()) {
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        ui_state_get(&view.st);

        TickType_t now = xTaskGetTickCount();
        if (!rtc_read_once || now - last_rtc_read >= pdMS_TO_TICKS(DISPLAY_RTC_PERIOD_MS)) {
            view.time_valid = ds1307_rtc_read(&view.time);
            last_rtc_read = now;
            rtc_read_once = true;
        }

        if (!auto_timer_remaining_sec(&view.countdown_sec)) {
            view.countdown_sec = -1;
        }

        ui_render(&fb, &view);
        oled_flush(&fb);

        vTaskDelay(pdMS_TO_TICKS(DISPLAY_PERIOD_MS));
    }
}
