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
        "menu.c"
        "fb.c"
        "ui_text.c"
        "ui.c"
        "ui_state.c"
        "oled.c"
        "display.c"
        "melody.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_timer esp_driver_gpio esp_driver_ledc esp_driver_i2c esp_lcd
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
