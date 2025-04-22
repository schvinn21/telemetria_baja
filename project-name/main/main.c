#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mpu6050.h"
#include "gps.h"
#include "can_transmiter.h"  // nome correto

extern float gps_latitude;
extern float gps_longitude;

void app_main(void) {
    printf("Iniciando sensores...\n");

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
