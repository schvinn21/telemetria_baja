#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mpu6050.h"
#include "gps.h"
#include "can_transmiter.h"  // nome correto
#include "sensor_temp.h"
#include "esp_log.h"


extern float gps_latitude;
extern float gps_longitude;

void app_main(void) {
    printf("Iniciando sensores...\n");
    
    ds18b20_t sensor;
    ESP_ERROR_CHECK(ds18b20_init(&sensor, GPIO_NUM_4, false)); // DQ no GPIO4
    // opcional: ESP_ERROR_CHECK(ds18b20_set_resolution(&sensor, 12));

    if (mpu6050_init() != ESP_OK) {
        printf("Falha ao inicializar o MPU6050\n");
        return;
    }

    gps_init();
    gps_start_task();

    if (can_transmiter_init() != ESP_OK) {
        printf("Falha ao inicializar CAN\n");
        return;
    }

    while (1) {
        float t = 0.0f;
        esp_err_t err = ds18b20_read_temperature(&sensor, &t);
        if (err == ESP_OK) {
            ESP_LOGI("APP", "Temperatura: %.2f °C", t);
        } else {
            ESP_LOGE("APP", "Falha ao ler temperatura (%s)", esp_err_to_name(err));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        int16_t ax, ay, az;
        if (mpu6050_read_accel(&ax, &ay, &az) == ESP_OK) {
            printf("Accel X: %d, Y: %d, Z: %d\n", ax, ay, az);
            printf("Latitude: %.6f, Longitude: %.6f\n", gps_latitude, gps_longitude);
            can_enviar_dados(ax, ay, az, gps_latitude, gps_longitude);
        } else {
            printf("Erro na leitura do MPU6050\n");
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
