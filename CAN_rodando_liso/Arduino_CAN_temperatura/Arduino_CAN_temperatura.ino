#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <SPI.h>
#include <ACAN2515.h>

// Cria um objeto para o sensor MLX90614
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// Definição dos pinos do MCP2515
static const byte MCP2515_CS = 10; // CS input of MCP2515
static const byte MCP2515_INT = 3; // INT output of MCP2515

// Criando objeto da biblioteca ACAN2515
ACAN2515 can(MCP2515_CS, SPI, MCP2515_INT);

// Frequência do cristal do MCP2515 (verifique se é 8MHz ou 16MHz)
static const uint32_t QUARTZ_FREQUENCY = 8UL * 1000UL * 1000UL; // 8 MHz

// Taxa de transmissão da rede CAN
static const uint32_t CAN_BIT_RATE = 500UL * 1000UL; // 500 kbps

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("Iniciando...");

    // Inicializa o sensor MLX90614
    if (!mlx.begin()) {
        Serial.println("Erro ao inicializar o sensor MLX90614. Verifique as conexões.");
        while (1);
    }
    Serial.println("Sensor MLX90614 iniciado com sucesso!");

    // Inicializa SPI
    SPI.begin();

    // Configuração do MCP2515
    Serial.println("Configurando MCP2515...");
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
    // Lê a temperatura do objeto
    float tempObjeto = mlx.readObjectTempC();

    // Exibe o resultado no Monitor Serial
    Serial.print("Temperatura do Objeto: ");
    Serial.print(tempObjeto);
    Serial.println(" °C");

    // Criando a mensagem CAN
    CANMessage frame;
    frame.id = 0x12; // ID padrão de 11 bits
    frame.len = 2;   // 2 bytes (para a temperatura do objeto)
    frame.ext = false; // Mensagem padrão (não estendida)

    // Converte a temperatura para um inteiro multiplicado por 100 (para preservar 2 casas decimais)
    int16_t tempObjetoInt = (int16_t)(tempObjeto * 100);

    // Preenche os dados da mensagem
    frame.data[0] = (tempObjetoInt >> 8) & 0xFF; // Byte alto da temperatura do objeto
    frame.data[1] = tempObjetoInt & 0xFF;        // Byte baixo da temperatura do objeto

    // Envia a mensagem CAN
    if (can.tryToSend(frame)) {
        Serial.println("Mensagem CAN enviada com sucesso!");
    } else {
        Serial.println("Falha ao enviar mensagem CAN!");
    }

    delay(1000); // Aguarda 1 segundo antes de repetir
}

