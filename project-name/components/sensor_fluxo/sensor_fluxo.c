    #include "sensor_fluxo.h"

    #include <stdatomic.h>
    #include <inttypes.h>              // para PRIu32

    #include "esp_err.h"
    #include "esp_log.h"
    #include "esp_timer.h"
    #include "driver/gpio.h"

    // FreeRTOS / port
    #include "freertos/FreeRTOS.h"
    #include "freertos/portmacro.h"

    static const char *TAG = "sensor_fluxo";

    static sensor_fluxo_config_t g_cfg;
    static esp_timer_handle_t    g_timer = NULL;

    // contador de pulsos (ISR incrementa)
    static _Atomic uint32_t      g_pulse_count = 0;
    // anti-glitch
    static uint64_t              g_last_edge_us = 0;

    // spinlock para proteger a struct de métricas
    static portMUX_TYPE          g_spin = portMUX_INITIALIZER_UNLOCKED;

    static sensor_fluxo_metrics_t g_metrics = {0};

    static void IRAM_ATTR isr_handler(void *arg) {
        uint64_t now = (uint64_t)esp_timer_get_time();
        if (now - g_last_edge_us > g_cfg.glitch_us) {
            atomic_fetch_add_explicit(&g_pulse_count, 1, memory_order_relaxed);
            g_last_edge_us = now;
        }
    }

    // Timer: roda a cada sample_ms e atualiza L/min / litros
    static void timer_cb(void *arg) {
        uint32_t pulses = atomic_exchange_explicit(&g_pulse_count, 0, memory_order_relaxed);
        uint64_t now_us = esp_timer_get_time();

        portENTER_CRITICAL(&g_spin);
        uint64_t prev_us = g_metrics.last_window_us;
        if (prev_us == 0) prev_us = now_us - (uint64_t)g_cfg.sample_ms * 1000ULL;
        g_metrics.last_window_us = now_us;
        g_metrics.pulses_window  = pulses;

        float liters_window = (g_cfg.pulses_per_liter > 0.f) ? (pulses / g_cfg.pulses_per_liter) : 0.f;
        float minutes = (float)(now_us - prev_us) / 60000000.0f; // us -> min
        g_metrics.flow_l_min = (minutes > 0.f) ? (liters_window / minutes) : 0.f;
        g_metrics.total_liters += liters_window;
        portEXIT_CRITICAL(&g_spin);
    }

    esp_err_t sensor_fluxo_init(const sensor_fluxo_config_t *cfg) {
        if (!cfg) {
            ESP_LOGI(TAG, "cfg nulo");
            return ESP_ERR_INVALID_ARG;
        }
        if (cfg->pin < 0) {
            ESP_LOGI(TAG, "pino invalido");
            return ESP_ERR_INVALID_ARG;
        }

        g_cfg = *cfg;

        // GPIO de entrada
        gpio_config_t io = {
            .pin_bit_mask = 1ULL << g_cfg.pin,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = g_cfg.use_internal_pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_NEGEDGE
        };
        esp_err_t err = gpio_config(&io);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "gpio_config falhou: %s", esp_err_to_name(err));
            return err;
        }

        // serviço de interrupção
        err = gpio_install_isr_service(0);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGI(TAG, "gpio_install_isr_service falhou: %s", esp_err_to_name(err));
            return err;
        }
        err = gpio_isr_handler_add(g_cfg.pin, isr_handler, NULL);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "gpio_isr_handler_add falhou: %s", esp_err_to_name(err));
            return err;
        }

        // timer periódico
        const esp_timer_create_args_t targs = {
            .callback = &timer_cb,
            .name = "sensor_fluxo_tmr"
        };
        err = esp_timer_create(&targs, &g_timer);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "esp_timer_create falhou: %s", esp_err_to_name(err));
            return err;
        }
        err = esp_timer_start_periodic(g_timer, (uint64_t)g_cfg.sample_ms * 1000ULL);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "esp_timer_start_periodic falhou: %s", esp_err_to_name(err));
            return err;
        }

        ESP_LOGI(TAG,
                "Init OK (pin=%d, PPL=%.2f, sample=%" PRIu32 " ms, glitch=%" PRIu32 " us, pullup=%d)",
                g_cfg.pin, (double)g_cfg.pulses_per_liter, g_cfg.sample_ms, g_cfg.glitch_us,
                g_cfg.use_internal_pullup);

        return ESP_OK;
    }

    esp_err_t sensor_fluxo_set_enabled(bool enabled) {
        if (!g_timer) {
            ESP_LOGI(TAG, "timer nao criado");
            return ESP_ERR_INVALID_STATE;
        }
        esp_err_t err;
        if (enabled) {
            err = esp_timer_start_periodic(g_timer, (uint64_t)g_cfg.sample_ms * 1000ULL);
            if (err != ESP_OK) ESP_LOGI(TAG, "esp_timer_start_periodic falhou: %s", esp_err_to_name(err));
        } else {
            err = esp_timer_stop(g_timer);
            if (err != ESP_OK) ESP_LOGI(TAG, "esp_timer_stop falhou: %s", esp_err_to_name(err));
        }
        return err;
    }

    void sensor_fluxo_reset_totals(void) {
        atomic_store_explicit(&g_pulse_count, 0, memory_order_relaxed);
        portENTER_CRITICAL(&g_spin);
        g_metrics.total_liters = 0.0;
        g_metrics.pulses_window = 0;
        g_metrics.flow_l_min = 0.0f;
        g_metrics.last_window_us = 0;
        portEXIT_CRITICAL(&g_spin);
    }

    void sensor_fluxo_get_metrics(sensor_fluxo_metrics_t *out) {
        if (!out) return;
        portENTER_CRITICAL(&g_spin);
        *out = g_metrics;
        portEXIT_CRITICAL(&g_spin);
    }

    uint32_t sensor_fluxo_get_pulse_count(void) {
        return atomic_load_explicit(&g_pulse_count, memory_order_relaxed);
    }
