#pragma once
// Header LIMPO: não coloque variáveis globais nem funções estáticas aqui.

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t pin;             // pino do sinal do sensor
    float      pulses_per_liter; // PPL do sensor (ex.: 450.0)
    uint32_t   sample_ms;       // janela p/ cálculo (ms)
    uint32_t   glitch_us;       // anti-ruído: ignora bordas mais próximas que isso (us)
    bool       use_internal_pullup; // true: habilita pull-up interno
} sensor_fluxo_config_t;

typedef struct {
    float     flow_l_min;     // L/min (atualizado a cada janela)
    double    total_liters;   // litros acumulados desde o reset
    uint32_t  pulses_window;  // pulsos na última janela
    uint64_t  last_window_us; // timestamp da última janela (us)
} sensor_fluxo_metrics_t;

// API
esp_err_t sensor_fluxo_init(const sensor_fluxo_config_t *cfg);
esp_err_t sensor_fluxo_set_enabled(bool enabled);
void      sensor_fluxo_reset_totals(void);
void      sensor_fluxo_get_metrics(sensor_fluxo_metrics_t *out);
uint32_t  sensor_fluxo_get_pulse_count(void);

#ifdef __cplusplus
}
#endif
