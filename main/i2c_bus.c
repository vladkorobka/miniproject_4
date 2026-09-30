#include "i2c_bus.h"
#include "esp_err.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "i2c_bus";

static i2c_master_bus_handle_t bus = NULL;

void i2c_bus_init(void)
{
    i2c_master_bus_config_t conf = {
        .i2c_port = -1,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&conf, &bus));
}

i2c_master_bus_handle_t i2c_bus_get(void)
{
    return bus;
}

void i2c_bus_scan(void)
{
    int found = 0;
    for (uint16_t addr = 0x08; addr < 0x78; addr++) {
        if (i2c_master_probe(bus, addr, I2C_TIMEOUT_MS) == ESP_OK) {
            ESP_LOGI(TAG, "знайдено пристрій 0x%02X", addr);
            found++;
        }
    }
    ESP_LOGI(TAG, "скан завершено: %d пристроїв (очікувано 0x3C, 0x50, 0x68)", found);
}
