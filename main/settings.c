#include "settings.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"

#include "config.h"

static const char *TAG = "settings";

#define NVS_NAMESPACE   "feeder"
#define KEY_PORTION     "portion"
#define KEY_AUTO        "auto_on"

static nvs_handle_t handle;
static bool nvs_ready = false;

/* Кеш у RAM: читаємо NVS один раз на старті, далі працюємо з кешем
 * і пишемо тільки дельту. */
static uint8_t cached_portion = PORTION_MIN;
static bool    cached_auto    = false;

void settings_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* Розділ пошкоджений або від старішої версії - стираємо і пробуємо ще раз. */
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        /* Не фатально: годівничка працюватиме, просто без запам'ятовування. */
        ESP_LOGW(TAG, "nvs_open failed (%s), працюємо на значеннях за замовчуванням",
                 esp_err_to_name(err));
        return;
    }
    nvs_ready = true;

    uint8_t value;
    if (nvs_get_u8(handle, KEY_PORTION, &value) == ESP_OK &&
        value >= PORTION_MIN && value <= PORTION_MAX) {
        cached_portion = value;
    }
    if (nvs_get_u8(handle, KEY_AUTO, &value) == ESP_OK) {
        cached_auto = (value != 0);
    }

    ESP_LOGI(TAG, "відновлено: порція=%u, auto=%d", cached_portion, (int)cached_auto);
}

uint8_t settings_get_portion(void)
{
    return cached_portion;
}

bool settings_get_auto_enabled(void)
{
    return cached_auto;
}

static void store_u8(const char *key, uint8_t value)
{
    if (!nvs_ready) {
        return;
    }
    esp_err_t err = nvs_set_u8(handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "запис %s не вдався: %s", key, esp_err_to_name(err));
    }
}

void settings_set_portion(uint8_t portion)
{
    if (portion == cached_portion) {
        return;
    }
    cached_portion = portion;
    store_u8(KEY_PORTION, portion);
}

void settings_set_auto_enabled(bool enabled)
{
    if (enabled == cached_auto) {
        return;
    }
    cached_auto = enabled;
    store_u8(KEY_AUTO, enabled ? 1 : 0);
}
