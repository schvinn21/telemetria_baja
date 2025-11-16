#include "can_transmiter.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_log.h"
#include <string.h>  // Para memcpy

static const char *TAG = "CAN_TX";

esp_err_t can_transmiter_init(void) {
    // TX = GPIO33, RX = GPIO32 -> confere com seu hardware!
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_33, GPIO_NUM_32, TWAI_MODE_NORMAL);
    twai_timing_config_t  t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t  f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao instalar driver TWAI: %s", esp_err_to_name(err));
        return err;
    }

    err = twai_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao iniciar TWAI: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "CAN/TWAI iniciado (TX=GPIO33, RX=GPIO32, 500 kbps)");
    return ESP_OK;
}

esp_err_t can_enviar_temp_e_total_compacto(float temp1, float temp2, float total_liters)
{
    int16_t t1_i    = (int16_t)(temp1 * 100.0f);
    int16_t t2_i    = (int16_t)(temp2 * 100.0f);
    uint32_t total_ml = (uint32_t)(total_liters * 1000.0f); // mililitros

    twai_message_t msg = {
        .identifier       = 0x125,
        .data_length_code = 8,
        .flags            = TWAI_MSG_FLAG_NONE,
    };

    // Temperaturas em centésimos de grau (big-endian)
    msg.data[0] = (t1_i >> 8) & 0xFF;
    msg.data[1] = (t1_i)      & 0xFF;
    msg.data[2] = (t2_i >> 8) & 0xFF;
    msg.data[3] = (t2_i)      & 0xFF;

    // total_ml em 4 bytes (big-endian)
    msg.data[4] = (total_ml >> 24) & 0xFF;
    msg.data[5] = (total_ml >> 16) & 0xFF;
    msg.data[6] = (total_ml >> 8)  & 0xFF;
    msg.data[7] = (total_ml)       & 0xFF;

    esp_err_t err = twai_transmit(&msg, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao transmitir CAN (compacto): %s", esp_err_to_name(err));
        return ESP_FAIL;
    }
    return ESP_OK;
}

// esp_err_t can_enviar_dados(int16_t ax, int16_t ay, int16_t az, float latitude, float longitude) {
//     // Frame 1: Aceleração
//     twai_message_t msg1 = {
//         .identifier = 0x123,
//         .data_length_code = 6,
//         .flags = TWAI_MSG_FLAG_NONE,
//     };

//     msg1.data[0] = (ax >> 8) & 0xFF; // byte CANH
//     msg1.data[1] = ax & 0xFF; // byte CANL
//     msg1.data[2] = (ay >> 8) & 0xFF;// byte CANH
//     msg1.data[3] = ay & 0xFF; // byte CANL
//     msg1.data[4] = (az >> 8) & 0xFF;// byte CANH
//     msg1.data[5] = az & 0xFF; // byte CANL

//     // Frame 2: GPS
//     twai_message_t msg2 = {
//         .identifier = 0x124,
//         .data_length_code = 8,
//         .flags = TWAI_MSG_FLAG_NONE,
//     };

//     memcpy(&msg2.data[0], &latitude, sizeof(float));
//     memcpy(&msg2.data[4], &longitude, sizeof(float));

//     esp_err_t err1 = twai_transmit(&msg1, pdMS_TO_TICKS(100));
//     esp_err_t err2 = twai_transmit(&msg2, pdMS_TO_TICKS(100));

//     if (err1 != ESP_OK || err2 != ESP_OK) {
//         ESP_LOGE(TAG, "Erro ao transmitir dados CAN: %s / %s", esp_err_to_name(err1), esp_err_to_name(err2));
//         return ESP_FAIL;
//     }

//     return ESP_OK;
// }



