#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "can_receiver.h"
#include <inttypes.h>

#define TAG "CAN_RECV"

static void can_receive_task(void *arg) {
    twai_message_t msg;
    while (1) {
        if (twai_receive(&msg, pdMS_TO_TICKS(1000)) == ESP_OK) {
            ESP_LOGI(TAG, "Recebido ID: 0x%" PRIX32 ", DLC: %d", msg.identifier, msg.data_length_code);

            if (msg.identifier == 0x123 && msg.data_length_code == 6) {
                int16_t ax = (msg.data[0] << 8) | msg.data[1];
                int16_t ay = (msg.data[2] << 8) | msg.data[3];
                int16_t az = (msg.data[4] << 8) | msg.data[5];
                ESP_LOGI(TAG, "🔵 Aceleração - X: %d | Y: %d | Z: %d", ax, ay, az);

            } else if (msg.identifier == 0x124 && msg.data_length_code == 8) {
                float latitude, longitude;
                memcpy(&latitude, &msg.data[0], sizeof(float));
                memcpy(&longitude, &msg.data[4], sizeof(float));
                ESP_LOGI(TAG, "🟢 GPS - Latitude: %.6f | Longitude: %.6f", latitude, longitude);

            } else {
                ESP_LOGW(TAG, "Dados desconhecidos (ID 0x%" PRIX32 "):", msg.identifier);
                for (int i = 0; i < msg.data_length_code; i++) {
                    printf("%02X ", msg.data[i]);
                }
                printf("\n");
            }
        }
    }
}

esp_err_t can_receiver_init(void) {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_5, GPIO_NUM_4, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(TAG, "Receptor CAN iniciado");

    xTaskCreate(can_receive_task, "can_rx", 4096, NULL, 5, NULL);
    return ESP_OK;
}

