#include "sensor_temp.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

#define TAG "DS18B20"

// --- Timings 1-Wire (µs) ---
#define T_RESETL_US   480
#define T_PRESENCE_US 70
#define T_RESET_END   410

#define T_SLOT_US     60
#define T_WRITE1_LOW  6
#define T_WRITE0_LOW  60
#define T_READ_INIT   6
#define T_READ_SAMPLE 9

// DS18B20 Commands
#define CMD_SKIP_ROM          0xCC
#define CMD_CONVERT_T         0x44
#define CMD_READ_SCRATCHPAD   0xBE
#define CMD_WRITE_SCRATCHPAD  0x4E
#define CMD_COPY_SCRATCHPAD   0x48

// Scratchpad size
#define SCRATCHPAD_LEN 9

// ---------- GPIO helpers ----------
static inline void ow_set_output(gpio_num_t pin) {
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
}
static inline void ow_set_input(gpio_num_t pin) {
    gpio_set_direction(pin, GPIO_MODE_INPUT);
}
static inline void ow_write_low(gpio_num_t pin) {
    // drive low
    gpio_set_level(pin, 0);
}
static inline void ow_release(gpio_num_t pin) {
    // release line (input with pull-up)
    ow_set_input(pin);
}

static esp_err_t ow_drive_low_then_release(gpio_num_t pin, uint32_t low_us, uint32_t rest_us) {
    ow_set_output(pin);
    ow_write_low(pin);
    esp_rom_delay_us(low_us);
    ow_release(pin);
    if (rest_us) esp_rom_delay_us(rest_us);
    return ESP_OK;
}

// Reset + presence detect. Returns ESP_OK if presence detected.
static esp_err_t ow_reset(gpio_num_t pin) {
    // init as input with pull-up
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD, // we emulate OD manually; keep pull-up
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&cfg);

    // drive low for reset
    ow_set_output(pin);
    ow_write_low(pin);
    esp_rom_delay_us(T_RESETL_US);

    // release and wait presence
    ow_release(pin);
    esp_rom_delay_us(T_PRESENCE_US);

    int presence = gpio_get_level(pin); // presence pulse holds the line LOW
    esp_rom_delay_us(T_RESET_END);

    return (presence == 0) ? ESP_OK : ESP_ERR_NOT_FOUND;
}

static void ow_write_bit(gpio_num_t pin, int bit) {
    if (bit) {
        // write '1': pull low briefly, then release for rest of slot
        ow_drive_low_then_release(pin, T_WRITE1_LOW, T_SLOT_US - T_WRITE1_LOW);
    } else {
        // write '0': keep low for the whole slot
        ow_drive_low_then_release(pin, T_WRITE0_LOW, T_SLOT_US - T_WRITE0_LOW);
    }
}

static int ow_read_bit(gpio_num_t pin) {
    // init read time slot
    ow_set_output(pin);
    ow_write_low(pin);
    esp_rom_delay_us(T_READ_INIT);
    ow_release(pin);
    esp_rom_delay_us(T_READ_SAMPLE);
    int level = gpio_get_level(pin);
    // wait until slot end
    uint32_t elapsed = T_READ_INIT + T_READ_SAMPLE;
    if (T_SLOT_US > elapsed) esp_rom_delay_us(T_SLOT_US - elapsed);
    return level ? 1 : 0;
}

static void ow_write_byte(gpio_num_t pin, uint8_t byte) {
    for (int i = 0; i < 8; ++i) {
        ow_write_bit(pin, (byte >> i) & 0x01);
    }
}

static uint8_t ow_read_byte(gpio_num_t pin) {
    uint8_t val = 0;
    for (int i = 0; i < 8; ++i) {
        int b = ow_read_bit(pin);
        val |= (b << i);
    }
    return val;
}

// Dallas/Maxim CRC-8 (polynomial 0x31, reflected 0x8C)
static uint8_t crc8_dallas(const uint8_t *data, int len) {
    uint8_t crc = 0;
    for (int i = 0; i < len; ++i) {
        uint8_t inbyte = data[i];
        for (int j = 0; j < 8; ++j) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

// ---------- Public API ----------
esp_err_t ds18b20_init(ds18b20_t *dev, gpio_num_t pin, bool parasite_power) {
    if (!dev) return ESP_ERR_INVALID_ARG;
    dev->pin = pin;
    dev->parasite_power = parasite_power;

    // Basic GPIO setup (internal pull-up helps, but use resistor externo ~4.7k)
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&cfg);

    // Check presence
    esp_err_t err = ow_reset(pin);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Sem presença no 1-Wire (verifique sensor e pull-up)");
        return err;
    }
    return ESP_OK;
}

static esp_err_t ds18b20_start_conversion(const ds18b20_t *dev) {
    if (ow_reset(dev->pin) != ESP_OK) return ESP_FAIL;
    ow_write_byte(dev->pin, CMD_SKIP_ROM);
    ow_write_byte(dev->pin, CMD_CONVERT_T);

    // Busy polling: enquanto converter, sensor segura o bit=0; quando termina, lê-se 1.
    // Timeout de segurança ~1000 ms.
    const uint32_t timeout_ms = 1000;
    uint32_t waited = 0;

    // Se estiver alimentado externamente, busy-poll funciona bem; em parasita, também,
    // mas não oferecemos "strong pull-up" via GPIO.
    while (waited < timeout_ms) {
        int done = ow_read_bit(dev->pin);
        if (done) return ESP_OK;
        esp_rom_delay_us(1000);
        waited += 1;
    }
    ESP_LOGW(TAG, "Timeout aguardando conversão");
    return ESP_ERR_TIMEOUT;
}

static esp_err_t ds18b20_read_scratchpad(const ds18b20_t *dev, uint8_t *buf9) {
    if (ow_reset(dev->pin) != ESP_OK) return ESP_FAIL;
    ow_write_byte(dev->pin, CMD_SKIP_ROM);
    ow_write_byte(dev->pin, CMD_READ_SCRATCHPAD);

    for (int i = 0; i < SCRATCHPAD_LEN; ++i) {
        buf9[i] = ow_read_byte(dev->pin);
    }
    // valida CRC
    uint8_t crc = crc8_dallas(buf9, 8);
    if (crc != buf9[8]) {
        ESP_LOGE(TAG, "CRC inválido (calc=0x%02X, rx=0x%02X)", crc, buf9[8]);
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}

esp_err_t ds18b20_read_temperature(const ds18b20_t *dev, float *out_celsius) {
    if (!dev || !out_celsius) return ESP_ERR_INVALID_ARG;

    esp_err_t err = ds18b20_start_conversion(dev);
    if (err != ESP_OK) return err;

    uint8_t sp[SCRATCHPAD_LEN] = {0};
    err = ds18b20_read_scratchpad(dev, sp);
    if (err != ESP_OK) return err;

    // Temperatura: 16 bits signed, LSB primeiro.
    int16_t raw = (int16_t)((sp[1] << 8) | sp[0]);
    // Conversão independente da resolução; se usar <12 bits, os LSBs são zero.
    *out_celsius = (float)raw / 16.0f;
    return ESP_OK;
}

esp_err_t ds18b20_set_resolution(const ds18b20_t *dev, uint8_t resolution_bits) {
    if (!dev) return ESP_ERR_INVALID_ARG;
    if (resolution_bits < 9 || resolution_bits > 12) return ESP_ERR_INVALID_ARG;

    // Config byte (bits R1:R0 em [6:5]) — restante pode ficar default
    uint8_t cfg = 0;
    switch (resolution_bits) {
        case 9:  cfg = 0x1F; break;   // R1:R0 = 00
        case 10: cfg = 0x3F; break;   // 01
        case 11: cfg = 0x5F; break;   // 10
        case 12: cfg = 0x7F; break;   // 11
    }

    // Precisamos escrever TH, TL, CONFIG (usaremos 0x4B e 0x46 padrão)
    if (ow_reset(dev->pin) != ESP_OK) return ESP_FAIL;
    ow_write_byte(dev->pin, CMD_SKIP_ROM);
    ow_write_byte(dev->pin, CMD_WRITE_SCRATCHPAD);
    ow_write_byte(dev->pin, 0x4B); // TH (default)
    ow_write_byte(dev->pin, 0x46); // TL (default)
    ow_write_byte(dev->pin, cfg);  // CONFIG

    // Opcional: COPY SCRATCHPAD para EEPROM interna (demora ~10ms)
    if (ow_reset(dev->pin) != ESP_OK) return ESP_FAIL;
    ow_write_byte(dev->pin, CMD_SKIP_ROM);
    ow_write_byte(dev->pin, CMD_COPY_SCRATCHPAD);
    // Espera mínima para completar cópia
    esp_rom_delay_us(15000);

    return ESP_OK;
}
