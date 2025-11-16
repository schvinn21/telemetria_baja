// #include <stdio.h>
// #include <inttypes.h>              // PRIu32
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"

// #include "sensor_temp.h"           // driver DS18B20
// #include "sensor_fluxo.h"          // driver de fluxo
// #include "can_transmiter.h"        // seu driver CAN

// static const char *TAG = "app_main";

// // Pinos escolhidos
// #define DS18_PIN    GPIO_NUM_4     // DS18B20 no GPIO4
// #define FLUXO_PIN   GPIO_NUM_14    // Sensor de fluxo no GPIO14

// void app_main(void)
// {
//     esp_err_t err;

//     vTaskDelay(pdMS_TO_TICKS(100));   // pequena estabilização

//     // ---------- LOG DO DRIVER GPIO MAIS BAIXO (pra não poluir serial) ----------
//     esp_log_level_set("gpio", ESP_LOG_WARN);

//     // ---------- CONFIG DS18B20 ----------
//     ds18b20_t temp1;

//     err = ds18b20_init(&temp1, DS18_PIN, false);   // VDD em 3V3, sem modo parasita
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "DS18B20 init falhou: %s", esp_err_to_name(err));
//         return;
//     }

//     (void) ds18b20_set_resolution(&temp1, 12);     // 12 bits (~750 ms conversão)

//     // ---------- CONFIG FLUXO ----------
//     sensor_fluxo_config_t cfg = {
//         .pin               = FLUXO_PIN,
//         .pulses_per_liter  = 450.0f,   // ajusta pro seu sensor
//         .sample_ms         = 1000,     // janela de cálculo (ms)
//         .glitch_us         = 100,      // anti-ruído (us)
//         .use_internal_pullup = true    // ideal ter pull-up externo de 10k em 3V3 também
//     };

//     err = sensor_fluxo_init(&cfg);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "sensor_fluxo_init falhou: %s", esp_err_to_name(err));
//         return;
//     }

//     // ---------- CONFIG CAN ----------
//     err = can_transmiter_init();
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "CAN init falhou: %s", esp_err_to_name(err));
//         return;
//     }

//     ESP_LOGI(TAG, "Sistema iniciado: CSV pela serial + envio via CAN...");

//     // --------- CABEÇALHO CSV (uma vez só) ---------
//     // Formato: temp_c;flow_l_min;total_liters;pulses_window
//     printf("temp_c;flow_l_min;total_liters;pulses_window\r\n");
//     fflush(stdout);

//     while (1) {
//         float t1 = 0.0f;
//         sensor_fluxo_metrics_t m = (sensor_fluxo_metrics_t){0};

//         // --------- TEMPERATURA ---------
//         if (ds18b20_read_celsius(&temp1, &t1) != ESP_OK) {
//             ESP_LOGW(TAG, "falha ao ler temp1");
//             t1 = -999.0f;  // marca erro no CSV
//         }

//         // --------- FLUXO ---------
//         sensor_fluxo_get_metrics(&m);
// dgbd
//         // --------- LINHA CSV PARA PYTHON ---------
//         printf("%.2f;%.3f;%.3f;%" PRIu32 "\r\n",
//                t1,
//                (double)m.flow_l_min,
//                (double)m.total_liters,
//                m.pulses_window);
//         fflush(stdout);

//         // --------- ENVIO POR CAN ---------
//         // temp2 por enquanto = temp1 (até você ligar um segundo DS18)
//         float temp2 = t1;
//         err = can_enviar_temp_e_total_compacto(t1, temp2, m.total_liters);
//         if (err != ESP_OK) {
//             ESP_LOGW(TAG, "Falha ao enviar quadro CAN");
//         }

//         // Log básico local
//         ESP_LOGI(TAG, "T=%.2f °C | flow=%.3f L/min | total=%.3f L",
//                  t1, (double)m.flow_l_min, (double)m.total_liters);

//         vTaskDelay(pdMS_TO_TICKS(1000));  // 1 amostra / frame por segundo
//     }
// }




// #include <stdio.h>
// #include <inttypes.h>              // PRIu32
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"

// #include "sensor_temp.h"           // driver DS18B20
// #include "sensor_fluxo.h"          // driver de fluxo

// static const char *TAG = "app_main";

// // Pinos escolhidos
// #define DS18_PIN    GPIO_NUM_4     // DS18B20 no GPIO4
// #define FLUXO_PIN   GPIO_NUM_14    // Sensor de fluxo no GPIO14

// void app_main(void)
// {
//     esp_err_t err;

//     vTaskDelay(pdMS_TO_TICKS(100));   // pequena estabilização

//     // ---------- CONFIG DS18B20 ----------
//     ds18b20_t temp1;

//     err = ds18b20_init(&temp1, DS18_PIN, false);   // VDD em 3V3, sem modo parasita
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "DS18B20 init falhou: %s", esp_err_to_name(err));
//         return;
//     }

//     (void) ds18b20_set_resolution(&temp1, 12);     // 12 bits (~750 ms conversão)

//     // ---------- CONFIG FLUXO (igual teu exemplo) ----------
//     sensor_fluxo_config_t cfg = {
//         .pin               = FLUXO_PIN,
//         .pulses_per_liter  = 450.0f,   // ajusta pro seu sensor
//         .sample_ms         = 1000,     // janela de cálculo (ms)
//         .glitch_us         = 100,      // anti-ruído (us)
//         .use_internal_pullup = true    // ideal ter pull-up externo de 10k em 3V3 também
//     };

//     err = sensor_fluxo_init(&cfg);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "sensor_fluxo_init falhou: %s", esp_err_to_name(err));
//         return;
//     }

//     ESP_LOGI(TAG, "Sistema iniciado: lendo temperatura (GPIO4) e fluxo (GPIO14)...");

//     while (1) {
//         float t1 = 0.0f;
//         sensor_fluxo_metrics_t m = {0};

//         // --------- TEMPERATURA ---------
//         if (ds18b20_read_celsius(&temp1, &t1) != ESP_OK) {
//             ESP_LOGW(TAG, "falha ao ler temp1");
//         }

//         // --------- FLUXO ---------
//         sensor_fluxo_get_metrics(&m);

//         ESP_LOGI(TAG,
//                  "T=%.2f °C | pulses=%" PRIu32 " | flow=%.3f L/min | total=%.3f L",
//                  t1,
//                  m.pulses_window,
//                  (double)m.flow_l_min,
//                  (double)m.total_liters);

//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }


#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rotacao.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "app";

// === Ajustes do projeto ===
#define RPM_PIN             GPIO_NUM_27   // escolha um GPIO "seguro" para entrada
#define PULSES_PER_REV      2             // nº de pulsos por volta mecânica
#define MIN_PULSE_US        500        // anti-ruído: rejeita bordas mais rápidas que isso
#define SAMPLE_WINDOW_MS    1000          // janela de amostragem p/ cálculo de RPM

// === Parâmetros do filtro exponencial ===
#define RPM_FILTER_ALPHA    0.20f         // 0.1 = mais suave; 0.5 = mais responsivo

void rpm_task(void *arg)
{
    rpm_counter_t *rc = (rpm_counter_t*)arg;

    // estado do filtro
    static float rpm_filt = 0.0f;
    static bool  filt_init = false;

    ESP_LOGI(TAG, "rpm_task iniciada, janela=%d ms, pulses/rev=%u",
             SAMPLE_WINDOW_MS, rc->cfg.pulses_per_rev);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_WINDOW_MS));

        uint32_t pulses = rpm_counter_read_and_reset(rc);
        uint32_t rpm    = rpm_counter_to_rpm(pulses, SAMPLE_WINDOW_MS,
                                             rc->cfg.pulses_per_rev);

        // inicializa filtro com primeira medida
        if (!filt_init) {
            rpm_filt = (float)rpm;
            filt_init = true;
        } else {
            rpm_filt = RPM_FILTER_ALPHA * (float)rpm
                     + (1.0f - RPM_FILTER_ALPHA) * rpm_filt;
        }

        // 1) Linha "limpa" para o Python: RAW;FILT
        //    Ex.: 1234;1180.5
        printf("%" PRIu32 ";%.1f\n", rpm, (double)rpm_filt);

        // 2) Log informativo no monitor
        ESP_LOGI(TAG, "RPM raw=%" PRIu32 " | RPM filt=%.1f (pulses=%" PRIu32 ")",
                 rpm, (double)rpm_filt, pulses);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando leitura de RPM...");

    rpm_counter_t rc;
    rpm_counter_cfg_t cfg = {
        .pin            = RPM_PIN,
        .pulses_per_rev = PULSES_PER_REV,
        .min_pulse_us   = MIN_PULSE_US,
        .pullup         = true,     // habilita PULLUP interno
        .rising_edge    = true
    };

    esp_err_t err = rpm_counter_init(&rc, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha em rpm_counter_init(): %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "rpm_counter_init OK");

    err = rpm_counter_start(&rc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha em rpm_counter_start(): %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "rpm_counter_start OK");

    if (xTaskCreatePinnedToCore(rpm_task,
                                "rpm_task",
                                4096,      // mais stack pra garantir
                                &rc,
                                5,
                                NULL,
                                tskNO_AFFINITY) != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar tarefa RPM");
        return;
    }

    ESP_LOGI(TAG, "Tarefa RPM criada com sucesso");
    // app_main pode retornar; rpm_task continua rodando
}

// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"

// #include "sensor_temp.h"       // DS18B20
// #include "sensor_fluxo.h"      // teu driver já funcional
// #include "can_transmiter.h"    // abaixo

// #define TAG "APP"

// // dois sensores de temp
// #define DS18_PIN_1  GPIO_NUM_33
// #define DS18_PIN_2  GPIO_NUM_32
// void app_main(void)
// {
//     esp_err_t err;

//     vTaskDelay(pdMS_TO_TICKS(100));

//     // ---------- TEMPERATURA ----------
//     ds18b20_t temp1;
//     ds18b20_t temp2;

//     err = ds18b20_init(&temp1, DS18_PIN_1, false);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "DS18B20 #1 init falhou: %s", esp_err_to_name(err));
//         return;
//     }

//     err = ds18b20_init(&temp2, DS18_PIN_2, false);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "DS18B20 #2 init falhou: %s", esp_err_to_name(err));
//         return;
//     }

//     (void) ds18b20_set_resolution(&temp1, 12);
//     (void) ds18b20_set_resolution(&temp2, 12);

//     // ---------- FLUXO (só quero o total) ----------
//     sensor_fluxo_config_t fcfg = {
//         .pin                 = GPIO_NUM_35,   // AJUSTA
//         .pulses_per_liter    = 450.0f,       // AJUSTA
//         .sample_ms           = 1000,
//         .glitch_us           = 100,
//         .use_internal_pullup = true
//     };

//     err = sensor_fluxo_init(&fcfg);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "sensor_fluxo_init falhou: %s", esp_err_to_name(err));
//         return;
//     }

//     // ---------- CAN ----------
//     err = can_transmiter_init();
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "CAN init falhou: %s", esp_err_to_name(err));
//         return;
//     }

//     ESP_LOGI(TAG, "Sistema iniciado. Enviando 2x temp + total_liters via CAN...");

//     while (1) {
//         float t1 = 0.0f;
//         float t2 = 0.0f;
//         sensor_fluxo_metrics_t m = {0};

//         // temperaturas
//         if (ds18b20_read_celsius(&temp1, &t1) != ESP_OK) {
//             ESP_LOGW(TAG, "falha ler temp1");
//         }
//         if (ds18b20_read_celsius(&temp2, &t2) != ESP_OK) {
//             ESP_LOGW(TAG, "falha ler temp2");
//         }

//         // fluxo (mas vou usar só o total)
//         sensor_fluxo_get_metrics(&m);

//         ESP_LOGI(TAG,
//                  "T1=%.2f °C | T2=%.2f °C | total=%.3f L",
//                  t1, t2, (double)m.total_liters);

//         // 1) envia temperaturas
//         // can_enviar_temp_e_total_compacto(t1, t2, m.total_liters);

//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }





// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "mpu6050.h"
// #include "gps.h"
// #include "can_transmiter.h"  // nome correto
// #include "sensor_temp.h"
// #include "esp_log.h"


// extern float gps_latitude;
// extern float gps_longitude;

// void app_main(void) {
//     printf("Iniciando sensores...\n");
    
//     ds18b20_t sensor;
//     ESP_ERROR_CHECK(ds18b20_init(&sensor, GPIO_NUM_4, false)); // DQ no GPIO4
//     // opcional: ESP_ERROR_CHECK(ds18b20_set_resolution(&sensor, 12));

//     // if (mpu6050_init() != ESP_OK) {
//     //     printf("Falha ao inicializar o MPU6050\n");
//     //     return;
//     // }

//     // gps_init();
//     // gps_start_task();

//     // if (can_transmiter_init() != ESP_OK) {
//     //     printf("Falha ao inicializar CAN\n");
//     //     return;
//     // }

//     while (1) {
//         float t = 0.0f;
//         esp_err_t err = ds18b20_read_temperature(&sensor, &t);
//         if (err == ESP_OK) {
//             ESP_LOGI("APP", "Temperatura: %.2f °C", t);
//         } else {
//             ESP_LOGE("APP", "Falha ao ler temperatura (%s)", esp_err_to_name(err));
//         }
//         vTaskDelay(pdMS_TO_TICKS(1000));
//         // int16_t ax, ay, az;
//         // if (mpu6050_read_accel(&ax, &ay, &az) == ESP_OK) {
//         //     printf("Accel X: %d, Y: %d, Z: %d\n", ax, ay, az);
//         //     printf("Latitude: %.6f, Longitude: %.6f\n", gps_latitude, gps_longitude);
//         //     can_enviar_dados(ax, ay, az, gps_latitude, gps_longitude);
//         // } else {
//         //     printf("Erro na leitura do MPU6050\n");
//         // }
//         // vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }


// #include "sensor_temp.h"
// #include "esp_log.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"

// #define APP_TAG   "APP"
// #define DS18_PIN  GPIO_NUM_33  // já está OK no seu hardware

// void app_main(void) {
//     vTaskDelay(pdMS_TO_TICKS(100)); // estabiliza

//     ds18b20_t sensor;
//     esp_err_t err = ds18b20_init(&sensor, DS18_PIN, false); // VDD=3V3 (sem parasita)
//     if (err != ESP_OK) {
//         ESP_LOGE(APP_TAG, "init falhou: %s", esp_err_to_name(err));
//         return;
//     }

//     // opcional: define resolução (12 bits = 750 ms)
//     err = ds18b20_set_resolution(&sensor, 12);
//     if (err != ESP_OK) {
//         ESP_LOGW(APP_TAG, "nao consegui setar resolucao: %s (seguindo default)", esp_err_to_name(err));
//     }

//     while (1) {
//         float c = 0.0f;
//         err = ds18b20_read_celsius(&sensor, &c);
//         if (err == ESP_OK) {
//             ESP_LOGI(APP_TAG, "Temperatura: %.2f °C", c);
//         } else {
//             ESP_LOGE(APP_TAG, "Falha ao ler temperatura (%s)", esp_err_to_name(err));
//         }
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }
// #include <stdio.h>
// #include "sensor_temp.h"
// #include "esp_log.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"

// #define APP_TAG   "APP"
// #define DS18_PIN  GPIO_NUM_4   // ajuste conforme o pino real do seu hardware

// void app_main(void) {
//     vTaskDelay(pdMS_TO_TICKS(100)); // estabiliza o sistema

//     ds18b20_t sensor;
//     esp_err_t err = ds18b20_init(&sensor, DS18_PIN, false); // VDD = 3V3 (sem modo parasita)
//     // if (err != ESP_OK) {
//     //     ESP_LOGE(APP_TAG, "Falha na inicialização: %s", esp_err_to_name(err));
//     //     return;
//     // }

//     // Define resolução opcionalmente (12 bits = ~750 ms de conversão)
//     err = ds18b20_set_resolution(&sensor, 12);
//     if (err != ESP_OK) {
//         ESP_LOGW(APP_TAG, "Não consegui definir resolução: %s (seguindo padrão)", esp_err_to_name(err));
//     }

//     ESP_LOGI(APP_TAG, "Iniciando leitura contínua do sensor DS18B20...");

//     // === LOOP INFINITO DE LEITURA ===
//     while (1) {
//         float temperatura = 0.0f;
//         err = ds18b20_read_celsius(&sensor, &temperatura);

//         if (err == ESP_OK) {
//             ESP_LOGI(APP_TAG, "Temperatura: %.2f °C", temperatura);
//         } else {
//             ESP_LOGE(APP_TAG, "Falha ao ler temperatura (%s)", esp_err_to_name(err));
//         }

//         // Intervalo de 1 segundo entre leituras
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }


// #include <stdio.h>
// #include <inttypes.h>              // <- para PRIu32
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"
// #include "sensor_fluxo.h"

// static const char *TAG = "app_main";

// void app_main(void)
// {
//     sensor_fluxo_config_t cfg = {
//         .pin = GPIO_NUM_5,          // ajuste o pino
//         .pulses_per_liter = 450.0f,  // ajuste o PPL do seu sensor
//         .sample_ms = 1000,           // janela de cálculo (ms)
//         .glitch_us = 100,            // anti-ruído (us)
//         .use_internal_pullup = true  // ideal ter pull-up externo de 10k para 3V3 também
//     };

//     esp_err_t err = sensor_fluxo_init(&cfg);
//     if (err != ESP_OK) {
//         ESP_LOGI(TAG, "sensor_fluxo_init falhou: %s", esp_err_to_name(err));
//         return;
//     }

//     while (1) {
//         sensor_fluxo_metrics_t m;
//         sensor_fluxo_get_metrics(&m);

//         // pulses_window é uint32_t -> use PRIu32
//         ESP_LOGI(TAG, "pulses=%" PRIu32 " | flow=%.3f L/min | total=%.3f L",
//                  m.pulses_window, (double)m.flow_l_min, (double)m.total_liters);

//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }


