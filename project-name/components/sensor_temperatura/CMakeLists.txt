idf_component_register(
    SRCS         "sensor_temp.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver esp_rom log
)
