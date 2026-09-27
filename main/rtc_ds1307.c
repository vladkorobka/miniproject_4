#include "rtc_ds1307.h"

#include <string.h>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"

#include "build_time.h"
#include "config.h"
#include "ds1307_codec.h"
#include "i2c_bus.h"
#include "settings.h"

static const char *TAG = "rtc";

static i2c_master_dev_handle_t dev = NULL;
static bool time_valid = false;

static bool read_regs(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_transmit_receive(dev, &reg, 1, buf, len, I2C_TIMEOUT_MS) == ESP_OK;
}

static bool write_regs(uint8_t reg, const uint8_t *data, size_t len)
{
    uint8_t buf[1 + DS1307_TIME_REG_COUNT];   /* найбільший запис - регістри часу */
    if (len > DS1307_TIME_REG_COUNT) {
        return false;
    }
    buf[0] = reg;
    memcpy(buf + 1, data, len);
    return i2c_master_transmit(dev, buf, len + 1, I2C_TIMEOUT_MS) == ESP_OK;
}

static void log_time(const char *what, const rtc_datetime_t *t)
{
    ESP_LOGI(TAG, "%s: %04u-%02u-%02u %02u:%02u:%02u", what,
             t->year, t->month, t->day, t->hour, t->minute, t->second);
}

static bool set_from_build(void)
{
    rtc_datetime_t t = {
        .year = BUILD_YEAR, .month = BUILD_MONTH, .day = BUILD_DAY,
        .hour = BUILD_HOUR, .minute = BUILD_MIN, .second = BUILD_SEC,
    };
    rtc_datetime_add_seconds(&t, RTC_BUILD_OFFSET_SEC);

    uint8_t regs[DS1307_TIME_REG_COUNT];
    ds1307_encode(&t, regs);   /* заодно CH = 0 і 24h */

    if (!write_regs(0x00, regs, sizeof regs) ||
        !write_regs(DS1307_RAM_MARKER_REG, ds1307_marker, DS1307_MARKER_LEN)) {
        return false;
    }
    log_time("час встановлено зі збірки", &t);
    return true;
}

void ds1307_rtc_init(void)
{
    i2c_device_config_t conf = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = RTC_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_get(), &conf, &dev));

    bool present = i2c_master_probe(i2c_bus_get(), RTC_ADDR, I2C_TIMEOUT_MS) == ESP_OK;

    char stamp[24];
    bool new_firmware = !settings_get_build_stamp(stamp, sizeof stamp) ||
                        strcmp(stamp, BUILD_STAMP) != 0;

    uint8_t regs[DS1307_TIME_REG_COUNT];
    uint8_t ram[DS1307_MARKER_LEN];
    rtc_datetime_t now;
    bool regs_ok = present && read_regs(0x00, regs, sizeof regs) && ds1307_decode(regs, &now);
    bool marker_ok = present && read_regs(DS1307_RAM_MARKER_REG, ram, sizeof ram) &&
                     ds1307_marker_matches(ram);

    switch (rtc_startup_decide(present, new_firmware, marker_ok, regs_ok)) {
    case RTC_START_ABSENT:
        ESP_LOGW(TAG, "DS1307 не відповідає на 0x%02X - час показуватиметься як --:--", RTC_ADDR);
        time_valid = false;
        break;

    case RTC_START_SET_FROM_BUILD:
        time_valid = set_from_build();
        /* Мітку зберігаємо лише після успішного запису: інакше RTC,
         * підключений пізніше, так і не отримав би час. */
        if (time_valid) {
            settings_set_build_stamp(BUILD_STAMP);
        } else {
            ESP_LOGW(TAG, "не вдалося записати час у DS1307");
        }
        break;

    case RTC_START_INVALID:
        ESP_LOGW(TAG, "живлення RTC пропадало - час невідомий до наступної прошивки");
        time_valid = false;
        break;

    case RTC_START_VALID:
        time_valid = true;
        log_time("час RTC", &now);
        break;
    }
}

bool ds1307_rtc_read(rtc_datetime_t *out)
{
    if (!time_valid) {
        return false;
    }
    uint8_t regs[DS1307_TIME_REG_COUNT];
    return read_regs(0x00, regs, sizeof regs) && ds1307_decode(regs, out);
}
