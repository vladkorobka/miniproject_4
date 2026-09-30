#include "oled.h"

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_log.h"

#include "config.h"
#include "i2c_bus.h"

static const char *TAG = "oled";

static esp_lcd_panel_handle_t panel = NULL;

bool oled_init(void)
{
    if (i2c_master_probe(i2c_bus_get(), OLED_ADDR, I2C_TIMEOUT_MS) != ESP_OK) {
        ESP_LOGW(TAG, "SSD1306 не відповідає на 0x%02X - екран вимкнено", OLED_ADDR);
        return false;
    }

    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_i2c_config_t io_conf = {
        .dev_addr = OLED_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    esp_lcd_panel_ssd1306_config_t ssd_conf = {
        .height = FB_HEIGHT,
    };
    esp_lcd_panel_dev_config_t panel_conf = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,
        .vendor_config = &ssd_conf,
    };

    if (esp_lcd_new_panel_io_i2c(i2c_bus_get(), &io_conf, &io) != ESP_OK ||
        esp_lcd_new_panel_ssd1306(io, &panel_conf, &panel) != ESP_OK ||
        esp_lcd_panel_reset(panel) != ESP_OK ||
        esp_lcd_panel_init(panel) != ESP_OK ||
        esp_lcd_panel_disp_on_off(panel, true) != ESP_OK) {
        ESP_LOGW(TAG, "ініціалізація SSD1306 не вдалася - екран вимкнено");
        return false;
    }
    return true;
}

bool oled_flush(const fb_t *fb)
{
    return esp_lcd_panel_draw_bitmap(panel, 0, 0, FB_WIDTH, FB_HEIGHT, fb->buf) == ESP_OK;
}
