#include <SPI.h>
#include <ACAN2515.h>

// Definição dos pinos do MCP2515
static const byte MCP2515_SCK = 13; // SCK input of MCP2515
static const byte MCP2515_SI = 11;  // SI input of MCP2515
static const byte MCP2515_SO = 12;  // SO output of MCP2515
static const byte MCP2515_CS = 10;  // CS input of MCP2515
static const byte MCP2515_INT = 3;  // INT output of MCP2515

// Criando objeto da biblioteca ACAN2515
ACAN2515 can(MCP2515_CS, SPI, MCP2515_INT);

// Frequência do cristal do MCP2515 (verifique se é 8MHz ou 16MHz)
static const uint32_t QUARTZ_FREQUENCY = 8UL * 1000UL * 1000UL; // 8 MHz

// Taxa de transmissão da rede CAN
static const uint32_t CAN_BIT_RATE = 500UL * 1000UL; // 500 kbps

#define SENSOR_PIN 2  // Pino digital conectado ao sinal do sensor
volatile unsigned int contadorPulsos = 0;

// Configurações físicas da roda fônica
const float raioRoda = 0.3; // Raio da roda em metros (exemplo: 30 cm)
const unsigned int furosPorRevolucao = 1; // Número de furos por revolução da roda fônica

void setup() {
    // Configuração do sensor Hall
    pinMode(SENSOR_PIN, INPUT_PULLUP);  // Configura o pino com pull-up interno
    attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), contarPulsos, FALLING); // Configura interrupção

    // Inicializa comunicação serial
    Serial.begin(115200);
    while (!Serial);

    Serial.println("Iniciando MCP2515...");

    // Inicializa SPI
    SPI.begin();

    // Configuração do MCP2515
    ACAN2515Settings settings(QUARTZ_FREQUENCY, CAN_BIT_RATE);
    settings.mRequestedMode = ACAN2515Settings::NormalMode; // Modo normal para comunicação real

    const uint32_t errorCode = can.begin(settings, [] { can.isr(); });

    if (errorCode == 0) {
        Serial.println("MCP2515 configurado com sucesso!");
    } else {
        Serial.print("Erro na configuração do MCP2515: 0x");
        Serial.println(errorCode, HEX);
        while (1);
    }

    Serial.println("Sistema iniciado.");
}

void loop() {
    // Atualiza a contagem de pulsos e calcula a velocidade a cada segundo
    static unsigned long ultimoTempo = 0;
    unsigned long tempoAtual = millis();

    if (tempoAtual - ultimoTempo >= 1000) {
        // Calcula a velocidade
        float circunferencia = 2 * 3.14159 * raioRoda; // Comprimento da circunferência
        float revolucoesPorSegundo = (float)contadorPulsos / furosPorRevolucao;
        float velocidade = revolucoesPorSegundo * circunferencia; // Velocidade em metros por segundo

        // Converte para km/h
        velocidade *= 3.6;

        // Cria uma mensagem CAN com a velocidade
        CANMessage frame;
        frame.id = 0x13; // ID da mensagem (ajuste conforme necessário)
        frame.len = 2;   // Número de bytes (2 bytes para a velocidade em km/h)
        frame.ext = false; // Mensagem padrão (não estendida)

        // Converte a velocidade para inteiro (multiplicado por 100 para preservar duas casas decimais)
        int16_t velocidadeInt = (int16_t)(velocidade * 100);

        // Preenche os dados da mensagem
        frame.data[0] = (velocidadeInt >> 8) & 0xFF; // Byte alto da velocidade
        frame.data[1] = velocidadeInt & 0xFF;        // Byte baixo da velocidade

        // Envia a mensagem CAN
        if (can.tryToSend(frame)) {
            Serial.print("Mensagem CAN enviada com velocidade: ");
            Serial.print(velocidade);
            Serial.println(" km/h");
        } else {
            Serial.println("Falha ao enviar mensagem CAN!");
        }

        // Reseta o contador de pulsos
        contadorPulsos = 0;
        ultimoTempo = tempoAtual;
    }
}

void contarPulsos() {
    contadorPulsos++; // Incrementa o contador de pulsos a cada interrupção
}

