#ifndef MPU6050_H
#define MPU6050_H

#include "esp_err.h"
#include <stdint.h>

esp_err_t mpu6050_init(void);
esp_err_t mpu6050_read_accel(int16_t *ax, int16_t *ay, int16_t *az);

#endif
