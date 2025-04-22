# 📡 Projeto de Telemetria CAN com ESP32

Este projeto realiza a leitura de dados do acelerômetro MPU6050 e do módulo GPS NEO-M8N, transmitindo-os via protocolo CAN utilizando dois ESP32 e módulos com CI SN65HVD230. O objetivo é facilitar a coleta de dados em tempo real em ambientes como veículos off-road (ex: Baja SAE).

---

## 📁 Estrutura de Diretórios Recomendada

```bash
project-name/
├── CMakeLists.txt
├── sdkconfig
├── sdkconfig.defaults
├── main/
│   ├── main.c
│   └── CMakeLists.txt
├── components/
│   ├── mpu6050/
│   │   ├── mpu6050.c
│   │   ├── mpu6050.h
│   │   └── CMakeLists.txt
│   ├── gps/
│   │   ├── gps.c
│   │   ├── gps.h
│   │   └── CMakeLists.txt
│   └── can_transmiter/
│       ├── can_transmiter.c
│       ├── can_transmiter.h
│       └── CMakeLists.txt
├── .gitignore
└── README.md
```

> ❗ **Atenção:** A pasta `build/` gerada na compilação não deve ser versionada. Certifique-se de incluí-la no `.gitignore`.

---

## 💻 Requisitos para Execução

### 🧰 Instalações Necessárias

1. [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
2. Python 3 (recomendado usar virtualenv)
3. Dependências via pip:
   ```bash
   pip install esptool pyserial
   ```

### 📦 Drivers (Windows)

- Driver USB-to-Serial para ESP32 (CH340 ou CP2102)

---

## 🛠️ Como Compilar e Rodar

1. Configure o ambiente:
   ```bash
   . $HOME/esp/esp-idf/export.sh
   ```

2. Navegue até o diretório do projeto:
   ```bash
   cd project-name
   ```

3. Compile e carregue:
   ```bash
   idf.py build
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

---

## 🚐 Funcionamento da Comunicação CAN

A rede CAN (Controller Area Network) é utilizada para comunicação entre microcontroladores, permitindo troca de dados em tempo real, com alta confiabilidade e resistência a ruído.

### ✅ Características:

- Comunicação serial diferencial (alta imunidade a interferências)
- Taxa usada: **500 kbps**
- Até **8 bytes por frame CAN**
- Prioridade de mensagens baseada no ID (menor ID = maior prioridade)

### 🔧 Aplicação neste Projeto:

Dois ESP32 comunicam-se via CAN:

- O **Transmissor** envia dados do MPU6050 e GPS usando dois frames:
  - `ID 0x123`: Aceleração X, Y, Z
  - `ID 0x124`: Latitude e Longitude (tipo `float`, codificados em IEEE 754)

- O **Receptor** decodifica os frames recebidos e imprime os valores no terminal.

> ⚠️ Os módulos CAN (SN65HVD230) devem ser ligados com resistores de terminação de 120 Ω entre CAN_H e CAN_L.

---

## 📌 Observações

- O GPS deve estar em ambiente aberto para obter sinal.
- Certifique-se de alimentar corretamente os módulos. O GPS e o CAN podem exigir corrente acima de 100 mA.
- Evite subir arquivos como `build/` e `.pioenvs/` ao repositório. Use `.gitignore` corretamente.

---

## 📷 Exemplo de Saída

```
Accel X: -520, Y: 256, Z: 16234
Latitude: -30.0457
Longitude: -51.2309
```

---

## 📍 Próximos Passos

- Envio dos dados via rede GSM (SIM900A)
- Visualização em dashboard web
- Armazenamento local (SD Card) ou remoto (MQTT, REST API)

---

Desenvolvido por **Eduardo Schvinn**  
Projeto para aplicação em competições **Baja SAE** 🏁
