#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t  pin;               // GPIO do sinal
    uint8_t     pulses_per_rev;    // Pulsos por volta mecânica
    uint32_t    min_pulse_us;      // Rejeição: largura mínima entre bordas (µs)
    bool        pullup;            // Habilitar pull-up interno no pino (se aplicável)
    bool        rising_edge;       // true=RISING, false=FALLING
} rpm_counter_cfg_t;

typedef struct {
    // estado interno (não acessar diretamente fora do .c)
    volatile uint32_t pulse_count;
    volatile uint32_t last_edge_us;
    portMUX_TYPE      spin;
    rpm_counter_cfg_t cfg;
    bool              started;
} rpm_counter_t;

/** Inicializa estrutura e configura GPIO/ISR (não inicia contagem). */
esp_err_t rpm_counter_init(rpm_counter_t *rc, const rpm_counter_cfg_t *cfg);

/** Inicia contagem (registra ISR). */
esp_err_t rpm_counter_start(rpm_counter_t *rc);

/** Para contagem (desregistra ISR). */
esp_err_t rpm_counter_stop(rpm_counter_t *rc);

/** Lê de forma atômica os pulsos acumulados e zera o contador. */
uint32_t rpm_counter_read_and_reset(rpm_counter_t *rc);

/** Converte contagem para RPM, dado o intervalo de amostragem em milissegundos. */
static inline uint32_t rpm_counter_to_rpm(uint32_t pulses_in_window,
                                          uint32_t window_ms,
                                          uint8_t pulses_per_rev)
{
    if (window_ms == 0 || pulses_per_rev == 0) return 0;
    // RPM = (pulsos / janela_ms) * 60000 / pulses_per_rev
    uint64_t num = (uint64_t)pulses_in_window * 60000ULL;
    return (uint32_t)(num / ((uint64_t)window_ms * (uint64_t)pulses_per_rev));
}

#ifdef __cplusplus
}
#endif
