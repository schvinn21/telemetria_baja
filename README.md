# Explicação Detalhada sobre a Rede CAN e sua Implementação no Projeto

## Introdução à Rede CAN

A **Controller Area Network (CAN)** é um protocolo de comunicação serial desenvolvido pela Bosch, amplamente utilizado em aplicações automotivas, industriais e embarcadas. Diferente de redes convencionais, a CAN opera com um barramento compartilhado, onde múltiplos dispositivos podem enviar e receber mensagens sem a necessidade de um mestre controlador.

### Princípios Fundamentais da Rede CAN

- **Mensagens Baseadas em Identificadores:** Cada mensagem possui um identificador (ID) único que define sua prioridade.
- **Arbitragem por Prioridade:** Se dois dispositivos tentam transmitir simultaneamente, aquele com menor valor binário no ID tem prioridade.
- **Detecção de Erros:** Inclui verificação de erros por meio de CRC (Cyclic Redundancy Check) e bits de confirmação (ACK).
- **Velocidade:** A taxa de transmissão pode chegar a 1 Mbps em curtas distâncias, sendo comum o uso de 500 kbps em veículos.

## Estrutura de um Quadro CAN

O protocolo CAN define dois tipos principais de quadros de dados: o formato padrão (11 bits de ID) e o formato estendido (29 bits de ID). A estrutura de um quadro padrão é mostrada abaixo:

| **Campo**                                | **Tamanho**  | **Descrição**                                      |
|------------------------------------------|-------------|---------------------------------------------------|
| **Start of Frame**                       | 1 bit       | Indica o início da mensagem                      |
| **Identificador (ID)**                    | 11 bits     | Define a prioridade da mensagem                  |
| **RTR (Remote Transmission Request)**     | 1 bit       | Indica se é uma requisição ou dado               |
| **Controle**                              | 6 bits      | Indica o tamanho da mensagem                     |
| **Dados**                                 | 0-8 bytes   | Informação real transmitida                      |
| **CRC (Cyclic Redundancy Check)**         | 15 bits     | Verificação de erros                             |
| **ACK (Acknowledgment)**                  | 1 bit       | Indica se a mensagem foi recebida corretamente   |
| **End of Frame**                          | 7 bits      | Marca o fim da mensagem                          |

## Implementação da Rede CAN no Projeto

O projeto utiliza **Arduino** com **MCP2515** e **ESP32**, cada um desempenhando uma função específica dentro da rede CAN.

### Comunicação Entre os Nós

- **Arduino + MCP2515 (Transmissor)**
  - Envia dados para o barramento CAN.
  - Envia temperatura usando o sensor MLX90614 ou a velocidade com um sensor Hall.
  - Utiliza identificadores CAN distintos:
    - **ID 0x12**: Dados de temperatura.
    - **ID 0x13**: Dados de velocidade.

- **ESP32 (Receptor)**
  - Recebe mensagens CAN e interpreta os dados.
  - Exibe a temperatura e a velocidade no Monitor Serial.

### Código de Envio de Mensagem (Arduino)

```cpp
CANMessage frame;
frame.id = 0x12;  // ID da mensagem (Temperatura)
frame.len = 2;     // 2 bytes de dados
frame.data[0] = (tempObjetoInt >> 8) & 0xFF;
frame.data[1] = tempObjetoInt & 0xFF;
can.tryToSend(frame);
```

### Código de Recebimento de Mensagem (ESP32)

```cpp
CANMessage frame;
if (ACAN_ESP32::can.receive(frame)) {
    if (frame.id == 0x12 && frame.len == 2) {
        int16_t tempObjetoInt = (frame.data[0] << 8) | frame.data[1];
        float tempObjeto = tempObjetoInt / 100.0;
        Serial.print("Temperatura do Objeto: ");
        Serial.print(tempObjeto);
        Serial.println(" °C");
    }
}
```

## Melhorias no Projeto

Algumas melhorias a serem implementadas para aprimorar a telemetria do projeto:

- **Adição de um Gateway ESP32:** Um nó intermediário que coleta os dados e os envia via Wi-Fi para um servidor.
- **Armazenamento dos Dados:** Registro das mensagens CAN em um cartão SD ou banco de dados online.
- **Visualização em Tempo Real:** Uso de **Streamlit** para criar um painel interativo com os dados coletados.
- **Redundância e Verificação de Erros:** Implementação de checksums extras para maior confiabilidade.


