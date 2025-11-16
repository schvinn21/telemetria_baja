idf_component_register(
    SRCS "rotacao.c"
    INCLUDE_DIRS "."
    REQUIRES driver
    PRIV_REQUIRES esp_timer esp_driver_gpio
)