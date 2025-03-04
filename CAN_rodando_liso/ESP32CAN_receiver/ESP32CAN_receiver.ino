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
        Serial.print("Tipo: ");
        Serial.println(frame.ext ? "Extended" : "Standard");

        Serial.print("Dados: ");
        for (uint8_t i = 0; i < frame.len; i++) {
            Serial.print((char)frame.data[i]); // Exibe os caracteres recebidos
        }
        Serial.println();
    }

    delay(100); // Pequeno atraso para evitar sobrecarga no loop
}

