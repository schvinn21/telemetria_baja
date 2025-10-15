#pragma once
#include "driver/gpio.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t pin;      // GPIO conectado ao DQ do DS18B20 (com pull-up a 4.7k)
    bool parasite_power; // true se estiver usando alimentação parasita (Vdd não ligado). (usamos busy-poll; não "strong pull-up")
} ds18b20_t;

/**
 * @brief Inicializa a linha 1-Wire no pino dado.
 * Configura o GPIO como open-drain "manual" (saída/entrada) com pull-up habilitado.
 */
esp_err_t ds18b20_init(ds18b20_t *dev, gpio_num_t pin, bool parasite_power);

/**
 * @brief Lê a temperatura em graus Celsius (float).
 * - Dispara a conversão
 * - Aguarda término (busy-poll, timeout ~1s)
 * - Lê o scratchpad e valida CRC
 */
esp_err_t ds18b20_read_temperature(const ds18b20_t *dev, float *out_celsius);

/**
 * @brief Define a resolução de conversão (9..12 bits). Padrão de fábrica: 12 bits.
 * @note Maior resolução = maior tempo de conversão (até ~750 ms em 12 bits).
 */
esp_err_t ds18b20_set_resolution(const ds18b20_t *dev, uint8_t resolution_bits);

#ifdef __cplusplus
}
#endif
