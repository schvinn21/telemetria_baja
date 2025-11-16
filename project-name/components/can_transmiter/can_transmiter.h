#ifndef CAN_TRANSMITER_H
#define CAN_TRANSMITER_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa o barramento CAN (TWAI).
 *
 * @return ESP_OK em caso de sucesso, ESP_FAIL ou outro código em caso de erro.
 */
esp_err_t can_transmiter_init(void);

/**
 * @brief Envia dois frames CAN:
 *        - ID 0x123: aceleração (ax, ay, az)
 *        - ID 0x124: latitude e longitude (float, 4 bytes cada)
 *
 * @param ax Aceleração em X (int16_t)
 * @param ay Aceleração em Y (int16_t)
 * @param az Aceleração em Z (int16_t)
 * @param latitude Latitude em float (ex: -30.123456)
 * @param longitude Longitude em float (ex: -51.123456)
 * @return ESP_OK em caso de sucesso; ESP_FAIL em caso de erro no envio.
 */
//esp_err_t can_enviar_dados(int16_t ax, int16_t ay, int16_t az, float latitude, float longitude);

esp_err_t can_enviar_temp_e_total_compacto(float temp1, float temp2, float total_liters);

#ifdef __cplusplus
}
#endif

#endif // CAN_TRANSMITER_H
