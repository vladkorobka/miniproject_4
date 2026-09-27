# miniproject_4: RTC DS1307 + OLED SSD1306 + меню — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Перенести годівничку з miniproject_3 у miniproject_4 і додати RTC DS1307 (час зі збірки), OLED SSD1306 128×64 з рядком стану та меню Portion / Auto Mode, кероване енкодером і кнопкою «Назад».

**Architecture:** Три задачі FreeRTOS: `input_task` (кнопки → черга), `feeder_task` (єдиний власник стану, виконує дії чистого модуля `menu`), `display_task` (єдиний користувач I2C після старту: читає RTC, рахує відлік, малює кадр). Уся логіка без заліза (`rtc_time`, `ds1307_codec`, `menu`, `fb`, `ui_text`, `ui`) — окремі файли без ESP-IDF заголовків, покриті хостовими Unity-тестами (gcc з msys64).

**Tech Stack:** ESP-IDF v6.0.2 (esp32s3), `esp_driver_i2c` (`i2c_master`), `esp_lcd` (вбудований SSD1306), FreeRTOS, NVS; Unity з ESP-IDF для хостових тестів; шрифт font8x8_basic (public domain).

**Spec:** `docs/superpowers/specs/2026-09-27-rtc-oled-menu-design.md`

**Відхилення від спеку (свідомі, дрібні):**
- Відлік рахується від власного дедлайну `next_deadline_us`, а не `esp_timer_get_expiry_time()` — у v6.0.2 вона повертає `ESP_ERR_NOT_SUPPORTED` для періодичних таймерів.
- Шрифт «8×16» реалізовано як public-domain font8x8, подвоєний по вертикалі (клітинка 8×16, 16×4 символи — як у спеку).

## Global Constraints

- ESP-IDF **v6.0.2**, target **esp32s3**, flash 16 MB через `sdkconfig.defaults` (вже в репо).
- Жодних сторонніх компонентів: лише ESP-IDF + файл `main/font8x8_basic.h` (public domain).
- I2C лише через новий драйвер `driver/i2c_master.h`; legacy `driver/i2c.h` не використовувати.
- Піни: SDA = GPIO18, SCL = GPIO3, енкодер CLK/DT/SW = 4/5/6, ручна кнопка = 7, «Назад» = 8, LED порції = 9–12, LED AUTO = 13, LED FEEDING = 14, зумер = 15, серво = 16, мотор = 17.
- Адреси: OLED `0x3C`, DS1307 `0x68`. Шина 100 кГц.
- `AUTO_INTERVAL_SEC` = 30, `RTC_BUILD_OFFSET_SEC` = 30, порція 1–4.
- Після `app_main` шиною I2C користується **лише** `display_task`.
- Чисті модулі (`rtc_time`, `ds1307_codec`, `menu`, `fb`, `ui_text`, `ui`) не включають `config.h`, FreeRTOS чи інші ESP-IDF заголовки.
- Стиль: коментарі українською, як у miniproject_3; ідентифікатори англійською; 4 пробіли.
- Коміти: **без** трейлера `Co-Authored-By` (глобальне правило користувача).
- Команда хостових тестів (Git Bash, з кореня репо): `bash test/host/run_tests.sh`
- Команда збірки (PowerShell, з кореня репо): `. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1 *> $null; idf.py build`

## Review Focus

1. **Подія автотаймера, що вже в черзі, коли AUTO щойно вимкнули** → годування не має статися. Тест: `test_auto_timer_ignored_when_disabled` (Task 4).
2. **Відлік у момент спрацювання / після дедлайну** → `0s`, а не від'ємне чи гігантське число. Тест: `test_countdown_sec_past_deadline_is_zero` (Task 6).
3. **Дуже великий інтервал** → відлік не вилазить за 6 символів і не ламає рядок. Тест: `test_status_clamps_huge_countdown` (Task 6).
4. **DS1307 не відповідає коректно при 3.3 В і віддає `0xFF` або нулі** → час невалідний, `--:--`. Тести: `test_decode_rejects_all_ff`, `test_decode_rejects_all_zero` (Task 3).
5. **Перший старт нової прошивки без підключеного RTC** → мітка збірки не зберігається в NVS, тож коли RTC підключать, час встановиться. Тест: `test_decide_absent_wins_over_new_firmware` (Task 3) + код `rtc_init` зберігає мітку лише після успішного запису (Task 7).
6. **Перехід часу збірки + offset через північ / кінець місяця / року / 29 лютого** → коректна дата. Тести: `test_add_seconds_*` (Task 2).

---

### Task 1: Базова копія miniproject_3

**Files:**
- Create (копія): `main/buttons.c`, `main/buttons.h`, `main/buzzer.c`, `main/buzzer.h`, `main/config.h`, `main/encoder.c`, `main/encoder.h`, `main/feeder.c`, `main/feeder.h`, `main/feeder_types.h`, `main/leds.c`, `main/leds.h`, `main/motor.c`, `main/motor.h`, `main/servo.c`, `main/servo.h`, `main/settings.c`, `main/settings.h`
- Modify (перезапис копією): `main/main.c`, `main/CMakeLists.txt`
- Modify: `.gitignore` (додати `build_host/`)

**Interfaces:**
- Consumes: —
- Produces: робоча прошивка з поведінкою miniproject_3; усі API з miniproject_3 (`feed_cycle`, `input_task`, `feeder_task`, `auto_timer_start/stop`, `update_portion_leds`, `led_set`, `settings_*`, `buttons_*`, `encoder_init`, `event_queue`).

- [ ] **Step 1: Скопіювати сирці**

```bash
SRC=/c/embedded/miniproject_3/miniproject_3
cp "$SRC"/main/*.c "$SRC"/main/*.h main/
cp "$SRC"/main/CMakeLists.txt main/CMakeLists.txt
ls main
```

Expected: 20 файлів (`*.c`, `*.h`, `CMakeLists.txt`); `main/main.c` тепер містить `app_main` з miniproject_3.

- [ ] **Step 2: Додати `build_host/` у `.gitignore`**

Дописати в кінець `.gitignore`:

```
# Хостові модульні тести (test/host/run_tests.sh)
build_host/
```

- [ ] **Step 3: Чиста збірка**

Run (PowerShell):
```powershell
. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1 *> $null; idf.py fullclean; idf.py build
```
Expected: `Project build complete.`; далі `Select-String -Path sdkconfig -Pattern 'CONFIG_ESPTOOLPY_FLASHSIZE="16MB"'` знаходить рядок.

- [ ] **Step 4: Commit**

```bash
git add .gitignore .clangd .devcontainer CMakeLists.txt sdkconfig.defaults main
git commit -m "feat: port miniproject_3 feeder as baseline"
```

---

### Task 2: Хостові тести + `rtc_time`

**Files:**
- Create: `test/host/run_tests.sh`, `main/rtc_time.h`, `main/rtc_time.c`
- Test: `test/host/test_rtc_time.c`

**Interfaces:**
- Consumes: —
- Produces:
  ```c
  typedef struct { uint16_t year; uint8_t month, day, hour, minute, second; } rtc_datetime_t;
  uint8_t rtc_days_in_month(uint16_t year, uint8_t month);   /* 0 для month поза 1..12 */
  bool    rtc_datetime_is_valid(const rtc_datetime_t *t);    /* рік 2000..2099 */
  void    rtc_datetime_add_seconds(rtc_datetime_t *t, uint32_t seconds);
  ```
  Скрипт `test/host/run_tests.sh [test_name]` — збирає кожен `test/host/test_*.c` з усіма наявними чистими модулями.

- [ ] **Step 1: Скрипт запуску тестів**

`test/host/run_tests.sh`:
```bash
#!/usr/bin/env bash
# Хостові модульні тести чистої логіки (без заліза і без ESP-IDF заголовків).
# Використання: bash test/host/run_tests.sh [test_name]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
UNITY_DIR="${UNITY_DIR:-/c/esp/v6.0.2/esp-idf/components/unity/unity/src}"
# gcc з msys64 потребує своїх DLL у PATH, інакше падає мовчки.
export PATH="/c/msys64/ucrt64/bin:$PATH"
OUT="$ROOT/build_host"
mkdir -p "$OUT"

PURE_SRCS=()
for name in rtc_time ds1307_codec menu fb ui_text ui; do
    if [ -f "$ROOT/main/$name.c" ]; then
        PURE_SRCS+=("$ROOT/main/$name.c")
    fi
done

if [ $# -gt 0 ]; then
    TESTS=("$ROOT/test/host/$1.c")
else
    TESTS=("$ROOT"/test/host/test_*.c)
fi

fail=0
for test in "${TESTS[@]}"; do
    exe="$OUT/$(basename "${test%.c}").exe"
    gcc -std=c11 -Wall -Wextra -Werror -I"$ROOT/main" -I"$UNITY_DIR" \
        "$test" "${PURE_SRCS[@]}" "$UNITY_DIR/unity.c" -o "$exe"
    "$exe" || fail=1
done
exit $fail
```

- [ ] **Step 2: Написати тест, що падає**

`test/host/test_rtc_time.c`:
```c
#include "unity.h"
#include "rtc_time.h"

void setUp(void) {}
void tearDown(void) {}

static rtc_datetime_t dt(uint16_t y, uint8_t mo, uint8_t d, uint8_t h, uint8_t mi, uint8_t s)
{
    rtc_datetime_t t = { .year = y, .month = mo, .day = d, .hour = h, .minute = mi, .second = s };
    return t;
}

static void assert_dt(rtc_datetime_t exp, rtc_datetime_t act)
{
    TEST_ASSERT_EQUAL_UINT16(exp.year, act.year);
    TEST_ASSERT_EQUAL_UINT8(exp.month, act.month);
    TEST_ASSERT_EQUAL_UINT8(exp.day, act.day);
    TEST_ASSERT_EQUAL_UINT8(exp.hour, act.hour);
    TEST_ASSERT_EQUAL_UINT8(exp.minute, act.minute);
    TEST_ASSERT_EQUAL_UINT8(exp.second, act.second);
}

static void test_days_in_month(void)
{
    TEST_ASSERT_EQUAL_UINT8(31, rtc_days_in_month(2026, 1));
    TEST_ASSERT_EQUAL_UINT8(28, rtc_days_in_month(2026, 2));
    TEST_ASSERT_EQUAL_UINT8(29, rtc_days_in_month(2028, 2));
    TEST_ASSERT_EQUAL_UINT8(28, rtc_days_in_month(2100, 2));
    TEST_ASSERT_EQUAL_UINT8(30, rtc_days_in_month(2026, 4));
    TEST_ASSERT_EQUAL_UINT8(0, rtc_days_in_month(2026, 0));
    TEST_ASSERT_EQUAL_UINT8(0, rtc_days_in_month(2026, 13));
}

static void test_is_valid(void)
{
    rtc_datetime_t ok = dt(2026, 9, 27, 14, 37, 5);
    TEST_ASSERT_TRUE(rtc_datetime_is_valid(&ok));

    rtc_datetime_t feb29 = dt(2026, 2, 29, 0, 0, 0);
    rtc_datetime_t hour24 = dt(2026, 1, 1, 24, 0, 0);
    rtc_datetime_t min60 = dt(2026, 1, 1, 0, 60, 0);
    rtc_datetime_t y1999 = dt(1999, 1, 1, 0, 0, 0);
    rtc_datetime_t day0 = dt(2026, 1, 0, 0, 0, 0);
    TEST_ASSERT_FALSE(rtc_datetime_is_valid(&feb29));
    TEST_ASSERT_FALSE(rtc_datetime_is_valid(&hour24));
    TEST_ASSERT_FALSE(rtc_datetime_is_valid(&min60));
    TEST_ASSERT_FALSE(rtc_datetime_is_valid(&y1999));
    TEST_ASSERT_FALSE(rtc_datetime_is_valid(&day0));
}

static void test_add_seconds_simple(void)
{
    rtc_datetime_t t = dt(2026, 9, 27, 14, 37, 5);
    rtc_datetime_add_seconds(&t, 30);
    assert_dt(dt(2026, 9, 27, 14, 37, 35), t);
}

static void test_add_seconds_crosses_minute_and_hour(void)
{
    rtc_datetime_t t = dt(2026, 9, 27, 14, 59, 45);
    rtc_datetime_add_seconds(&t, 30);
    assert_dt(dt(2026, 9, 27, 15, 0, 15), t);
}

static void test_add_seconds_crosses_midnight(void)
{
    rtc_datetime_t t = dt(2026, 9, 27, 23, 59, 45);
    rtc_datetime_add_seconds(&t, 30);
    assert_dt(dt(2026, 9, 28, 0, 0, 15), t);
}

static void test_add_seconds_crosses_month_end(void)
{
    rtc_datetime_t t = dt(2026, 4, 30, 23, 59, 50);
    rtc_datetime_add_seconds(&t, 30);
    assert_dt(dt(2026, 5, 1, 0, 0, 20), t);
}

static void test_add_seconds_crosses_year_end(void)
{
    rtc_datetime_t t = dt(2026, 12, 31, 23, 59, 59);
    rtc_datetime_add_seconds(&t, 1);
    assert_dt(dt(2027, 1, 1, 0, 0, 0), t);
}

static void test_add_seconds_leap_day(void)
{
    rtc_datetime_t t = dt(2028, 2, 28, 23, 59, 59);
    rtc_datetime_add_seconds(&t, 1);
    assert_dt(dt(2028, 2, 29, 0, 0, 0), t);
}

static void test_add_seconds_multiple_days(void)
{
    rtc_datetime_t t = dt(2026, 2, 27, 12, 0, 0);
    rtc_datetime_add_seconds(&t, 3u * 24u * 3600u);
    assert_dt(dt(2026, 3, 2, 12, 0, 0), t);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_days_in_month);
    RUN_TEST(test_is_valid);
    RUN_TEST(test_add_seconds_simple);
    RUN_TEST(test_add_seconds_crosses_minute_and_hour);
    RUN_TEST(test_add_seconds_crosses_midnight);
    RUN_TEST(test_add_seconds_crosses_month_end);
    RUN_TEST(test_add_seconds_crosses_year_end);
    RUN_TEST(test_add_seconds_leap_day);
    RUN_TEST(test_add_seconds_multiple_days);
    return UNITY_END();
}
```

- [ ] **Step 3: Переконатися, що тест падає**

Run: `bash test/host/run_tests.sh test_rtc_time`
Expected: FAIL — помилка компіляції `rtc_time.h: No such file or directory`.

- [ ] **Step 4: Реалізація**

`main/rtc_time.h`:
```c
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Календарний час без часового поясу - локальний, як його зберігає DS1307. */
typedef struct {
    uint16_t year;    /* 2000..2099 - діапазон DS1307 */
    uint8_t  month;   /* 1..12 */
    uint8_t  day;     /* 1..31 */
    uint8_t  hour;    /* 0..23 */
    uint8_t  minute;  /* 0..59 */
    uint8_t  second;  /* 0..59 */
} rtc_datetime_t;

/* Кількість днів у місяці з урахуванням високосного року; 0 для month поза 1..12. */
uint8_t rtc_days_in_month(uint16_t year, uint8_t month);

bool rtc_datetime_is_valid(const rtc_datetime_t *t);

/* Додає секунди з переносом через хвилину, годину, добу, місяць і рік. */
void rtc_datetime_add_seconds(rtc_datetime_t *t, uint32_t seconds);
```

`main/rtc_time.c`:
```c
#include "rtc_time.h"

static bool is_leap(uint16_t year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

uint8_t rtc_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (month < 1 || month > 12) {
        return 0;
    }
    if (month == 2 && is_leap(year)) {
        return 29;
    }
    return days[month - 1];
}

bool rtc_datetime_is_valid(const rtc_datetime_t *t)
{
    return t->year >= 2000 && t->year <= 2099 &&
           t->month >= 1 && t->month <= 12 &&
           t->day >= 1 && t->day <= rtc_days_in_month(t->year, t->month) &&
           t->hour < 24 && t->minute < 60 && t->second < 60;
}

void rtc_datetime_add_seconds(rtc_datetime_t *t, uint32_t seconds)
{
    uint32_t total = t->second + seconds;
    t->second = (uint8_t)(total % 60);

    total = t->minute + total / 60;
    t->minute = (uint8_t)(total % 60);

    total = t->hour + total / 60;
    t->hour = (uint8_t)(total % 24);

    /* Доби додаються по одній: так перенос через кінець місяця
     * і року лишається тривіальним. */
    for (uint32_t days = total / 24; days > 0; days--) {
        if (t->day < rtc_days_in_month(t->year, t->month)) {
            t->day++;
            continue;
        }
        t->day = 1;
        if (t->month < 12) {
            t->month++;
        } else {
            t->month = 1;
            t->year++;
        }
    }
}
```

- [ ] **Step 5: Тести проходять**

Run: `bash test/host/run_tests.sh test_rtc_time`
Expected: `9 Tests 0 Failures 0 Ignored` і `OK`.

- [ ] **Step 6: Commit**

```bash
git add test/host/run_tests.sh test/host/test_rtc_time.c main/rtc_time.h main/rtc_time.c
git commit -m "feat: add calendar time helpers with host unit tests"
```

---

### Task 3: `ds1307_codec` — регістри, BCD, рішення при старті

**Files:**
- Create: `main/ds1307_codec.h`, `main/ds1307_codec.c`
- Test: `test/host/test_ds1307_codec.c`

**Interfaces:**
- Consumes: `rtc_datetime_t`, `rtc_datetime_is_valid()` з Task 2.
- Produces:
  ```c
  #define DS1307_TIME_REG_COUNT 7
  #define DS1307_RAM_MARKER_REG 0x08
  #define DS1307_MARKER_LEN     4
  extern const uint8_t ds1307_marker[DS1307_MARKER_LEN];
  uint8_t bcd_encode(uint8_t value);
  uint8_t bcd_decode(uint8_t bcd);
  bool    bcd_is_valid(uint8_t bcd);
  void    ds1307_encode(const rtc_datetime_t *t, uint8_t regs[DS1307_TIME_REG_COUNT]);
  bool    ds1307_decode(const uint8_t regs[DS1307_TIME_REG_COUNT], rtc_datetime_t *out);
  bool    ds1307_marker_matches(const uint8_t ram[DS1307_MARKER_LEN]);
  typedef enum { RTC_START_ABSENT, RTC_START_SET_FROM_BUILD, RTC_START_INVALID, RTC_START_VALID } rtc_start_t;
  rtc_start_t rtc_startup_decide(bool present, bool new_firmware, bool marker_ok, bool regs_ok);
  ```

- [ ] **Step 1: Написати тест, що падає**

`test/host/test_ds1307_codec.c`:
```c
#include <string.h>

#include "unity.h"
#include "ds1307_codec.h"

void setUp(void) {}
void tearDown(void) {}

static void test_bcd_roundtrip(void)
{
    for (uint8_t v = 0; v < 100; v++) {
        uint8_t b = bcd_encode(v);
        TEST_ASSERT_TRUE(bcd_is_valid(b));
        TEST_ASSERT_EQUAL_UINT8(v, bcd_decode(b));
    }
    TEST_ASSERT_EQUAL_HEX8(0x59, bcd_encode(59));
}

static void test_bcd_invalid_nibbles(void)
{
    TEST_ASSERT_FALSE(bcd_is_valid(0x1A));
    TEST_ASSERT_FALSE(bcd_is_valid(0xA1));
    TEST_ASSERT_FALSE(bcd_is_valid(0xFF));
}

static void test_encode_known_time(void)
{
    rtc_datetime_t t = { .year = 2026, .month = 9, .day = 27, .hour = 14, .minute = 37, .second = 5 };
    uint8_t regs[DS1307_TIME_REG_COUNT];
    const uint8_t expected[DS1307_TIME_REG_COUNT] = { 0x05, 0x37, 0x14, 0x01, 0x27, 0x09, 0x26 };

    ds1307_encode(&t, regs);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, regs, DS1307_TIME_REG_COUNT);
}

static void test_decode_known_time(void)
{
    const uint8_t regs[DS1307_TIME_REG_COUNT] = { 0x05, 0x37, 0x14, 0x07, 0x27, 0x09, 0x26 };
    rtc_datetime_t t;

    TEST_ASSERT_TRUE(ds1307_decode(regs, &t));
    TEST_ASSERT_EQUAL_UINT16(2026, t.year);
    TEST_ASSERT_EQUAL_UINT8(9, t.month);
    TEST_ASSERT_EQUAL_UINT8(27, t.day);
    TEST_ASSERT_EQUAL_UINT8(14, t.hour);
    TEST_ASSERT_EQUAL_UINT8(37, t.minute);
    TEST_ASSERT_EQUAL_UINT8(5, t.second);
}

static void test_decode_rejects_clock_halt(void)
{
    const uint8_t regs[DS1307_TIME_REG_COUNT] = { 0x80 | 0x05, 0x37, 0x14, 0x01, 0x27, 0x09, 0x26 };
    rtc_datetime_t t;
    TEST_ASSERT_FALSE(ds1307_decode(regs, &t));
}

static void test_decode_rejects_12h_mode(void)
{
    const uint8_t regs[DS1307_TIME_REG_COUNT] = { 0x05, 0x37, 0x40 | 0x02, 0x01, 0x27, 0x09, 0x26 };
    rtc_datetime_t t;
    TEST_ASSERT_FALSE(ds1307_decode(regs, &t));
}

static void test_decode_rejects_all_ff(void)
{
    uint8_t regs[DS1307_TIME_REG_COUNT];
    rtc_datetime_t t;
    memset(regs, 0xFF, sizeof regs);
    TEST_ASSERT_FALSE(ds1307_decode(regs, &t));
}

static void test_decode_rejects_all_zero(void)
{
    const uint8_t regs[DS1307_TIME_REG_COUNT] = { 0 };   /* день і місяць 0 */
    rtc_datetime_t t;
    TEST_ASSERT_FALSE(ds1307_decode(regs, &t));
}

static void test_decode_rejects_impossible_date(void)
{
    const uint8_t regs[DS1307_TIME_REG_COUNT] = { 0x00, 0x00, 0x00, 0x01, 0x30, 0x02, 0x26 };  /* 30 лютого */
    rtc_datetime_t t;
    TEST_ASSERT_FALSE(ds1307_decode(regs, &t));
}

static void test_decode_leaves_out_untouched_on_failure(void)
{
    uint8_t regs[DS1307_TIME_REG_COUNT];
    rtc_datetime_t t = { .year = 2030, .month = 1, .day = 1, .hour = 1, .minute = 1, .second = 1 };
    memset(regs, 0xFF, sizeof regs);
    ds1307_decode(regs, &t);
    TEST_ASSERT_EQUAL_UINT16(2030, t.year);
}

static void test_marker_matches(void)
{
    uint8_t ram[DS1307_MARKER_LEN];
    memcpy(ram, ds1307_marker, DS1307_MARKER_LEN);
    TEST_ASSERT_TRUE(ds1307_marker_matches(ram));
    ram[3] ^= 0x01;
    TEST_ASSERT_FALSE(ds1307_marker_matches(ram));
}

static void test_decide_absent_wins_over_new_firmware(void)
{
    TEST_ASSERT_EQUAL(RTC_START_ABSENT, rtc_startup_decide(false, true, false, false));
    TEST_ASSERT_EQUAL(RTC_START_ABSENT, rtc_startup_decide(false, false, true, true));
}

static void test_decide_new_firmware_sets_time(void)
{
    TEST_ASSERT_EQUAL(RTC_START_SET_FROM_BUILD, rtc_startup_decide(true, true, false, false));
    TEST_ASSERT_EQUAL(RTC_START_SET_FROM_BUILD, rtc_startup_decide(true, true, true, true));
}

static void test_decide_valid_needs_marker_and_regs(void)
{
    TEST_ASSERT_EQUAL(RTC_START_VALID, rtc_startup_decide(true, false, true, true));
    TEST_ASSERT_EQUAL(RTC_START_INVALID, rtc_startup_decide(true, false, false, true));
    TEST_ASSERT_EQUAL(RTC_START_INVALID, rtc_startup_decide(true, false, true, false));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bcd_roundtrip);
    RUN_TEST(test_bcd_invalid_nibbles);
    RUN_TEST(test_encode_known_time);
    RUN_TEST(test_decode_known_time);
    RUN_TEST(test_decode_rejects_clock_halt);
    RUN_TEST(test_decode_rejects_12h_mode);
    RUN_TEST(test_decode_rejects_all_ff);
    RUN_TEST(test_decode_rejects_all_zero);
    RUN_TEST(test_decode_rejects_impossible_date);
    RUN_TEST(test_decode_leaves_out_untouched_on_failure);
    RUN_TEST(test_marker_matches);
    RUN_TEST(test_decide_absent_wins_over_new_firmware);
    RUN_TEST(test_decide_new_firmware_sets_time);
    RUN_TEST(test_decide_valid_needs_marker_and_regs);
    return UNITY_END();
}
```

- [ ] **Step 2: Переконатися, що тест падає**

Run: `bash test/host/run_tests.sh test_ds1307_codec`
Expected: FAIL — `ds1307_codec.h: No such file or directory`.

- [ ] **Step 3: Реалізація**

`main/ds1307_codec.h`:
```c
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rtc_time.h"

/* Регістри 0x00..0x06: секунди, хвилини, години, день тижня, дата, місяць, рік. */
#define DS1307_TIME_REG_COUNT   7

/* Початок RAM DS1307 (56 байт), що живе, поки модуль має живлення. */
#define DS1307_RAM_MARKER_REG   0x08
#define DS1307_MARKER_LEN       4

/* Маркер "час встановлено прошивкою, і живлення відтоді не пропадало".
 * Після подачі живлення без батарейки регістри DS1307 не визначені,
 * тож одного біта CH для цього недостатньо. */
extern const uint8_t ds1307_marker[DS1307_MARKER_LEN];

uint8_t bcd_encode(uint8_t value);   /* 0..99 */
uint8_t bcd_decode(uint8_t bcd);
bool    bcd_is_valid(uint8_t bcd);   /* обидві тетради 0..9 */

/* 24-годинний режим, CH = 0 (осцилятор запущено), день тижня = 1 (не використовується). */
void ds1307_encode(const rtc_datetime_t *t, uint8_t regs[DS1307_TIME_REG_COUNT]);

/* false (і *out не змінюється), якщо CH = 1, 12-годинний режим,
 * некоректний BCD або неіснуюча дата. */
bool ds1307_decode(const uint8_t regs[DS1307_TIME_REG_COUNT], rtc_datetime_t *out);

bool ds1307_marker_matches(const uint8_t ram[DS1307_MARKER_LEN]);

typedef enum {
    RTC_START_ABSENT,          /* чип не відповідає */
    RTC_START_SET_FROM_BUILD,  /* перший старт нової прошивки: записати час збірки */
    RTC_START_INVALID,         /* живлення RTC пропадало: --:-- до наступної прошивки */
    RTC_START_VALID,           /* час іде з моменту встановлення */
} rtc_start_t;

/* Алгоритм старту (спек, розділ 5.3). regs_ok - ds1307_decode() повернула true. */
rtc_start_t rtc_startup_decide(bool present, bool new_firmware, bool marker_ok, bool regs_ok);
```

`main/ds1307_codec.c`:
```c
#include "ds1307_codec.h"

#include <string.h>

#define REG_SEC_CH      0x80  /* Clock Halt: 1 = осцилятор зупинено */
#define REG_HOUR_12H    0x40  /* 1 = 12-годинний режим */

const uint8_t ds1307_marker[DS1307_MARKER_LEN] = { 'F', 'D', 'R', '4' };

uint8_t bcd_encode(uint8_t value)
{
    return (uint8_t)(((value / 10) << 4) | (value % 10));
}

uint8_t bcd_decode(uint8_t bcd)
{
    return (uint8_t)((bcd >> 4) * 10 + (bcd & 0x0F));
}

bool bcd_is_valid(uint8_t bcd)
{
    return (bcd >> 4) <= 9 && (bcd & 0x0F) <= 9;
}

void ds1307_encode(const rtc_datetime_t *t, uint8_t regs[DS1307_TIME_REG_COUNT])
{
    regs[0] = bcd_encode(t->second);                    /* CH = 0 */
    regs[1] = bcd_encode(t->minute);
    regs[2] = bcd_encode(t->hour);                      /* біт 6 = 0 -> 24h */
    regs[3] = 1;
    regs[4] = bcd_encode(t->day);
    regs[5] = bcd_encode(t->month);
    regs[6] = bcd_encode((uint8_t)(t->year - 2000));
}

bool ds1307_decode(const uint8_t regs[DS1307_TIME_REG_COUNT], rtc_datetime_t *out)
{
    if ((regs[0] & REG_SEC_CH) || (regs[2] & REG_HOUR_12H)) {
        return false;
    }

    uint8_t sec = regs[0] & 0x7F;
    uint8_t hour = regs[2] & 0x3F;
    if (!bcd_is_valid(sec) || !bcd_is_valid(regs[1]) || !bcd_is_valid(hour) ||
        !bcd_is_valid(regs[4]) || !bcd_is_valid(regs[5]) || !bcd_is_valid(regs[6])) {
        return false;
    }

    rtc_datetime_t t = {
        .year = (uint16_t)(2000 + bcd_decode(regs[6])),
        .month = bcd_decode(regs[5]),
        .day = bcd_decode(regs[4]),
        .hour = bcd_decode(hour),
        .minute = bcd_decode(regs[1]),
        .second = bcd_decode(sec),
    };
    if (!rtc_datetime_is_valid(&t)) {
        return false;
    }
    *out = t;
    return true;
}

bool ds1307_marker_matches(const uint8_t ram[DS1307_MARKER_LEN])
{
    return memcmp(ram, ds1307_marker, DS1307_MARKER_LEN) == 0;
}

rtc_start_t rtc_startup_decide(bool present, bool new_firmware, bool marker_ok, bool regs_ok)
{
    if (!present) {
        return RTC_START_ABSENT;
    }
    if (new_firmware) {
        return RTC_START_SET_FROM_BUILD;
    }
    if (marker_ok && regs_ok) {
        return RTC_START_VALID;
    }
    return RTC_START_INVALID;
}
```

- [ ] **Step 4: Тести проходять**

Run: `bash test/host/run_tests.sh`
Expected: обидва тест-файли `OK` (9 + 14 тестів, 0 Failures).

- [ ] **Step 5: Commit**

```bash
git add main/ds1307_codec.h main/ds1307_codec.c test/host/test_ds1307_codec.c
git commit -m "feat: add DS1307 register codec and startup decision logic"
```

---

### Task 4: Чиста логіка меню

**Files:**
- Modify: `main/feeder_types.h` (порція + нові події), `main/config.h` (прибрати блок «Порція», включити `feeder_types.h`)
- Create: `main/menu.h`, `main/menu.c`
- Test: `test/host/test_menu.c`

**Interfaces:**
- Consumes: `event_type_t` з `feeder_types.h`.
- Produces:
  ```c
  /* feeder_types.h */
  #define PORTION_MIN 1
  #define PORTION_MAX 4
  typedef enum { EVT_ENCODER_DELTA, EVT_BTN_MANUAL, EVT_BTN_AUTO_TOGGLE, EVT_AUTO_TIMER,
                 EVT_ENC_CLICK, EVT_BTN_BACK } event_type_t;   /* EVT_BTN_AUTO_TOGGLE прибирається в Task 9 */
  /* menu.h */
  typedef enum { SCREEN_MAIN, SCREEN_MENU_ROOT, SCREEN_MENU_PORTION, SCREEN_MENU_AUTO } ui_screen_t;
  #define MENU_ROOT_PORTION 0
  #define MENU_ROOT_AUTO 1
  #define MENU_AUTO_ENABLED 0
  #define MENU_AUTO_DISABLED 1
  typedef struct { ui_screen_t screen; uint8_t cursor; uint8_t portion; uint8_t preview;
                   bool auto_enabled; bool feeding; } ui_state_t;
  typedef enum { ACT_NONE, ACT_SHOW_LEDS, ACT_COMMIT_PORTION, ACT_SET_AUTO, ACT_FEED } menu_action_type_t;
  typedef struct { menu_action_type_t type; uint8_t arg; } menu_action_t;
  void menu_init(ui_state_t *st, uint8_t portion, bool auto_enabled);
  menu_action_t menu_handle(ui_state_t *st, event_type_t evt, int8_t value);
  ```

- [ ] **Step 1: Оновити `main/feeder_types.h`**

Повний вміст:
```c
#pragma once

#include <stdint.h>

/* ---------- Порція (тут, а не в config.h: потрібна чистій логіці без ESP-IDF) ---------- */
#define PORTION_MIN             1
#define PORTION_MAX             4

typedef enum {
    EVT_ENCODER_DELTA,    /* поворот енкодера, value = +1 або -1 */
    EVT_BTN_MANUAL,       /* натиснута кнопка ручного насипання */
    EVT_BTN_AUTO_TOGGLE,  /* застаріла: прибирається разом зі старим feeder_task */
    EVT_AUTO_TIMER,       /* спрацював таймер авто-режиму */
    EVT_ENC_CLICK,        /* клік енкодера (GPIO6) */
    EVT_BTN_BACK,         /* кнопка «Назад» (GPIO8) */
} event_type_t;

typedef struct {
    event_type_t type;
    int8_t value;
} feeder_event_t;

typedef enum {
    LED_AUTO,     /* червоний: AUTO on/off */
    LED_FEEDING,  /* зелений: іде насипання */
} led_id_t;
```

- [ ] **Step 2: Оновити `main/config.h`**

Після `#include "driver/gpio.h"` додати рядок:
```c
#include "feeder_types.h"   /* PORTION_MIN / PORTION_MAX */
```
і видалити блок:
```c
/* ---------- Порція ---------- */
#define PORTION_MIN             1
#define PORTION_MAX             4
```

- [ ] **Step 3: Написати тест, що падає**

`test/host/test_menu.c`:
```c
#include "unity.h"
#include "menu.h"

void setUp(void) {}
void tearDown(void) {}

static ui_state_t st;

static menu_action_t send(event_type_t evt, int8_t value)
{
    return menu_handle(&st, evt, value);
}

static void assert_action(menu_action_type_t type, uint8_t arg, menu_action_t a)
{
    TEST_ASSERT_EQUAL(type, a.type);
    TEST_ASSERT_EQUAL_UINT8(arg, a.arg);
}

/* Головний екран, порція 2, AUTO вимкнено. */
static void start(void)
{
    menu_init(&st, 2, false);
}

static void open_portion(void)
{
    start();
    send(EVT_ENC_CLICK, 0);   /* -> MENU_ROOT, курсор на Portion */
    send(EVT_ENC_CLICK, 0);   /* -> MENU_PORTION */
}

static void open_auto(bool auto_enabled)
{
    menu_init(&st, 2, auto_enabled);
    send(EVT_ENC_CLICK, 0);
    send(EVT_ENCODER_DELTA, +1);   /* курсор на Auto Mode */
    send(EVT_ENC_CLICK, 0);
}

static void test_init_state(void)
{
    start();
    TEST_ASSERT_EQUAL(SCREEN_MAIN, st.screen);
    TEST_ASSERT_EQUAL_UINT8(2, st.portion);
    TEST_ASSERT_EQUAL_UINT8(2, st.preview);
    TEST_ASSERT_FALSE(st.auto_enabled);
    TEST_ASSERT_FALSE(st.feeding);
}

static void test_main_ignores_rotation_and_back(void)
{
    start();
    assert_action(ACT_NONE, 0, send(EVT_ENCODER_DELTA, +1));
    assert_action(ACT_NONE, 0, send(EVT_BTN_BACK, 0));
    TEST_ASSERT_EQUAL(SCREEN_MAIN, st.screen);
    TEST_ASSERT_EQUAL_UINT8(2, st.portion);
}

static void test_main_click_opens_root_on_portion(void)
{
    start();
    assert_action(ACT_NONE, 0, send(EVT_ENC_CLICK, 0));
    TEST_ASSERT_EQUAL(SCREEN_MENU_ROOT, st.screen);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_PORTION, st.cursor);
}

static void test_root_cursor_clamps(void)
{
    start();
    send(EVT_ENC_CLICK, 0);
    send(EVT_ENCODER_DELTA, +1);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_AUTO, st.cursor);
    send(EVT_ENCODER_DELTA, +1);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_AUTO, st.cursor);
    send(EVT_ENCODER_DELTA, -1);
    send(EVT_ENCODER_DELTA, -1);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_PORTION, st.cursor);
}

static void test_root_back_returns_to_main(void)
{
    start();
    send(EVT_ENC_CLICK, 0);
    assert_action(ACT_NONE, 0, send(EVT_BTN_BACK, 0));
    TEST_ASSERT_EQUAL(SCREEN_MAIN, st.screen);
}

static void test_portion_opens_with_current_level(void)
{
    open_portion();
    TEST_ASSERT_EQUAL(SCREEN_MENU_PORTION, st.screen);
    TEST_ASSERT_EQUAL_UINT8(2, st.preview);
}

static void test_portion_rotation_previews_and_clamps(void)
{
    open_portion();
    assert_action(ACT_SHOW_LEDS, 3, send(EVT_ENCODER_DELTA, +1));
    assert_action(ACT_SHOW_LEDS, 4, send(EVT_ENCODER_DELTA, +1));
    assert_action(ACT_SHOW_LEDS, 4, send(EVT_ENCODER_DELTA, +1));
    TEST_ASSERT_EQUAL_UINT8(2, st.portion);   /* ще не підтверджено */
    for (int i = 0; i < 5; i++) {
        send(EVT_ENCODER_DELTA, -1);
    }
    TEST_ASSERT_EQUAL_UINT8(PORTION_MIN, st.preview);
}

static void test_portion_click_commits_and_returns_to_root(void)
{
    open_portion();
    send(EVT_ENCODER_DELTA, +1);
    send(EVT_ENCODER_DELTA, +1);
    assert_action(ACT_COMMIT_PORTION, 4, send(EVT_ENC_CLICK, 0));
    TEST_ASSERT_EQUAL_UINT8(4, st.portion);
    TEST_ASSERT_EQUAL(SCREEN_MENU_ROOT, st.screen);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_PORTION, st.cursor);
}

static void test_portion_back_reverts_preview(void)
{
    open_portion();
    send(EVT_ENCODER_DELTA, +1);
    send(EVT_ENCODER_DELTA, +1);
    assert_action(ACT_SHOW_LEDS, 2, send(EVT_BTN_BACK, 0));
    TEST_ASSERT_EQUAL_UINT8(2, st.portion);
    TEST_ASSERT_EQUAL_UINT8(2, st.preview);
    TEST_ASSERT_EQUAL(SCREEN_MENU_ROOT, st.screen);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_PORTION, st.cursor);
}

static void test_auto_cursor_starts_on_current_mode(void)
{
    open_auto(false);
    TEST_ASSERT_EQUAL(SCREEN_MENU_AUTO, st.screen);
    TEST_ASSERT_EQUAL_UINT8(MENU_AUTO_DISABLED, st.cursor);
    open_auto(true);
    TEST_ASSERT_EQUAL_UINT8(MENU_AUTO_ENABLED, st.cursor);
}

static void test_auto_cursor_clamps(void)
{
    open_auto(false);
    send(EVT_ENCODER_DELTA, +1);
    TEST_ASSERT_EQUAL_UINT8(MENU_AUTO_DISABLED, st.cursor);
    send(EVT_ENCODER_DELTA, -1);
    send(EVT_ENCODER_DELTA, -1);
    TEST_ASSERT_EQUAL_UINT8(MENU_AUTO_ENABLED, st.cursor);
}

static void test_auto_click_enables(void)
{
    open_auto(false);
    send(EVT_ENCODER_DELTA, -1);   /* курсор на Enabled */
    assert_action(ACT_SET_AUTO, 1, send(EVT_ENC_CLICK, 0));
    TEST_ASSERT_TRUE(st.auto_enabled);
    TEST_ASSERT_EQUAL(SCREEN_MENU_ROOT, st.screen);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_AUTO, st.cursor);
}

static void test_auto_click_disables(void)
{
    open_auto(true);
    send(EVT_ENCODER_DELTA, +1);   /* курсор на Disabled */
    assert_action(ACT_SET_AUTO, 0, send(EVT_ENC_CLICK, 0));
    TEST_ASSERT_FALSE(st.auto_enabled);
}

static void test_auto_click_same_mode_is_noop(void)
{
    open_auto(true);
    assert_action(ACT_NONE, 0, send(EVT_ENC_CLICK, 0));
    TEST_ASSERT_TRUE(st.auto_enabled);
    TEST_ASSERT_EQUAL(SCREEN_MENU_ROOT, st.screen);
}

static void test_auto_back_keeps_mode(void)
{
    open_auto(false);
    send(EVT_ENCODER_DELTA, -1);
    assert_action(ACT_NONE, 0, send(EVT_BTN_BACK, 0));
    TEST_ASSERT_FALSE(st.auto_enabled);
    TEST_ASSERT_EQUAL(SCREEN_MENU_ROOT, st.screen);
    TEST_ASSERT_EQUAL_UINT8(MENU_ROOT_AUTO, st.cursor);
}

static void test_manual_feeds_from_any_screen_and_keeps_menu(void)
{
    start();
    assert_action(ACT_FEED, 0, send(EVT_BTN_MANUAL, 0));
    TEST_ASSERT_EQUAL(SCREEN_MAIN, st.screen);

    open_portion();
    send(EVT_ENCODER_DELTA, +1);
    send(EVT_ENCODER_DELTA, +1);
    assert_action(ACT_FEED, 0, send(EVT_BTN_MANUAL, 0));
    TEST_ASSERT_EQUAL(SCREEN_MENU_PORTION, st.screen);
    TEST_ASSERT_EQUAL_UINT8(4, st.preview);   /* непідтверджений вибір зберігся */
    TEST_ASSERT_EQUAL_UINT8(2, st.portion);   /* годуємо підтвердженою */
}

static void test_auto_timer_feeds_when_enabled(void)
{
    menu_init(&st, 2, true);
    assert_action(ACT_FEED, 0, send(EVT_AUTO_TIMER, 0));
}

static void test_auto_timer_ignored_when_disabled(void)
{
    start();
    assert_action(ACT_NONE, 0, send(EVT_AUTO_TIMER, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_state);
    RUN_TEST(test_main_ignores_rotation_and_back);
    RUN_TEST(test_main_click_opens_root_on_portion);
    RUN_TEST(test_root_cursor_clamps);
    RUN_TEST(test_root_back_returns_to_main);
    RUN_TEST(test_portion_opens_with_current_level);
    RUN_TEST(test_portion_rotation_previews_and_clamps);
    RUN_TEST(test_portion_click_commits_and_returns_to_root);
    RUN_TEST(test_portion_back_reverts_preview);
    RUN_TEST(test_auto_cursor_starts_on_current_mode);
    RUN_TEST(test_auto_cursor_clamps);
    RUN_TEST(test_auto_click_enables);
    RUN_TEST(test_auto_click_disables);
    RUN_TEST(test_auto_click_same_mode_is_noop);
    RUN_TEST(test_auto_back_keeps_mode);
    RUN_TEST(test_manual_feeds_from_any_screen_and_keeps_menu);
    RUN_TEST(test_auto_timer_feeds_when_enabled);
    RUN_TEST(test_auto_timer_ignored_when_disabled);
    return UNITY_END();
}
```

- [ ] **Step 4: Переконатися, що тест падає**

Run: `bash test/host/run_tests.sh test_menu`
Expected: FAIL — `menu.h: No such file or directory`.

- [ ] **Step 5: Реалізація**

`main/menu.h`:
```c
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "feeder_types.h"

typedef enum {
    SCREEN_MAIN,
    SCREEN_MENU_ROOT,
    SCREEN_MENU_PORTION,
    SCREEN_MENU_AUTO,
} ui_screen_t;

/* Пункти кореневого меню і меню Auto Mode (значення поля cursor). */
#define MENU_ROOT_PORTION   0
#define MENU_ROOT_AUTO      1
#define MENU_AUTO_ENABLED   0
#define MENU_AUTO_DISABLED  1

/* Увесь стан, який бачить користувач. Власник - feeder_task;
 * display_task отримує копію через ui_state_get(). */
typedef struct {
    ui_screen_t screen;
    uint8_t cursor;         /* курсор у MENU_ROOT / MENU_AUTO */
    uint8_t portion;        /* підтверджена порція */
    uint8_t preview;        /* рівень попереднього перегляду в MENU_PORTION */
    bool    auto_enabled;
    bool    feeding;        /* виставляє feeder_task навколо feed_cycle() */
} ui_state_t;

typedef enum {
    ACT_NONE,
    ACT_SHOW_LEDS,       /* arg = рівень для шкали LED (перегляд або відкат) */
    ACT_COMMIT_PORTION,  /* arg = нова підтверджена порція: LED + NVS */
    ACT_SET_AUTO,        /* arg = 0/1: LED AUTO, NVS, старт/стоп таймера */
    ACT_FEED,            /* годувати підтвердженою порцією */
} menu_action_type_t;

typedef struct {
    menu_action_type_t type;
    uint8_t arg;
} menu_action_t;

void menu_init(ui_state_t *st, uint8_t portion, bool auto_enabled);

/* Чиста функція переходу: оновлює *st і повертає побічний ефект,
 * який має виконати feeder_task. Заліза не торкається. */
menu_action_t menu_handle(ui_state_t *st, event_type_t evt, int8_t value);
```

`main/menu.c`:
```c
#include "menu.h"

static menu_action_t act(menu_action_type_t type, uint8_t arg)
{
    menu_action_t a = { .type = type, .arg = arg };
    return a;
}

static uint8_t clamp_step(uint8_t value, int8_t delta, uint8_t lo, uint8_t hi)
{
    int r = (int)value + delta;
    if (r < lo) r = lo;
    if (r > hi) r = hi;
    return (uint8_t)r;
}

void menu_init(ui_state_t *st, uint8_t portion, bool auto_enabled)
{
    st->screen = SCREEN_MAIN;
    st->cursor = 0;
    st->portion = portion;
    st->preview = portion;
    st->auto_enabled = auto_enabled;
    st->feeding = false;
}

static menu_action_t handle_main(ui_state_t *st, event_type_t evt)
{
    /* Поворот і «Назад» на головному екрані нічого не роблять (спек 4.1). */
    if (evt == EVT_ENC_CLICK) {
        st->screen = SCREEN_MENU_ROOT;
        st->cursor = MENU_ROOT_PORTION;
    }
    return act(ACT_NONE, 0);
}

static menu_action_t handle_root(ui_state_t *st, event_type_t evt, int8_t value)
{
    switch (evt) {
    case EVT_ENCODER_DELTA:
        st->cursor = clamp_step(st->cursor, value, MENU_ROOT_PORTION, MENU_ROOT_AUTO);
        break;
    case EVT_ENC_CLICK:
        if (st->cursor == MENU_ROOT_PORTION) {
            st->screen = SCREEN_MENU_PORTION;
            st->preview = st->portion;
        } else {
            st->screen = SCREEN_MENU_AUTO;
            st->cursor = st->auto_enabled ? MENU_AUTO_ENABLED : MENU_AUTO_DISABLED;
        }
        break;
    case EVT_BTN_BACK:
        st->screen = SCREEN_MAIN;
        st->cursor = 0;
        break;
    default:
        break;
    }
    return act(ACT_NONE, 0);
}

static menu_action_t handle_portion(ui_state_t *st, event_type_t evt, int8_t value)
{
    switch (evt) {
    case EVT_ENCODER_DELTA:
        st->preview = clamp_step(st->preview, value, PORTION_MIN, PORTION_MAX);
        return act(ACT_SHOW_LEDS, st->preview);
    case EVT_ENC_CLICK:
        st->portion = st->preview;
        st->screen = SCREEN_MENU_ROOT;
        st->cursor = MENU_ROOT_PORTION;
        return act(ACT_COMMIT_PORTION, st->portion);
    case EVT_BTN_BACK:
        st->preview = st->portion;
        st->screen = SCREEN_MENU_ROOT;
        st->cursor = MENU_ROOT_PORTION;
        return act(ACT_SHOW_LEDS, st->portion);
    default:
        return act(ACT_NONE, 0);
    }
}

static menu_action_t handle_auto(ui_state_t *st, event_type_t evt, int8_t value)
{
    switch (evt) {
    case EVT_ENCODER_DELTA:
        st->cursor = clamp_step(st->cursor, value, MENU_AUTO_ENABLED, MENU_AUTO_DISABLED);
        return act(ACT_NONE, 0);
    case EVT_ENC_CLICK: {
        bool want = (st->cursor == MENU_AUTO_ENABLED);
        st->screen = SCREEN_MENU_ROOT;
        st->cursor = MENU_ROOT_AUTO;
        if (want == st->auto_enabled) {
            return act(ACT_NONE, 0);   /* той самий режим: таймер не перезапускаємо */
        }
        st->auto_enabled = want;
        return act(ACT_SET_AUTO, want ? 1 : 0);
    }
    case EVT_BTN_BACK:
        st->screen = SCREEN_MENU_ROOT;
        st->cursor = MENU_ROOT_AUTO;
        return act(ACT_NONE, 0);
    default:
        return act(ACT_NONE, 0);
    }
}

menu_action_t menu_handle(ui_state_t *st, event_type_t evt, int8_t value)
{
    /* Годування не залежить від екрана, і меню після нього лишається
     * тим самим (спек 4.5). Подія таймера могла потрапити в чергу
     * за мить до вимкнення AUTO - тоді її ігноруємо. */
    if (evt == EVT_BTN_MANUAL) {
        return act(ACT_FEED, 0);
    }
    if (evt == EVT_AUTO_TIMER) {
        return st->auto_enabled ? act(ACT_FEED, 0) : act(ACT_NONE, 0);
    }

    switch (st->screen) {
    case SCREEN_MAIN:         return handle_main(st, evt);
    case SCREEN_MENU_ROOT:    return handle_root(st, evt, value);
    case SCREEN_MENU_PORTION: return handle_portion(st, evt, value);
    case SCREEN_MENU_AUTO:    return handle_auto(st, evt, value);
    }
    return act(ACT_NONE, 0);
}
```

- [ ] **Step 6: Тести проходять + прошивка збирається**

Run: `bash test/host/run_tests.sh`
Expected: усі три файли `OK`, `test_menu` — 18 Tests 0 Failures.

Run (PowerShell): `. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1 *> $null; idf.py build`
Expected: `Project build complete.` (menu.c ще не в CMake — перевіряємо, що зміни `feeder_types.h`/`config.h` нічого не зламали).

- [ ] **Step 7: Commit**

```bash
git add main/feeder_types.h main/config.h main/menu.h main/menu.c test/host/test_menu.c
git commit -m "feat: add pure menu state machine with host tests"
```

---

### Task 5: Кадровий буфер і шрифт

**Files:**
- Create: `main/font8x8_basic.h` (завантажений), `main/fb.h`, `main/fb.c`
- Test: `test/host/test_fb.c`

**Interfaces:**
- Consumes: —
- Produces:
  ```c
  #define FB_WIDTH 128
  #define FB_HEIGHT 64
  #define FB_CHAR_W 8
  #define FB_CHAR_H 16
  typedef struct { uint8_t buf[FB_WIDTH * FB_HEIGHT / 8]; } fb_t;   /* формат сторінок SSD1306 */
  void fb_clear(fb_t *fb);
  void fb_set_pixel(fb_t *fb, int x, int y, bool on);   /* поза межами - ігнорується */
  bool fb_get_pixel(const fb_t *fb, int x, int y);      /* поза межами - false */
  void fb_draw_char(fb_t *fb, int x, int y, char c);    /* 8x16; не ASCII 0x20..0x7E -> '?' */
  void fb_draw_text(fb_t *fb, int x, int y, const char *s);
  void fb_invert_rect(fb_t *fb, int x, int y, int w, int h);
  ```

- [ ] **Step 1: Завантажити шрифт (public domain) і зробити його `static const`**

```bash
{ echo '#pragma once'; curl -fsSL https://raw.githubusercontent.com/dhepper/font8x8/master/font8x8_basic.h \
  | sed 's/^char font8x8_basic\[128\]\[8\]/static const unsigned char font8x8_basic[128][8]/'; } > main/font8x8_basic.h
grep -c 'static const unsigned char font8x8_basic\[128\]\[8\]' main/font8x8_basic.h
grep -c 'License: Public Domain' main/font8x8_basic.h
```
Expected: `1` і `1`. Якщо `curl` не вдається — зупинитися і повідомити користувача (не підставляти шрифт з іншого джерела).

- [ ] **Step 2: Написати тест, що падає**

`test/host/test_fb.c`:
```c
#include <string.h>

#include "unity.h"
#include "fb.h"
#include "font8x8_basic.h"

void setUp(void) {}
void tearDown(void) {}

static fb_t fb;

static void assert_glyph_at(int x, int y, char c)
{
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            bool expected = (font8x8_basic[(unsigned char)c][row] >> col) & 1;
            TEST_ASSERT_EQUAL(expected, fb_get_pixel(&fb, x + col, y + 2 * row));
            TEST_ASSERT_EQUAL(expected, fb_get_pixel(&fb, x + col, y + 2 * row + 1));
        }
    }
}

static void test_clear_zeroes_buffer(void)
{
    memset(fb.buf, 0xAA, sizeof fb.buf);
    fb_clear(&fb);
    for (size_t i = 0; i < sizeof fb.buf; i++) {
        TEST_ASSERT_EQUAL_HEX8(0, fb.buf[i]);
    }
}

static void test_pixel_uses_ssd1306_page_layout(void)
{
    fb_clear(&fb);
    fb_set_pixel(&fb, 0, 0, true);
    TEST_ASSERT_EQUAL_HEX8(0x01, fb.buf[0]);
    fb_set_pixel(&fb, 5, 13, true);                 /* сторінка 1, біт 5 */
    TEST_ASSERT_EQUAL_HEX8(0x20, fb.buf[FB_WIDTH + 5]);
    TEST_ASSERT_TRUE(fb_get_pixel(&fb, 5, 13));
    fb_set_pixel(&fb, 5, 13, false);
    TEST_ASSERT_FALSE(fb_get_pixel(&fb, 5, 13));
}

static void test_out_of_bounds_is_ignored(void)
{
    fb_clear(&fb);
    fb_set_pixel(&fb, -1, 0, true);
    fb_set_pixel(&fb, 0, -1, true);
    fb_set_pixel(&fb, FB_WIDTH, 0, true);
    fb_set_pixel(&fb, 0, FB_HEIGHT, true);
    for (size_t i = 0; i < sizeof fb.buf; i++) {
        TEST_ASSERT_EQUAL_HEX8(0, fb.buf[i]);
    }
    TEST_ASSERT_FALSE(fb_get_pixel(&fb, FB_WIDTH, FB_HEIGHT));
}

static void test_draw_char_doubles_glyph_rows(void)
{
    fb_clear(&fb);
    fb_draw_char(&fb, 8, 16, 'A');
    assert_glyph_at(8, 16, 'A');
}

static void test_draw_char_replaces_non_ascii(void)
{
    fb_clear(&fb);
    fb_draw_char(&fb, 0, 0, '\n');
    assert_glyph_at(0, 0, '?');
}

static void test_draw_text_advances_and_clips(void)
{
    fb_clear(&fb);
    fb_draw_text(&fb, 0, 0, "ABCDEFGHIJKLMNOPQRSTU");   /* 21 символ, вміщується 16 */
    assert_glyph_at(0, 0, 'A');
    assert_glyph_at(15 * FB_CHAR_W, 0, 'P');
}

static void test_invert_rect(void)
{
    fb_clear(&fb);
    fb_invert_rect(&fb, 0, 16, FB_WIDTH, 16);
    TEST_ASSERT_TRUE(fb_get_pixel(&fb, 0, 16));
    TEST_ASSERT_TRUE(fb_get_pixel(&fb, 127, 31));
    TEST_ASSERT_FALSE(fb_get_pixel(&fb, 127, 15));
    TEST_ASSERT_FALSE(fb_get_pixel(&fb, 0, 32));
    fb_invert_rect(&fb, 0, 16, FB_WIDTH, 16);
    TEST_ASSERT_FALSE(fb_get_pixel(&fb, 0, 16));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_clear_zeroes_buffer);
    RUN_TEST(test_pixel_uses_ssd1306_page_layout);
    RUN_TEST(test_out_of_bounds_is_ignored);
    RUN_TEST(test_draw_char_doubles_glyph_rows);
    RUN_TEST(test_draw_char_replaces_non_ascii);
    RUN_TEST(test_draw_text_advances_and_clips);
    RUN_TEST(test_invert_rect);
    return UNITY_END();
}
```

- [ ] **Step 3: Переконатися, що тест падає**

Run: `bash test/host/run_tests.sh test_fb`
Expected: FAIL — `fb.h: No such file or directory`.

- [ ] **Step 4: Реалізація**

`main/fb.h`:
```c
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define FB_WIDTH    128
#define FB_HEIGHT   64
#define FB_CHAR_W   8
#define FB_CHAR_H   16   /* гліф 8x8, подвоєний по вертикалі */

/* Монохромний кадр у форматі пам'яті SSD1306: байт = стовпчик із 8 пікселів
 * однієї сторінки (8 рядків), біт 0 - верхній. buf[page * FB_WIDTH + x]. */
typedef struct {
    uint8_t buf[FB_WIDTH * FB_HEIGHT / 8];
} fb_t;

void fb_clear(fb_t *fb);
void fb_set_pixel(fb_t *fb, int x, int y, bool on);
bool fb_get_pixel(const fb_t *fb, int x, int y);

/* Символ 8x16 з лівим верхнім кутом (x, y). Фон клітинки стирається. */
void fb_draw_char(fb_t *fb, int x, int y, char c);
void fb_draw_text(fb_t *fb, int x, int y, const char *s);

void fb_invert_rect(fb_t *fb, int x, int y, int w, int h);
```

`main/fb.c`:
```c
#include "fb.h"

#include <string.h>

#include "font8x8_basic.h"

static bool in_bounds(int x, int y)
{
    return x >= 0 && x < FB_WIDTH && y >= 0 && y < FB_HEIGHT;
}

void fb_clear(fb_t *fb)
{
    memset(fb->buf, 0, sizeof fb->buf);
}

void fb_set_pixel(fb_t *fb, int x, int y, bool on)
{
    if (!in_bounds(x, y)) {
        return;
    }
    uint8_t *byte = &fb->buf[(y / 8) * FB_WIDTH + x];
    uint8_t mask = (uint8_t)(1u << (y % 8));
    if (on) {
        *byte |= mask;
    } else {
        *byte &= (uint8_t)~mask;
    }
}

bool fb_get_pixel(const fb_t *fb, int x, int y)
{
    if (!in_bounds(x, y)) {
        return false;
    }
    return (fb->buf[(y / 8) * FB_WIDTH + x] >> (y % 8)) & 1;
}

void fb_draw_char(fb_t *fb, int x, int y, char c)
{
    unsigned char ch = (unsigned char)c;
    if (ch < 0x20 || ch > 0x7E) {
        ch = '?';
    }
    for (int row = 0; row < 8; row++) {
        uint8_t bits = font8x8_basic[ch][row];
        for (int col = 0; col < 8; col++) {
            bool on = (bits >> col) & 1;   /* біт 0 - крайній лівий піксель */
            fb_set_pixel(fb, x + col, y + 2 * row, on);
            fb_set_pixel(fb, x + col, y + 2 * row + 1, on);
        }
    }
}

void fb_draw_text(fb_t *fb, int x, int y, const char *s)
{
    for (; *s != '\0' && x < FB_WIDTH; s++, x += FB_CHAR_W) {
        fb_draw_char(fb, x, y, *s);
    }
}

void fb_invert_rect(fb_t *fb, int x, int y, int w, int h)
{
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            fb_set_pixel(fb, xx, yy, !fb_get_pixel(fb, xx, yy));
        }
    }
}
```

- [ ] **Step 5: Тести проходять**

Run: `bash test/host/run_tests.sh`
Expected: усі файли `OK`, `test_fb` — 7 Tests 0 Failures.

- [ ] **Step 6: Commit**

```bash
git add main/font8x8_basic.h main/fb.h main/fb.c test/host/test_fb.c
git commit -m "feat: add monochrome framebuffer with 8x16 text rendering"
```

---

### Task 6: Тексти та малювання екранів

**Files:**
- Create: `main/ui_text.h`, `main/ui_text.c`, `main/ui.h`, `main/ui.c`
- Test: `test/host/test_ui.c`

**Interfaces:**
- Consumes: `rtc_datetime_t` (Task 2), `ui_state_t`, `SCREEN_*`, `MENU_*` (Task 4), `fb_*`, `FB_*` (Task 5).
- Produces:
  ```c
  /* ui_text.h */
  #define UI_LINE_LEN 16
  #define UI_STATUS_PORTION_COL 8
  #define UI_COUNTDOWN_MAX 99999
  void    ui_fmt_status(char out[UI_LINE_LEN + 1], const rtc_datetime_t *time, uint8_t portion, int32_t countdown_sec);
  void    ui_fmt_portion_scale(char out[UI_LINE_LEN + 1]);
  int     ui_portion_digit_col(uint8_t level);
  void    ui_fmt_auto_option(char out[UI_LINE_LEN + 1], bool option_enabled, bool is_current);
  int32_t ui_countdown_sec(int64_t deadline_us, int64_t now_us);
  /* ui.h */
  typedef struct { ui_state_t st; bool time_valid; rtc_datetime_t time; int32_t countdown_sec; } ui_view_t;
  #define UI_ROW_Y(row) ((row) * FB_CHAR_H)
  void ui_render(fb_t *fb, const ui_view_t *v);
  ```

- [ ] **Step 1: Написати тест, що падає**

`test/host/test_ui.c`:
```c
#include <string.h>

#include "unity.h"
#include "ui.h"
#include "ui_text.h"

void setUp(void) {}
void tearDown(void) {}

static const rtc_datetime_t T1437 = { .year = 2026, .month = 9, .day = 27, .hour = 14, .minute = 37, .second = 5 };

/* ---------- ui_text ---------- */

static void test_status_full(void)
{
    char s[UI_LINE_LEN + 1];
    ui_fmt_status(s, &T1437, 3, 27);
    TEST_ASSERT_EQUAL_STRING("14:37   3    27s", s);
}

static void test_status_invalid_time(void)
{
    char s[UI_LINE_LEN + 1];
    ui_fmt_status(s, NULL, 3, 27);
    TEST_ASSERT_EQUAL_STRING("--:--   3    27s", s);
}

static void test_status_auto_off_leaves_right_empty(void)
{
    char s[UI_LINE_LEN + 1];
    ui_fmt_status(s, &T1437, 1, -1);
    TEST_ASSERT_EQUAL_STRING("14:37   1       ", s);
}

static void test_status_zero_countdown(void)
{
    char s[UI_LINE_LEN + 1];
    ui_fmt_status(s, &T1437, 4, 0);
    TEST_ASSERT_EQUAL_STRING("14:37   4     0s", s);
}

static void test_status_clamps_huge_countdown(void)
{
    char s[UI_LINE_LEN + 1];
    ui_fmt_status(s, &T1437, 2, 123456);
    TEST_ASSERT_EQUAL_STRING("14:37   2 99999s", s);
    TEST_ASSERT_EQUAL_size_t(UI_LINE_LEN, strlen(s));
}

static void test_status_pads_single_digit_time(void)
{
    const rtc_datetime_t t = { .year = 2026, .month = 1, .day = 1, .hour = 7, .minute = 5, .second = 0 };
    char s[UI_LINE_LEN + 1];
    ui_fmt_status(s, &t, 1, -1);
    TEST_ASSERT_EQUAL_STRING("07:05   1       ", s);
}

static void test_portion_scale(void)
{
    char s[UI_LINE_LEN + 1];
    ui_fmt_portion_scale(s);
    TEST_ASSERT_EQUAL_STRING("  1   2   3   4 ", s);
    for (uint8_t level = 1; level <= 4; level++) {
        TEST_ASSERT_EQUAL_CHAR('0' + level, s[ui_portion_digit_col(level)]);
    }
}

static void test_auto_options(void)
{
    char s[UI_LINE_LEN + 1];
    ui_fmt_auto_option(s, true, true);
    TEST_ASSERT_EQUAL_STRING("* Enabled       ", s);
    ui_fmt_auto_option(s, false, false);
    TEST_ASSERT_EQUAL_STRING("  Disabled      ", s);
    ui_fmt_auto_option(s, false, true);
    TEST_ASSERT_EQUAL_STRING("* Disabled      ", s);
}

static void test_countdown_sec_rounds_up(void)
{
    TEST_ASSERT_EQUAL_INT32(27, ui_countdown_sec(26200000, 0));
    TEST_ASSERT_EQUAL_INT32(27, ui_countdown_sec(27000000, 0));
    TEST_ASSERT_EQUAL_INT32(1, ui_countdown_sec(1000001, 1000000));
}

static void test_countdown_sec_past_deadline_is_zero(void)
{
    TEST_ASSERT_EQUAL_INT32(0, ui_countdown_sec(5000000, 5000000));
    TEST_ASSERT_EQUAL_INT32(0, ui_countdown_sec(5000000, 9000000));
}

/* ---------- ui_render: порівняння з еталонним кадром ---------- */

static fb_t actual, expected;

static ui_view_t view(ui_screen_t screen, uint8_t cursor)
{
    ui_view_t v;
    memset(&v, 0, sizeof v);
    v.st.screen = screen;
    v.st.cursor = cursor;
    v.st.portion = 3;
    v.st.preview = 3;
    v.time_valid = true;
    v.time = T1437;
    v.countdown_sec = 27;
    return v;
}

static void assert_frames_equal(void)
{
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected.buf, actual.buf, sizeof expected.buf);
}

static void test_render_main(void)
{
    ui_view_t v = view(SCREEN_MAIN, 0);
    ui_render(&actual, &v);
    fb_clear(&expected);
    fb_draw_text(&expected, 0, UI_ROW_Y(1), "14:37   3    27s");
    assert_frames_equal();
}

static void test_render_main_invalid_time(void)
{
    ui_view_t v = view(SCREEN_MAIN, 0);
    v.time_valid = false;
    ui_render(&actual, &v);
    fb_clear(&expected);
    fb_draw_text(&expected, 0, UI_ROW_Y(1), "--:--   3    27s");
    assert_frames_equal();
}

static void test_render_root_cursor_on_auto(void)
{
    ui_view_t v = view(SCREEN_MENU_ROOT, MENU_ROOT_AUTO);
    ui_render(&actual, &v);
    fb_clear(&expected);
    fb_draw_text(&expected, 0, UI_ROW_Y(0), "MENU");
    fb_draw_text(&expected, 0, UI_ROW_Y(1), " Portion");
    fb_draw_text(&expected, 0, UI_ROW_Y(2), " Auto Mode");
    fb_invert_rect(&expected, 0, UI_ROW_Y(2), FB_WIDTH, FB_CHAR_H);
    assert_frames_equal();
}

static void test_render_portion_highlights_preview(void)
{
    ui_view_t v = view(SCREEN_MENU_PORTION, 0);
    v.st.preview = 4;
    ui_render(&actual, &v);
    fb_clear(&expected);
    fb_draw_text(&expected, 0, UI_ROW_Y(0), "Portion");
    fb_draw_text(&expected, 0, UI_ROW_Y(2), "  1   2   3   4 ");
    fb_invert_rect(&expected, 13 * FB_CHAR_W, UI_ROW_Y(2), 3 * FB_CHAR_W, FB_CHAR_H);
    assert_frames_equal();
}

static void test_render_auto_marks_current_and_cursor(void)
{
    ui_view_t v = view(SCREEN_MENU_AUTO, MENU_AUTO_ENABLED);
    v.st.auto_enabled = false;   /* курсор на Enabled, але фактично вимкнено */
    ui_render(&actual, &v);
    fb_clear(&expected);
    fb_draw_text(&expected, 0, UI_ROW_Y(0), "Auto Mode");
    fb_draw_text(&expected, 0, UI_ROW_Y(1), "  Enabled       ");
    fb_draw_text(&expected, 0, UI_ROW_Y(2), "* Disabled      ");
    fb_invert_rect(&expected, 0, UI_ROW_Y(1), FB_WIDTH, FB_CHAR_H);
    assert_frames_equal();
}

static void test_render_feeding_overrides_screen(void)
{
    ui_view_t v = view(SCREEN_MENU_PORTION, 0);
    v.st.feeding = true;
    ui_render(&actual, &v);
    fb_clear(&expected);
    fb_draw_text(&expected, 0, UI_ROW_Y(0), "14:37   3    27s");
    fb_draw_text(&expected, 0, UI_ROW_Y(2), "   FEEDING...");
    assert_frames_equal();
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_status_full);
    RUN_TEST(test_status_invalid_time);
    RUN_TEST(test_status_auto_off_leaves_right_empty);
    RUN_TEST(test_status_zero_countdown);
    RUN_TEST(test_status_clamps_huge_countdown);
    RUN_TEST(test_status_pads_single_digit_time);
    RUN_TEST(test_portion_scale);
    RUN_TEST(test_auto_options);
    RUN_TEST(test_countdown_sec_rounds_up);
    RUN_TEST(test_countdown_sec_past_deadline_is_zero);
    RUN_TEST(test_render_main);
    RUN_TEST(test_render_main_invalid_time);
    RUN_TEST(test_render_root_cursor_on_auto);
    RUN_TEST(test_render_portion_highlights_preview);
    RUN_TEST(test_render_auto_marks_current_and_cursor);
    RUN_TEST(test_render_feeding_overrides_screen);
    return UNITY_END();
}
```

- [ ] **Step 2: Переконатися, що тест падає**

Run: `bash test/host/run_tests.sh test_ui`
Expected: FAIL — `ui.h: No such file or directory`.

- [ ] **Step 3: Реалізація `ui_text`**

`main/ui_text.h`:
```c
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rtc_time.h"

#define UI_LINE_LEN             16      /* символів у рядку: 128 px / 8 px */
#define UI_STATUS_PORTION_COL   8       /* колонка цифри порції в рядку стану */
#define UI_COUNTDOWN_MAX        99999   /* "99999s" - 6 символів праворуч */

/* Рядок стану: "HH:MM" або "--:--" | порція | "<N>s" вирівняно праворуч.
 * time == NULL -> "--:--"; countdown_sec < 0 -> права частина порожня. */
void ui_fmt_status(char out[UI_LINE_LEN + 1], const rtc_datetime_t *time,
                   uint8_t portion, int32_t countdown_sec);

/* Шкала меню Portion "  1   2   3   4 " і колонка цифри рівня в ній. */
void ui_fmt_portion_scale(char out[UI_LINE_LEN + 1]);
int  ui_portion_digit_col(uint8_t level);

/* "* Enabled" / "  Disabled" (доповнено пробілами): '*' - фактичний поточний режим. */
void ui_fmt_auto_option(char out[UI_LINE_LEN + 1], bool option_enabled, bool is_current);

/* Секунд до дедлайну з округленням вгору; 0, якщо дедлайн уже настав. */
int32_t ui_countdown_sec(int64_t deadline_us, int64_t now_us);
```

`main/ui_text.c`:
```c
#include "ui_text.h"

#include <stdio.h>
#include <string.h>

static void blank(char out[UI_LINE_LEN + 1])
{
    memset(out, ' ', UI_LINE_LEN);
    out[UI_LINE_LEN] = '\0';
}

void ui_fmt_status(char out[UI_LINE_LEN + 1], const rtc_datetime_t *time,
                   uint8_t portion, int32_t countdown_sec)
{
    char buf[16];

    blank(out);

    if (time != NULL) {
        snprintf(buf, sizeof buf, "%02u:%02u", (unsigned)time->hour, (unsigned)time->minute);
    } else {
        strcpy(buf, "--:--");
    }
    memcpy(out, buf, 5);

    out[UI_STATUS_PORTION_COL] = (char)('0' + portion);

    if (countdown_sec >= 0) {
        if (countdown_sec > UI_COUNTDOWN_MAX) {
            countdown_sec = UI_COUNTDOWN_MAX;
        }
        int n = snprintf(buf, sizeof buf, "%lds", (long)countdown_sec);
        memcpy(out + UI_LINE_LEN - n, buf, (size_t)n);
    }
}

void ui_fmt_portion_scale(char out[UI_LINE_LEN + 1])
{
    blank(out);
    for (uint8_t level = 1; level <= 4; level++) {
        out[ui_portion_digit_col(level)] = (char)('0' + level);
    }
}

int ui_portion_digit_col(uint8_t level)
{
    /* Клітинки по 4 символи: цифра по центру клітинки " n ". */
    return 4 * level - 2;
}

void ui_fmt_auto_option(char out[UI_LINE_LEN + 1], bool option_enabled, bool is_current)
{
    const char *label = option_enabled ? "Enabled" : "Disabled";

    blank(out);
    out[0] = is_current ? '*' : ' ';
    memcpy(out + 2, label, strlen(label));
}

int32_t ui_countdown_sec(int64_t deadline_us, int64_t now_us)
{
    int64_t left = deadline_us - now_us;
    if (left <= 0) {
        return 0;
    }
    int64_t sec = (left + 999999) / 1000000;
    return sec > INT32_MAX ? INT32_MAX : (int32_t)sec;
}
```

- [ ] **Step 4: Реалізація `ui`**

`main/ui.h`:
```c
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "fb.h"
#include "menu.h"
#include "rtc_time.h"

/* Усе, що потрібно для одного кадру. */
typedef struct {
    ui_state_t     st;
    bool           time_valid;
    rtc_datetime_t time;
    int32_t        countdown_sec;   /* < 0 -> AUTO вимкнено, праворуч порожньо */
} ui_view_t;

#define UI_ROW_Y(row)   ((row) * FB_CHAR_H)

/* Малює кадр з нуля (спек, розділ 4). Заліза не торкається. */
void ui_render(fb_t *fb, const ui_view_t *v);
```

`main/ui.c`:
```c
#include "ui.h"

#include "ui_text.h"

static void draw_row(fb_t *fb, int row, const char *text)
{
    fb_draw_text(fb, 0, UI_ROW_Y(row), text);
}

static void invert_row(fb_t *fb, int row)
{
    fb_invert_rect(fb, 0, UI_ROW_Y(row), FB_WIDTH, FB_CHAR_H);
}

void ui_render(fb_t *fb, const ui_view_t *v)
{
    const ui_state_t *st = &v->st;
    char line[UI_LINE_LEN + 1];

    fb_clear(fb);
    ui_fmt_status(line, v->time_valid ? &v->time : NULL, st->portion, v->countdown_sec);

    /* Під час насипання екран меню ховається, але стан меню зберігається
     * і повертається після циклу (спек 4.5). */
    if (st->feeding) {
        draw_row(fb, 0, line);
        draw_row(fb, 2, "   FEEDING...");
        return;
    }

    switch (st->screen) {
    case SCREEN_MAIN:
        draw_row(fb, 1, line);
        break;

    case SCREEN_MENU_ROOT:
        draw_row(fb, 0, "MENU");
        draw_row(fb, 1, " Portion");
        draw_row(fb, 2, " Auto Mode");
        invert_row(fb, 1 + st->cursor);
        break;

    case SCREEN_MENU_PORTION: {
        draw_row(fb, 0, "Portion");
        ui_fmt_portion_scale(line);
        draw_row(fb, 2, line);
        int col = ui_portion_digit_col(st->preview);
        fb_invert_rect(fb, (col - 1) * FB_CHAR_W, UI_ROW_Y(2), 3 * FB_CHAR_W, FB_CHAR_H);
        break;
    }

    case SCREEN_MENU_AUTO:
        draw_row(fb, 0, "Auto Mode");
        ui_fmt_auto_option(line, true, st->auto_enabled);
        draw_row(fb, 1, line);
        ui_fmt_auto_option(line, false, !st->auto_enabled);
        draw_row(fb, 2, line);
        invert_row(fb, 1 + st->cursor);
        break;
    }
}
```

- [ ] **Step 5: Тести проходять**

Run: `bash test/host/run_tests.sh`
Expected: усі файли `OK`, `test_ui` — 16 Tests 0 Failures.

- [ ] **Step 6: Commit**

```bash
git add main/ui_text.h main/ui_text.c main/ui.h main/ui.c test/host/test_ui.c
git commit -m "feat: add status/menu text formatting and screen rendering"
```

---

### Task 7: Шина I2C, мітка збірки, драйвер DS1307

**Files:**
- Create: `main/gen_build_time.cmake`, `main/i2c_bus.h`, `main/i2c_bus.c`, `main/rtc_ds1307.h`, `main/rtc_ds1307.c`
- Modify: `main/CMakeLists.txt`, `main/config.h`, `main/settings.h`, `main/settings.c`, `main/main.c`

**Interfaces:**
- Consumes: `rtc_datetime_t`, `rtc_datetime_add_seconds` (Task 2); `ds1307_*`, `rtc_startup_decide`, `ds1307_marker` (Task 3).
- Produces:
  ```c
  /* i2c_bus.h */
  void i2c_bus_init(void);                      /* ESP_ERROR_CHECK при помилці конфігурації */
  i2c_master_bus_handle_t i2c_bus_get(void);
  void i2c_bus_scan(void);                      /* адреси в лог */
  /* rtc_ds1307.h */
  void rtc_init(void);                          /* після settings_init() та i2c_bus_init() */
  bool rtc_read(rtc_datetime_t *out);           /* false: невалідний час або помилка читання */
  /* settings.h */
  bool settings_get_build_stamp(char *out, size_t out_len);
  void settings_set_build_stamp(const char *stamp);
  /* config.h */
  I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ, I2C_TIMEOUT_MS, RTC_ADDR, OLED_ADDR, RTC_BUILD_OFFSET_SEC
  /* build_time.h (генерований) */
  BUILD_YEAR, BUILD_MONTH, BUILD_DAY, BUILD_HOUR, BUILD_MIN, BUILD_SEC, BUILD_STAMP
  ```

- [ ] **Step 1: Генератор `build_time.h`**

`main/gen_build_time.cmake`:
```cmake
# Викликається на КОЖНУ збірку (custom target у main/CMakeLists.txt) і пише
# build_time.h з поточним локальним часом ПК. Використання:
#   cmake -DOUT=<шлях до build_time.h> -P gen_build_time.cmake

# Одна мітка на все, щоб поля не "розʼїхались" на межі секунди.
string(TIMESTAMP STAMP "%Y%m%d%H%M%S")

string(SUBSTRING "${STAMP}" 0 4 Y)
string(SUBSTRING "${STAMP}" 4 2 MO)
string(SUBSTRING "${STAMP}" 6 2 D)
string(SUBSTRING "${STAMP}" 8 2 H)
string(SUBSTRING "${STAMP}" 10 2 MI)
string(SUBSTRING "${STAMP}" 12 2 S)

# "09" у C - некоректний вісімковий літерал, тому провідний нуль прибираємо.
foreach(var MO D H MI S)
    string(REGEX REPLACE "^0([0-9])$" "\\1" ${var} "${${var}}")
endforeach()

file(WRITE "${OUT}"
"#pragma once
/* Згенеровано main/gen_build_time.cmake на кожну збірку. Не редагувати. */
#define BUILD_YEAR   ${Y}
#define BUILD_MONTH  ${MO}
#define BUILD_DAY    ${D}
#define BUILD_HOUR   ${H}
#define BUILD_MIN    ${MI}
#define BUILD_SEC    ${S}
#define BUILD_STAMP  \"${STAMP}\"
")
```

- [ ] **Step 2: `main/CMakeLists.txt`**

Повний вміст:
```cmake
idf_component_register(
    SRCS
        "main.c"
        "encoder.c"
        "buttons.c"
        "leds.c"
        "buzzer.c"
        "servo.c"
        "motor.c"
        "feeder.c"
        "settings.c"
        "i2c_bus.c"
        "rtc_time.c"
        "ds1307_codec.c"
        "rtc_ds1307.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_timer esp_driver_gpio esp_driver_ledc esp_driver_i2c
)

# Мітка часу збірки для RTC (спек, розділ 5.1). Custom target завжди
# вважається застарілим, тож заголовок оновлюється на кожну збірку -
# на відміну від __DATE__/__TIME__, що змінюються лише при перекомпіляції файлу.
set(BUILD_TIME_H "${CMAKE_CURRENT_BINARY_DIR}/build_time.h")
add_custom_target(gen_build_time ALL
    COMMAND ${CMAKE_COMMAND} -DOUT=${BUILD_TIME_H} -P ${CMAKE_CURRENT_SOURCE_DIR}/gen_build_time.cmake
    BYPRODUCTS ${BUILD_TIME_H}
    COMMENT "Generating build_time.h"
)
add_dependencies(${COMPONENT_LIB} gen_build_time)
target_include_directories(${COMPONENT_LIB} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
```

- [ ] **Step 3: `main/config.h` — нові константи**

Після блоку «DC-мотор» додати:
```c
/* ---------- Шина I2C: RTC DS1307 + OLED SSD1306 ----------
 * GPIO46 не використовується: strapping-пін режиму завантаження,
 * підтяжка до 1 заважає входу в режим прошивки. */
#define I2C_SDA_PIN         GPIO_NUM_18
#define I2C_SCL_PIN         GPIO_NUM_3
#define I2C_FREQ_HZ         100000
#define I2C_TIMEOUT_MS      50

#define RTC_ADDR            0x68
#define OLED_ADDR           0x3C

/* Час між збіркою і стартом плати (прошивка + завантаження), додається
 * до часу збірки при встановленні RTC. */
#define RTC_BUILD_OFFSET_SEC    30
```

- [ ] **Step 4: `settings` — мітка збірки**

У `main/settings.h` додати `#include <stddef.h>` після `#include <stdbool.h>` і в кінець:
```c
/* Мітка збірки (BUILD_STAMP), при якій востаннє встановлювався час RTC.
 * false, якщо мітки ще немає або NVS недоступний. */
bool settings_get_build_stamp(char *out, size_t out_len);
void settings_set_build_stamp(const char *stamp);
```

У `main/settings.c` після `#define KEY_AUTO        "auto_on"` додати:
```c
#define KEY_BUILD_STAMP "build_stamp"
```
і в кінець файлу:
```c
bool settings_get_build_stamp(char *out, size_t out_len)
{
    if (!nvs_ready) {
        return false;
    }
    size_t len = out_len;
    return nvs_get_str(handle, KEY_BUILD_STAMP, out, &len) == ESP_OK;
}

void settings_set_build_stamp(const char *stamp)
{
    if (!nvs_ready) {
        return;
    }
    esp_err_t err = nvs_set_str(handle, KEY_BUILD_STAMP, stamp);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "запис %s не вдався: %s", KEY_BUILD_STAMP, esp_err_to_name(err));
    }
}
```

- [ ] **Step 5: `i2c_bus`**

`main/i2c_bus.h`:
```c
#pragma once

#include "driver/i2c_master.h"

/* Одна шина на RTC і OLED. Помилка конфігурації - баг прошивки, тому abort. */
void i2c_bus_init(void);
i2c_master_bus_handle_t i2c_bus_get(void);

/* Опитує всі 7-бітні адреси і пише знайдені в лог (діагностика підключення). */
void i2c_bus_scan(void);
```

`main/i2c_bus.c`:
```c
#include "i2c_bus.h"

#include "esp_err.h"
#include "esp_log.h"

#include "config.h"

static const char *TAG = "i2c_bus";

static i2c_master_bus_handle_t bus = NULL;

void i2c_bus_init(void)
{
    i2c_master_bus_config_t conf = {
        .i2c_port = -1,   /* будь-який вільний контролер */
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        /* Основні підтяжки стоять на модулях; внутрішні (до 3.3 В) - підстраховка. */
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
```

- [ ] **Step 6: `rtc_ds1307`**

`main/rtc_ds1307.h`:
```c
#pragma once

#include <stdbool.h>

#include "rtc_time.h"

/* Алгоритм старту зі спеку 5.3: за потреби встановлює час зі збірки.
 * Викликати один раз після settings_init() та i2c_bus_init(). */
void rtc_init(void);

/* Поточний час. false - час невалідний зі старту або читання не вдалося. */
bool rtc_read(rtc_datetime_t *out);
```

`main/rtc_ds1307.c`:
```c
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

void rtc_init(void)
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

bool rtc_read(rtc_datetime_t *out)
{
    if (!time_valid) {
        return false;
    }
    uint8_t regs[DS1307_TIME_REG_COUNT];
    return read_regs(0x00, regs, sizeof regs) && ds1307_decode(regs, out);
}
```

- [ ] **Step 7: `main/main.c` — ініціалізація шини і RTC**

Додати інклуди після `#include "settings.h"`:
```c
#include "i2c_bus.h"
#include "rtc_ds1307.h"
```
Замінити коментар `/* NVS - першим: feeder_task одразу читає з нього відновлений стан. */` на:
```c
    /* NVS - першим: з нього відновлюється стан і мітка збірки для RTC. */
```
Після `motor_init();` додати:
```c

    /* I2C: скан у лог для діагностики, потім алгоритм старту RTC.
     * Після app_main шиною користується лише display_task. */
    i2c_bus_init();
    i2c_bus_scan();
    rtc_init();
```

- [ ] **Step 8: Збірка і перевірка мітки**

Run (PowerShell):
```powershell
. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1 *> $null; idf.py build; Get-Content build\esp-idf\main\build_time.h
```
Expected: `Project build complete.`; у `build_time.h` поточна дата/час ПК, `BUILD_MONTH` без провідного нуля.

Повторити `idf.py build` через ≥2 с і знову `Get-Content build\esp-idf\main\build_time.h`.
Expected: `BUILD_STAMP` змінився (мітка оновлюється на кожну збірку).

Run: `bash test/host/run_tests.sh` — Expected: усі `OK`.

- [ ] **Step 9: Commit**

```bash
git add main/gen_build_time.cmake main/CMakeLists.txt main/config.h main/settings.h main/settings.c \
        main/i2c_bus.h main/i2c_bus.c main/rtc_ds1307.h main/rtc_ds1307.c main/main.c
git commit -m "feat: add I2C bus, per-build timestamp and DS1307 driver"
```

---

### Task 8: OLED, спільний стан, відлік і `display_task`

**Files:**
- Create: `main/oled.h`, `main/oled.c`, `main/ui_state.h`, `main/ui_state.c`, `main/display.h`, `main/display.c`
- Modify: `main/feeder.h`, `main/feeder.c` (автотаймер з дедлайном), `main/config.h`, `main/CMakeLists.txt`, `main/main.c`

**Interfaces:**
- Consumes: `i2c_bus_get()` (Task 7), `rtc_read()` (Task 7), `ui_render`, `ui_view_t` (Task 6), `ui_countdown_sec` (Task 6), `fb_t` (Task 5), `menu_init`, `ui_state_t` (Task 4).
- Produces:
  ```c
  /* oled.h */
  bool oled_init(void);                 /* false - дисплей не відповідає */
  bool oled_flush(const fb_t *fb);
  /* ui_state.h */
  void ui_state_init(const ui_state_t *initial);
  void ui_state_publish(const ui_state_t *st);
  void ui_state_get(ui_state_t *out);
  /* display.h */
  void display_task(void *arg);
  /* feeder.h */
  bool auto_timer_remaining_sec(int32_t *sec);   /* false - таймер не запущено */
  /* config.h */
  DISPLAY_PERIOD_MS
  ```

- [ ] **Step 1: `oled`**

`main/oled.h`:
```c
#pragma once

#include <stdbool.h>

#include "fb.h"

/* SSD1306 128x64 через esp_lcd на спільній шині I2C.
 * false - дисплей не відповідає; годівничка тоді працює без екрана. */
bool oled_init(void);

/* Надсилає весь кадр. false - помилка I2C (кадр просто пропускається). */
bool oled_flush(const fb_t *fb);
```

`main/oled.c`:
```c
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
        .control_phase_bytes = 1,   /* за даташитом SSD1306 */
        .dc_bit_offset = 6,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    esp_lcd_panel_ssd1306_config_t ssd_conf = {
        .height = FB_HEIGHT,
    };
    esp_lcd_panel_dev_config_t panel_conf = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,   /* RST модуля не виведено */
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
```

- [ ] **Step 2: `ui_state`**

`main/ui_state.h`:
```c
#pragma once

#include "menu.h"

/* Знімок стану між feeder_task (пише) і display_task (читає), під м'ютексом. */
void ui_state_init(const ui_state_t *initial);
void ui_state_publish(const ui_state_t *st);
void ui_state_get(ui_state_t *out);
```

`main/ui_state.c`:
```c
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
```

- [ ] **Step 3: Автотаймер з дедлайном у `feeder.c`**

У `main/feeder.h` додати після `void auto_timer_stop(void);`:
```c

/* Секунд до наступної автоподачі (округлено вгору).
 * false - таймер не запущено (AUTO вимкнено). Безпечно з будь-якої задачі. */
bool auto_timer_remaining_sec(int32_t *sec);
```
і `#include <stdbool.h>` поруч з `#include <stdint.h>`.

У `main/feeder.c` додати `#include "ui_text.h"` після `#include "settings.h"` і замінити блок від `static esp_timer_handle_t auto_timer = NULL;` до кінця функції `auto_timer_stop()` на:
```c
/* Таймер створюється один раз і лише запускається/зупиняється: display_task
 * читає дедлайн паралельно, тож хендл не можна видаляти під ним. */
static esp_timer_handle_t auto_timer = NULL;
static int64_t auto_interval_us = 0;

/* Час наступного спрацювання; 0 = таймер зупинено. esp_timer_get_expiry_time()
 * не підтримує періодичні таймери, тому дедлайн ведемо самі. */
static int64_t next_deadline_us = 0;
static portMUX_TYPE deadline_lock = portMUX_INITIALIZER_UNLOCKED;

static void auto_timer_callback(void *arg)
{
    taskENTER_CRITICAL(&deadline_lock);
    /* Колбек міг уже стартувати, коли таймер зупинили, - тоді дедлайн не оживляємо. */
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
        return; /* вже запущений */
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
```

- [ ] **Step 4: `display_task`**

У `main/config.h` після блоку I2C додати:
```c
/* ---------- Екран ---------- */
#define DISPLAY_PERIOD_MS       200    /* період перемальовування */
#define DISPLAY_RTC_PERIOD_MS   1000   /* як часто читати RTC */
```

`main/display.h`:
```c
#pragma once

/* Єдина задача, що працює з I2C після старту: читає RTC, рахує відлік
 * і малює кадр зі знімка стану. Не залежить від блокуючого feed_cycle(). */
void display_task(void *arg);
```

`main/display.c`:
```c
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
    static fb_t fb;   /* 1 КБ - не на стеку задачі */
    ui_view_t view = { 0 };
    TickType_t last_rtc_read = 0;
    bool rtc_read_once = false;

    if (!oled_init()) {
        vTaskDelete(NULL);   /* без екрана годівничка працює далі (спек 6) */
        return;
    }

    while (1) {
        ui_state_get(&view.st);

        TickType_t now = xTaskGetTickCount();
        if (!rtc_read_once || now - last_rtc_read >= pdMS_TO_TICKS(DISPLAY_RTC_PERIOD_MS)) {
            view.time_valid = rtc_read(&view.time);
            last_rtc_read = now;
            rtc_read_once = true;
        }

        if (!auto_timer_remaining_sec(&view.countdown_sec)) {
            view.countdown_sec = -1;
        }

        ui_render(&fb, &view);
        oled_flush(&fb);   /* помилка - пропускаємо кадр, наступний спробує знову */

        vTaskDelay(pdMS_TO_TICKS(DISPLAY_PERIOD_MS));
    }
}
```

- [ ] **Step 5: `main/CMakeLists.txt` і `main/main.c`**

У `SRCS` після `"rtc_ds1307.c"` додати:
```cmake
        "menu.c"
        "fb.c"
        "ui_text.c"
        "ui.c"
        "ui_state.c"
        "oled.c"
        "display.c"
```
У `REQUIRES` дописати `esp_lcd`:
```cmake
    REQUIRES nvs_flash esp_timer esp_driver_gpio esp_driver_ledc esp_driver_i2c esp_lcd
```

У `main/main.c` додати інклуди після `#include "rtc_ds1307.h"`:
```c
#include "menu.h"
#include "ui_state.h"
#include "display.h"
```
Після `rtc_init();` додати:
```c

    /* Початковий стан меню з NVS - до старту задач, що його читають. */
    ui_state_t initial;
    menu_init(&initial, settings_get_portion(), settings_get_auto_enabled());
    ui_state_init(&initial);
```
Після `xTaskCreate(feeder_task, ...)` додати:
```c
    xTaskCreate(display_task, "display_task", 4096, NULL, 4, NULL);
```

- [ ] **Step 6: Збірка і тести**

Run (PowerShell): `. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1 *> $null; idf.py build`
Expected: `Project build complete.`

Run: `bash test/host/run_tests.sh` — Expected: усі `OK`.

- [ ] **Step 7: Commit**

```bash
git add main/oled.h main/oled.c main/ui_state.h main/ui_state.c main/display.h main/display.c \
        main/feeder.h main/feeder.c main/config.h main/CMakeLists.txt main/main.c
git commit -m "feat: add OLED output, shared UI state and display task"
```

---

### Task 9: Кнопки, події і `feeder_task` на меню

**Files:**
- Modify: `main/config.h`, `main/feeder_types.h`, `main/buttons.h`, `main/buttons.c`, `main/feeder.h`, `main/feeder.c`

**Interfaces:**
- Consumes: `menu_handle`, `menu_action_t`, `ACT_*`, `SCREEN_MENU_PORTION` (Task 4); `ui_state_get/publish` (Task 8); `auto_timer_start/stop` (Task 8).
- Produces:
  ```c
  /* buttons.h */
  bool button_manual_pressed(void);
  bool button_back_pressed(void);
  bool button_enc_click_pressed(void);
  /* feeder_types.h: EVT_BTN_AUTO_TOGGLE прибрано */
  typedef enum { EVT_ENCODER_DELTA, EVT_BTN_MANUAL, EVT_AUTO_TIMER, EVT_ENC_CLICK, EVT_BTN_BACK } event_type_t;
  ```

- [ ] **Step 1: `config.h`**

Замінити:
```c
#define ENCODER_SW_PIN      GPIO_NUM_6   /* зарезервовано, поки не використовується */
```
на:
```c
#define ENCODER_SW_PIN      GPIO_NUM_6   /* клік: відкрити меню / підтвердити вибір */
```
Замінити:
```c
#define BTN_AUTO_PIN        GPIO_NUM_8
```
на:
```c
#define BTN_BACK_PIN        GPIO_NUM_8   /* «Назад»: попереднє меню / вихід з меню */
```
Замінити `#define AUTO_INTERVAL_SEC 10` на:
```c
#define AUTO_INTERVAL_SEC       30
```

- [ ] **Step 2: `feeder_types.h` — прибрати застарілу подію**

Замінити enum на:
```c
typedef enum {
    EVT_ENCODER_DELTA,    /* поворот енкодера, value = +1 або -1 */
    EVT_BTN_MANUAL,       /* натиснута кнопка ручного насипання */
    EVT_AUTO_TIMER,       /* спрацював таймер авто-режиму */
    EVT_ENC_CLICK,        /* клік енкодера (GPIO6) */
    EVT_BTN_BACK,         /* кнопка «Назад» (GPIO8) */
} event_type_t;
```

- [ ] **Step 3: `buttons`**

`main/buttons.h` — повний вміст:
```c
#pragma once

#include <stdbool.h>

void buttons_init(void);

/* Повертають true рівно один раз на кожне фізичне натискання
 * (програмний дебаунс усередині), а не на весь час утримання. */
bool button_manual_pressed(void);
bool button_back_pressed(void);
bool button_enc_click_pressed(void);
```

У `main/buttons.c`:
- замінити
  ```c
  static bool manual_last_state = false;
  static bool auto_last_state = false;
  ```
  на
  ```c
  static bool manual_last_state = false;
  static bool back_last_state = false;
  static bool enc_click_last_state = false;
  ```
- у `buttons_init()` замінити `.pin_bit_mask = (1ULL << BTN_MANUAL_PIN) | (1ULL << BTN_AUTO_PIN),` на:
  ```c
          /* SW у KY-040 зазвичай без резистора на платі - потрібна внутрішня підтяжка. */
          .pin_bit_mask = (1ULL << BTN_MANUAL_PIN) | (1ULL << BTN_BACK_PIN) |
                          (1ULL << ENCODER_SW_PIN),
  ```
- замінити функцію `button_auto_pressed()` на:
  ```c
  bool button_back_pressed(void)
  {
      return debounce_check(BTN_BACK_PIN, &back_last_state);
  }

  bool button_enc_click_pressed(void)
  {
      return debounce_check(ENCODER_SW_PIN, &enc_click_last_state);
  }
  ```

- [ ] **Step 4: `feeder.h`**

Прибрати `static inline uint8_t clamp_u8(...)` (обмеження тепер у `menu.c`) і оновити коментар до `feeder_task`:
```c
/* Задача, що володіє станом (ui_state_t): передає події в menu_handle()
 * і виконує повернуті дії - LED, NVS, таймер, feed_cycle(). */
void feeder_task(void *arg);
```

- [ ] **Step 5: `feeder.c` — `input_task` і `feeder_task`**

Додати інклуди після `#include "settings.h"`:
```c
#include "menu.h"
#include "ui_state.h"
```

Замінити всю функцію `input_task` на:
```c
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
```

Замінити всю функцію `feeder_task` на:
```c
static void run_feed(ui_state_t *st)
{
    st->feeding = true;
    ui_state_publish(st);

    /* Годуємо підтвердженою порцією, і шкала показує саме її (спек 4.5). */
    update_portion_leds(st->portion);
    feed_cycle(st->portion);

    st->feeding = false;
    /* Повертаємось на той самий екран: у меню Portion - до непідтвердженого вибору. */
    update_portion_leds(st->screen == SCREEN_MENU_PORTION ? st->preview : st->portion);
    xQueueReset(event_queue); /* відкинути все, що накопичилось під час циклу */
}

void feeder_task(void *arg)
{
    feeder_event_t evt;
    ui_state_t st;

    /* Початковий стан (з NVS) заповнено в app_main до старту задач. */
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
```

- [ ] **Step 6: Перевірити, що старих імен не лишилось**

Run: `grep -rn "EVT_BTN_AUTO_TOGGLE\|button_auto_pressed\|BTN_AUTO_PIN\|clamp_u8" main/`
Expected: порожньо.

- [ ] **Step 7: Збірка і тести**

Run (PowerShell): `. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1 *> $null; idf.py build`
Expected: `Project build complete.`, без попереджень у файлах `main/`.

Run: `bash test/host/run_tests.sh` — Expected: усі `OK` (після зміни enum `test_menu` має пройти без змін).

- [ ] **Step 8: Commit**

```bash
git add main/config.h main/feeder_types.h main/buttons.h main/buttons.c main/feeder.h main/feeder.c
git commit -m "feat: drive feeder from menu with encoder click and back button"
```

---

### Task 10: README і фінальна перевірка

**Files:**
- Create: `README.md`

**Interfaces:**
- Consumes: усе вище.
- Produces: документацію для користувача.

- [ ] **Step 1: `README.md`**

Повний вміст:
````markdown
# Автоматична годівничка для кота з годинником і екраном (ESP32-S3)

Розвиток miniproject_3: та сама механіка дозування (бункер → диск з 4 отворами → приймач
із заслінкою), плюс годинник реального часу DS1307 і OLED-екран SSD1306 з меню.

## Компоненти

| Вузол | Деталь |
|---|---|
| Мікроконтролер | ESP32-S3-N16R8 |
| Годинник | модуль Tiny RTC (DS1307 + AT24C32), I2C 0x68 |
| Екран | OLED SSD1306 0.96" 128×64, I2C 0x3C |
| Керування | енкодер KY-040 (поворот + клік), кнопка «насипати зараз», кнопка «Назад» |
| Індикація порції | 4 сині LED (шкала 1–4) |
| Індикація режимів | LED AUTO (червоний), LED FEEDING (зелений) |
| Сигнал | пасивний п'єзо-зумер |
| Дозування | DC-мотор через 2N2222A + флайбек-діод |
| Заслінка | сервопривід SG90 |

## Пінаут

| Компонент | GPIO |
|---|---|
| I2C SDA / SCL (RTC + OLED) | 18 / 3 |
| KY-040 CLK / DT / SW | 4 / 5 / 6 |
| Кнопка «насипати зараз» / «Назад» | 7 / 8 |
| LED порції 1–4 | 9, 10, 11, 12 |
| LED AUTO / FEEDING | 13 / 14 |
| Зумер | 15 |
| Сервопривід SG90 | 16 |
| Транзистор мотора | 17 |

GPIO46 під I2C не використовується: це strapping-пін, і підтяжка до 1 заважає входу в режим прошивки.

## Підключення I2C і живлення RTC

OLED: VCC → 3.3 В, GND → GND, SDA → 18, SCL → 3.

На модулі Tiny RTC лінії SDA/SCL підтягнуті резисторами 4.7 кОм **до VCC модуля**.
DS1307 за даташитом потребує 4.5–5.5 В. Варіанти (за порядком спроб):

1. **VCC модуля → 3.3 В.** Шина лишається 3.3 В — безпечно для ESP32. DS1307 працює поза
   специфікацією і може не відповідати; тоді на екрані `--:--`, решта працює.
2. **VCC → 5Vin + перетворювач рівнів (BSS138)** — коректний варіант.
3. **VCC → 5Vin напряму** — на шині ~4–4.2 В, що вище допустимих 3.6 В для ESP32-S3. На свій ризик.

Батарейки немає, тому після вимкнення живлення RTC втрачає час. Якщо ставити батарейку — лише
**LIR2032** (на модулі є ланцюг заряджання, CR2032 може здутися) і лише при VCC = 5 В
(при 3.3 В з батарейкою DS1307 блокує I2C).

При старті прошивка сканує шину і пише знайдені адреси в лог: очікувано `0x3C`, `0x50`, `0x68`.

## Встановлення часу

Час береться з ПК **під час збірки** (мітка генерується на кожну збірку) і записується в RTC
**при першому старті нової прошивки**, плюс `RTC_BUILD_OFFSET_SEC` на прошивку і завантаження.

Якщо живлення RTC пропадало (маркер у RAM DS1307 втрачено), час невідомий — на екрані `--:--`
до наступної прошивки. Перехід на літній/зимовий час не обробляється — перепрошити.

## Екран і меню

Головний екран: `14:37   3    27s` — час, підтверджена порція, секунди до автоподачі
(праворуч порожньо, якщо AUTO вимкнено).

| Дія | Головний екран | Меню | Portion | Auto Mode |
|---|---|---|---|---|
| Поворот | — | вибір пункту | рівень 1–4 (LED одразу показують) | вибір Enabled / Disabled |
| Клік | відкрити меню | увійти в пункт | зберегти рівень → меню | застосувати → меню |
| «Назад» (GPIO8) | — | головний екран | скасувати (LED повертаються) → меню | без змін → меню |

- У Auto Mode `*` позначає поточний режим; червоний LED горить, коли AUTO увімкнено.
- «Насипати зараз» (GPIO7) працює з будь-якого екрана. Під час насипання — екран `FEEDING...`,
  годується **підтверджена** порція, ввід ігнорується; після циклу повертається той самий екран.
- Автоподача під час відкритого меню поводиться так само.

## Збірка

```
ESP-IDF: Open Folder → miniproject_4/
ESP-IDF: Set Espressif Device Target → esp32s3
ESP-IDF: Build
ESP-IDF: Flash
```

Або з PowerShell: `. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1; idf.py build flash monitor`.

Хостові модульні тести чистої логіки (Git Bash): `bash test/host/run_tests.sh`.

## Калібрування (`main/config.h`)

- `MOTOR_MS_PER_PORTION` — час роботи мотора на одну порцію.
- `SERVO_MIN_US` / `SERVO_MAX_US` у `servo.c` — якщо заслінка не до кінця відкривається/закривається.
- `AUTO_INTERVAL_SEC` — інтервал автоподачі (зараз 30 с для демо).
- `RTC_BUILD_OFFSET_SEC` — поправка часу між збіркою і стартом плати.

Якщо зображення на екрані перевернуте — у `oled_init()` (`main/oled.c`) після
`esp_lcd_panel_init()` додати `esp_lcd_panel_mirror(panel, true, true);`.
````

- [ ] **Step 2: Фінальна перевірка**

Run: `bash test/host/run_tests.sh`
Expected: 5 тест-файлів, усі `OK`, 0 Failures (9 + 14 + 18 + 7 + 16 = 64 тести).

Run (PowerShell): `. C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1 *> $null; idf.py fullclean; idf.py build`
Expected: `Project build complete.`

- [ ] **Step 3: Commit**

```bash
git add README.md
git commit -m "docs: README for RTC, OLED and menu"
```

- [ ] **Step 4: Передати користувачу ручний чекліст на платі**

Прошивку і перевірку на залізі виконує користувач (`idf.py flash monitor`):
1. У лозі скан I2C показує `0x3C`, `0x50`, `0x68` (або без `0x50/0x68`, якщо RTC не відповідає при 3.3 В).
2. Після прошивки на екрані правильний час (± `RTC_BUILD_OFFSET_SEC`).
3. Після вимкнення/увімкнення живлення — `--:--`.
4. Клік → меню; поворот рухає курсор; «Назад» → головний екран.
5. Portion: поворот змінює рівень і сині LED; клік зберігає (переживає перезавантаження); «Назад» скасовує і повертає LED.
6. Auto Mode: `*` на поточному режимі; Enabled → червоний LED і відлік праворуч; Disabled → LED гасне, праворуч порожньо.
7. GPIO7 з меню Portion з непідтвердженим вибором → `FEEDING...` підтвердженою порцією → повернення в те саме меню з тим самим вибором.
8. Автоподача під час відкритого меню → те саме, що п. 7.
