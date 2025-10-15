# main/CMakeLists.txt
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES gps mpu6050 can_transmiter sensor_temperatura
)
