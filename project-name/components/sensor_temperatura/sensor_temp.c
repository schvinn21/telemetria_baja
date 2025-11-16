#include "sensor_temp.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "DS18B20"

// ---------- Timings (µs) — folgados/estáveis ----------
#define T_RESETL_US    480
#define T_PRESENCE_US  70
#define T_RESET_END    410

#define T_SLOT_US      70
#define T_W1_LOW       6
#define T_W0_LOW       60
#define T_R_INIT       6
#define T_R_SAMPLE     12   // amostra ~18us após início do slot

// ---------- Comandos ----------
#define CMD_SKIP_ROM          0xCC
#define CMD_CONVERT_T         0x44
#define CMD_READ_SCRATCHPAD   0xBE
#define CMD_WRITE_SCRATCHPAD  0x4E
#define CMD_COPY_SCRATCHPAD   0x48

#define SCRATCHPAD_LEN 9

// ---------- GPIO helpers (open-drain) ----------
static inline void ow_cfg_od(gpio_num_t pin) {
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << pin),
        .mode         = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en   = GPIO_PULLUP_ENABLE,   // ainda use o pull-up EXTERNO 4k7
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&cfg);
}
static inline void ow_drive_low(gpio_num_t pin) { gpio_set_level(pin, 0); }
static inline void ow_release (gpio_num_t pin) { gpio_set_level(pin, 1); }

// ---------- 1-Wire low level ----------
static esp_err_t ow_reset(gpio_num_t pin) {
    ow_cfg_od(pin);
    ow_drive_low(pin);
    esp_rom_delay_us(T_RESETL_US);
    ow_release(pin);
    esp_rom_delay_us(T_PRESENCE_US);
    int presence = gpio_get_level(pin);  // 0 = presença
    esp_rom_delay_us(T_RESET_END + 5);
    return (presence == 0) ? ESP_OK : ESP_ERR_NOT_FOUND;
}

static void ow_write_bit(gpio_num_t pin, int bit) {
    if (bit) {
        ow_drive_low(pin);
        esp_rom_delay_us(T_W1_LOW);
        ow_release(pin);
        esp_rom_delay_us(T_SLOT_US - T_W1_LOW);
    } else {
        ow_drive_low(pin);
        esp_rom_delay_us(T_W0_LOW);
        ow_release(pin);
        esp_rom_delay_us(T_SLOT_US - T_W0_LOW);
    }
}

static int ow_read_bit(gpio_num_t pin) {
    ow_drive_low(pin);
    esp_rom_delay_us(T_R_INIT);
    ow_release(pin);
    esp_rom_delay_us(T_R_SAMPLE);
    int level = gpio_get_level(pin);
    uint32_t elapsed = T_R_INIT + T_R_SAMPLE;
    if (T_SLOT_US > elapsed) esp_rom_delay_us(T_SLOT_US - elapsed);
    return level ? 1 : 0;
}

static void ow_write_byte(gpio_num_t pin, uint8_t byte) {
    for (int i = 0; i < 8; ++i) {
        ow_write_bit(pin, (byte >> i) & 0x01);
    }
}

static uint8_t ow_read_byte(gpio_num_t pin) {
    uint8_t v = 0;
    for (int i = 0; i < 8; ++i) v |= (ow_read_bit(pin) << i);
    return v;
}

// ---------- CRC Dallas ----------
static uint8_t crc8_dallas(const uint8_t *data, int len) {
    uint8_t crc = 0;
    for (int i = 0; i < len; ++i) {
        uint8_t in = data[i];
        for (int b = 0; b < 8; ++b) {
            uint8_t mix = (crc ^ in) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            in >>= 1;
        }
    }
    return crc;
}

// ---------- Internos ----------
static int tconv_ms_from_cfg(uint8_t cfg) {
    switch ((cfg >> 5) & 0x03) { // R1:R0
        case 0: return 94;   //  9 bits
        case 1: return 188;  // 10 bits
        case 2: return 375;  // 11 bits
        default: return 750; // 12 bits
    }
}

static esp_err_t ds18b20_start_conversion(const ds18b20_t *dev) {
    if (dev->parasite_power) {
        // sem strong pull-up → não suportamos parasita
        return ESP_ERR_NOT_SUPPORTED;
    }

    // Inicia conversão
    esp_err_t err = ow_reset(dev->pin);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "reset antes de CONVERT_T falhou: %s", esp_err_to_name(err));
        return err;
    }
    ow_write_byte(dev->pin, CMD_SKIP_ROM);
    ow_write_byte(dev->pin, CMD_CONVERT_T);

    // Ler config para saber a resolução e esperar o tempo certo;
    // se falhar, espera 750ms (12 bits).
    err = ow_reset(dev->pin);
    if (err != ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(750 + 20));
        return ESP_OK;
    }
    ow_write_byte(dev->pin, CMD_SKIP_ROM);
    ow_write_byte(dev->pin, CMD_READ_SCRATCHPAD);
    uint8_t sp[SCRATCHPAD_LEN] = {0};
    for (int i = 0; i < SCRATCHPAD_LEN; ++i) sp[i] = ow_read_byte(dev->pin);

    int tconv = tconv_ms_from_cfg(sp[4]);
    vTaskDelay(pdMS_TO_TICKS(tconv + 20));
    return ESP_OK;
}

static esp_err_t ds18b20_read_scratchpad(const ds18b20_t *dev, uint8_t *buf9) {
    // Duas tentativas (ruído/clone)
    for (int attempt = 0; attempt < 2; ++attempt) {
        esp_err_t err = ow_reset(dev->pin);
        if (err != ESP_OK) return err;

        ow_write_byte(dev->pin, CMD_SKIP_ROM);
        ow_write_byte(dev->pin, CMD_READ_SCRATCHPAD);

        uint8_t sp[SCRATCHPAD_LEN];
        for (int i = 0; i < SCRATCHPAD_LEN; ++i) sp[i] = ow_read_byte(dev->pin);

        uint8_t crc = crc8_dallas(sp, 8);
        if (crc == sp[8]) {
            for (int i = 0; i < SCRATCHPAD_LEN; ++i) buf9[i] = sp[i];
            return ESP_OK;
        }
        esp_rom_delay_us(200);
    }
    return ESP_ERR_INVALID_CRC;
}

// ---------- API ----------
esp_err_t ds18b20_init(ds18b20_t *dev, gpio_num_t pin, bool parasite_power) {
    if (!dev) return ESP_ERR_INVALID_ARG;
    dev->pin = pin;
    dev->parasite_power = parasite_power;

    ow_cfg_od(pin);
    vTaskDelay(pdMS_TO_TICKS(10));

    esp_err_t err = ow_reset(pin);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Sem presença no 1-Wire (verifique VDD=3V3, GND, DQ→GPIO e pull-up 4k7)");
        return err;
    }
    return ESP_OK;
}

esp_err_t ds18b20_set_resolution(const ds18b20_t *dev, uint8_t resolution_bits) {
    if (!dev) return ESP_ERR_INVALID_ARG;
    if (resolution_bits < 9 || resolution_bits > 12) return ESP_ERR_INVALID_ARG;

    uint8_t cfg;
    switch (resolution_bits) {
        case 9:  cfg = 0x1F; break; // 00
        case 10: cfg = 0x3F; break; // 01
        case 11: cfg = 0x5F; break; // 10
        default: cfg = 0x7F; break; // 11 (12 bits)
    }

    esp_err_t err = ow_reset(dev->pin);
    if (err != ESP_OK) return err;

    ow_write_byte(dev->pin, CMD_SKIP_ROM);
    ow_write_byte(dev->pin, CMD_WRITE_SCRATCHPAD);
    ow_write_byte(dev->pin, 0x4B); // TH default
    ow_write_byte(dev->pin, 0x46); // TL default
    ow_write_byte(dev->pin, cfg);  // CONFIG

    err = ow_reset(dev->pin);
    if (err != ESP_OK) return err;

    ow_write_byte(dev->pin, CMD_SKIP_ROM);
    ow_write_byte(dev->pin, CMD_COPY_SCRATCHPAD);
    esp_rom_delay_us(15000); // 10–15ms
    return ESP_OK;
}

esp_err_t ds18b20_read_celsius(const ds18b20_t *dev, float *out_c) {
    if (!dev || !out_c) return ESP_ERR_INVALID_ARG;

    esp_err_t err = ds18b20_start_conversion(dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Conversão não iniciada: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t sp[SCRATCHPAD_LEN] = {0};
    err = ds18b20_read_scratchpad(dev, sp);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Scratchpad com CRC inválido");
        return err;
    }

    // 85.00°C = valor de power-on (sem conversão)
    if (sp[0] == 0x50 && sp[1] == 0x05) {
        ESP_LOGW(TAG, "85.00°C (power-on) — conversão não ocorreu");
        return ESP_ERR_INVALID_STATE;
    }

    // tudo zero? (linha morta / sem pull-up)
    bool all0 = true;
    for (int i = 0; i < 8; ++i) if (sp[i] != 0x00) { all0 = false; break; }
    if (all0) {
        ESP_LOGE(TAG, "Scratchpad todo zero — verifique pull-up/conexões.");
        return ESP_FAIL;
    }

    int16_t raw = (int16_t)((sp[1] << 8) | sp[0]);
    *out_c = (float)raw / 16.0f;
    return ESP_OK;
}
