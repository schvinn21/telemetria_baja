#ifndef CAN_RECEIVER_H
#define CAN_RECEIVER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa o receptor CAN (TWAI) e inicia a tarefa de recepção.
 *
 * @return ESP_OK em caso de sucesso ou um erro do tipo esp_err_t.
 */
esp_err_t can_receiver_init(void);

#ifdef __cplusplus
}
#endif

#endif // CAN_RECEIVER_H
