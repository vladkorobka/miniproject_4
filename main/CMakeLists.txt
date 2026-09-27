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
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_timer esp_driver_gpio esp_driver_ledc
)
