// mpu6050.c
#include "mpu6050.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

#define MPU6050_ADDR                0x68
#define MPU6050_REG_PWR_MGMT_1     0x6B
#define MPU6050_REG_ACCEL_XOUT_H   0x3B

static esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) return err;
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                              I2C_MASTER_RX_BUF_DISABLE,
                              I2C_MASTER_TX_BUF_DISABLE, 0);
}

static esp_err_t mpu6050_write_byte(uint8_t reg_addr, uint8_t data) {
    return i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR,
                                      (uint8_t[]){reg_addr, data}, 2,
                                      pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

static esp_err_t mpu6050_read_bytes(uint8_t reg_addr, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR,
                                        &reg_addr, 1, data, len,
                                        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

static int16_t convert_to_int16(uint8_t high, uint8_t low) {
    return (int16_t)((high << 8) | low);
}

esp_err_t mpu6050_init(void) {
    ESP_ERROR_CHECK(i2c_master_init());
    return mpu6050_write_byte(MPU6050_REG_PWR_MGMT_1, 0x00);
}

esp_err_t mpu6050_read_accel(int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t raw_data[6];
    esp_err_t err = mpu6050_read_bytes(MPU6050_REG_ACCEL_XOUT_H, raw_data, 6);
    if (err != ESP_OK) return err;

    *ax = convert_to_int16(raw_data[0], raw_data[1]);
    *ay = convert_to_int16(raw_data[2], raw_data[3]);
    *az = convert_to_int16(raw_data[4], raw_data[5]);

    return ESP_OK;
}
