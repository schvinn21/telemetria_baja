#include <ACAN_ESP32.h>

static const uint32_t DESIRED_BIT_RATE = 500UL * 1000UL; // 500 kbps

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("ESP32 CAN: Iniciando...");

    // Configuração do CAN no ESP32
    ACAN_ESP32_Settings settings(DESIRED_BIT_RATE);
    settings.mRequestedCANMode = ACAN_ESP32_Settings::NormalMode;
    settings.mRxPin = GPIO_NUM_4; // RX do ESP32 (recebe dados do barramento CAN)
    settings.mTxPin = GPIO_NUM_5; // TX do ESP32 (envia dados para o barramento CAN)

    const uint32_t errorCode = ACAN_ESP32::can.begin(settings);

    if (errorCode == 0) {
        Serial.println("ESP32 CAN: Configuração bem-sucedida!");
    } else {
        Serial.print("ESP32 CAN: Erro de configuração 0x");
        Serial.println(errorCode, HEX);
        while (1);
    }
}

void loop() {
    CANMessage frame;

    // Verifica se há uma mensagem CAN disponível
    if (ACAN_ESP32::can.receive(frame)) {
        Serial.println("\n=== Mensagem Recebida ===");
        Serial.print("ID: 0x");
        Serial.println(frame.id, HEX);

        // Decodifica as mensagens com base no ID
        if (frame.id == 0x12 && frame.len == 2) {
            // Mensagem de temperatura do objeto (ID 0x12)
            int16_t tempObjetoInt = (frame.data[0] << 8) | frame.data[1];
            float tempObjeto = tempObjetoInt / 100.0;

            Serial.print("Temperatura do Objeto: ");
            Serial.print(tempObjeto);
            Serial.println(" °C");
        } else if (frame.id == 0x13 && frame.len == 2) {
            // Mensagem de contagem de pulsos (ID 0x13)
            uint16_t contadorPulsos = (frame.data[0] << 8) | frame.data[1];

            Serial.print("Contador de Pulsos: ");
            Serial.println(contadorPulsos);
        } else {
            Serial.println("Mensagem com ID ou tamanho inesperado!");
        }
    }

    delay(100); // Pequeno atraso para evitar sobrecarga no loop
}

