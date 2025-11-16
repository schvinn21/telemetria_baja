#pragma once
#include "driver/gpio.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t pin;        // GPIO conectado ao DQ
    bool parasite_power;   // não suportamos strong pull-up; deixe false
} ds18b20_t;

/** Inicializa o barramento e checa presença. */
esp_err_t ds18b20_init(ds18b20_t *dev, gpio_num_t pin, bool parasite_power);

/** Define resolução (9..12 bits). Default de fábrica = 12 bits. */
esp_err_t ds18b20_set_resolution(const ds18b20_t *dev, uint8_t resolution_bits);

/** Lê temperatura em °C (bloqueante ~750 ms p/ 12 bits). */
esp_err_t ds18b20_read_celsius(const ds18b20_t *dev, float *out_c);

#ifdef __cplusplus
}
#endif
