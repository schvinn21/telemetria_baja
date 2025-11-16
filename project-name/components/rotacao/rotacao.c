#include "rotacao.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"

static const char *TAG = "rpm_counter";

// ISR instalada por gpio_isr_handler_add precisa dessa assinatura.
static void IRAM_ATTR rpm_isr_handler(void *arg)
{
    rpm_counter_t *rc = (rpm_counter_t*)arg;
    uint32_t now = (uint32_t)esp_timer_get_time(); // micros desde boot

    // Filtro por largura mínima de pulso (anti-ruído)
    if ((now - rc->last_edge_us) >= rc->cfg.min_pulse_us) {
        rc->last_edge_us = now;
        portENTER_CRITICAL_ISR(&rc->spin);
        rc->pulse_count++;
        portEXIT_CRITICAL_ISR(&rc->spin);
    }
}

esp_err_t rpm_counter_init(rpm_counter_t *rc, const rpm_counter_cfg_t *cfg)
{
    if (!rc || !cfg) return ESP_ERR_INVALID_ARG;

    rc->pulse_count  = 0;
    rc->last_edge_us = 0;
    rc->spin         = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;
    rc->cfg          = *cfg;
    rc->started      = false;

    // Configuração do GPIO
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << cfg->pin,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = cfg->pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = cfg->rising_edge ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE
    };

    esp_err_t e = gpio_config(&io);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config falhou (%d)", e);
        return e;
    }

    // Instala serviço de ISR de GPIO (uma única vez no app; aqui é seguro chamar)
    e = gpio_install_isr_service(0);
    if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "gpio_install_isr_service falhou (%d)", e);
        return e;
    }

    return ESP_OK;
}

esp_err_t rpm_counter_start(rpm_counter_t *rc)
{
    if (!rc) return ESP_ERR_INVALID_ARG;
    if (rc->started) return ESP_OK;

    rc->pulse_count  = 0;
    rc->last_edge_us = (uint32_t)esp_timer_get_time();

    esp_err_t e = gpio_isr_handler_add(rc->cfg.pin, rpm_isr_handler, (void*)rc);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "gpio_isr_handler_add falhou (%d)", e);
        return e;
    }

    rc->started = true;
    return ESP_OK;
}

esp_err_t rpm_counter_stop(rpm_counter_t *rc)
{
    if (!rc) return ESP_ERR_INVALID_ARG;
    if (!rc->started) return ESP_OK;

    gpio_isr_handler_remove(rc->cfg.pin);
    rc->started = false;
    return ESP_OK;
}

uint32_t rpm_counter_read_and_reset(rpm_counter_t *rc)
{
    if (!rc) return 0;
    uint32_t val;
    portENTER_CRITICAL(&rc->spin);
    val = rc->pulse_count;
    rc->pulse_count = 0;
    portEXIT_CRITICAL(&rc->spin);
    return val;
}
