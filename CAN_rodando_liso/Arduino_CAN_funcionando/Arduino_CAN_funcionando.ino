#include <SPI.h>
#include <ACAN2515.h>

// Definição dos pinos do MCP2515
static const byte MCP2515_SCK = 13 ; // SCK input of MCP2515
static const byte MCP2515_SI  = 11 ; // SI input of MCP2515
static const byte MCP2515_SO  = 12 ; // SO output of MCP2515

static const byte MCP2515_CS  = 10 ; // CS input of MCP2515
static const byte MCP2515_INT = 3 ; // INT output of MCP2515

// Criando objeto da biblioteca ACAN2515
ACAN2515 can(MCP2515_CS, SPI, MCP2515_INT);

// Frequência do cristal do MCP2515 (verifique se é 8MHz ou 16MHz)
static const uint32_t QUARTZ_FREQUENCY = 8UL * 1000UL * 1000UL; // 8 MHz

// Taxa de transmissão da rede CAN
static const uint32_t CAN_BIT_RATE = 500UL * 1000UL; // 500 kbps

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("Iniciando MCP2515...");

    // Inicializa SPI
    SPI.begin();

    // Configuração do MCP2515
    ACAN2515Settings settings(QUARTZ_FREQUENCY, CAN_BIT_RATE);
    settings.mRequestedMode = ACAN2515Settings::NormalMode; // Modo normal para comunicação real

    // Inicia o MCP2515 com a configuração
    const uint32_t errorCode = can.begin(settings, [] { can.isr(); });

    if (errorCode == 0) {
        Serial.println("MCP2515 configurado com sucesso!");
    } else {
        Serial.print("Erro na configuração do MCP2515: 0x");
        Serial.println(errorCode, HEX);
        while (1);
    }
}

void loop() {
    // Criando a mensagem CAN
    CANMessage frame;
    frame.id = 0x12; // ID padrão de 11 bits (pode ser alterado conforme necessário)
    frame.len = 5;   // Número de bytes na mensagem
    frame.ext = false; // Mensagem padrão (não estendida)

    // Dados da mensagem
    frame.data[0] = 'H';
    frame.data[1] = 'e';
    frame.data[2] = 'l';
    frame.data[3] = 'l';
    frame.data[4] = 'o';

    // Tentando enviar a mensagem
    if (can.tryToSend(frame)) {
        Serial.println("Mensagem CAN enviada com sucesso!");
    } else {
        Serial.println("Falha ao enviar mensagem CAN!");
    }

    delay(1000); // Aguarda 1 segundo antes de enviar novamente
}

