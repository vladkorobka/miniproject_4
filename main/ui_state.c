#include "ui_state.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_err.h"

static SemaphoreHandle_t lock = NULL;
static ui_state_t current;

void ui_state_init(const ui_state_t *initial)
{
    lock = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(lock != NULL ? ESP_OK : ESP_ERR_NO_MEM);
    current = *initial;
}

void ui_state_publish(const ui_state_t *st)
{
    xSemaphoreTake(lock, portMAX_DELAY);
    current = *st;
    xSemaphoreGive(lock);
}

void ui_state_get(ui_state_t *out)
{
    xSemaphoreTake(lock, portMAX_DELAY);
    *out = current;
    xSemaphoreGive(lock);
}
