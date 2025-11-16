import serial
import matplotlib.pyplot as plt
from collections import deque
import csv
import time

# === CONFIGURAÇÕES ===
PORTA_SERIAL = 'COM9'        # ajuste para a porta do ESP32 (COMx)
BAUDRATE = 115200
TAMANHO_JANELA = 100         # número de pontos exibidos no gráfico
ARQUIVO_CSV = 'dados_fluxo_temp_com_ruido.csv'

# Conecta à porta serial
ser = serial.Serial(PORTA_SERIAL, BAUDRATE)
print(f"Conectado à {PORTA_SERIAL}...")

# Cria / abre o arquivo CSV e escreve cabeçalho
csv_file = open(ARQUIVO_CSV, mode='w', newline='', encoding='utf-8')
csv_writer = csv.writer(csv_file, delimiter=';')
csv_writer.writerow(['timestamp_ms', 'temp_c', 'flow_l_min', 'total_liters', 'pulses_window'])
csv_file.flush()

# Buffers para plot
tempo = deque(maxlen=TAMANHO_JANELA)
temp_buf = deque(maxlen=TAMANHO_JANELA)
flow_buf = deque(maxlen=TAMANHO_JANELA)

# Configura o gráfico
plt.ion()
fig, ax1 = plt.subplots()

# Temperatura no eixo esquerdo
linha_temp, = ax1.plot([], [], label='Temperatura (°C)')
ax1.set_xlabel("Amostras")
ax1.set_ylabel("Temperatura (°C)")

# Fluxo no eixo direito
ax2 = ax1.twinx()
linha_flow, = ax2.plot([], [], label='Fluxo (L/min)', color='orange')
ax2.set_ylabel("Fluxo (L/min)")

fig.legend(loc="upper right")
plt.title("Temperatura e Fluxo em tempo real")

print("Lendo dados... (Ctrl+C para parar)")

amostra_idx = 0
t0 = time.time()  # referência para timestamp em ms

try:
    while True:
        linha = ser.readline().decode('utf-8', errors='ignore').strip()

        # Ignora linhas vazias
        if not linha:
            continue

        # Ignora cabeçalho vindo do ESP e logs do ESP-IDF
        if linha.startswith("temp_c") or linha.startswith("I (") or "gpio:" in linha:
            # opcional: descomente para ver o que está sendo ignorado
            # print("Ignorada:", linha)
            continue

        # Espera formato: temp_c;flow_l_min;total_liters;pulses_window
        partes = linha.split(';')

        # Aceita 4+ campos, pega só os 4 primeiros
        if len(partes) < 4:
            print("Linha inesperada (poucos campos):", linha)
            continue

        try:
            temp_c = float(partes[0])
            flow_l_min = float(partes[1])
            total_liters = float(partes[2])
            pulses_window = int(partes[3])
        except ValueError:
            print("Falha ao converter linha:", linha)
            continue

        # Gera timestamp em ms no Python
        timestamp_ms = int((time.time() - t0) * 1000)

        # Escreve no CSV
        csv_writer.writerow([timestamp_ms, temp_c, flow_l_min, total_liters, pulses_window])
        csv_file.flush()

        # Atualiza buffers de plot
        amostra_idx += 1
        tempo.append(amostra_idx)
        temp_buf.append(temp_c)
        flow_buf.append(flow_l_min)

        # Atualiza curvas
        linha_temp.set_xdata(range(len(temp_buf)))
        linha_temp.set_ydata(temp_buf)

        linha_flow.set_xdata(range(len(flow_buf)))
        linha_flow.set_ydata(flow_buf)

        ax1.relim()
        ax1.autoscale_view()

        ax2.relim()
        ax2.autoscale_view()

        plt.draw()
        plt.pause(0.01)

except KeyboardInterrupt:
    print("\nEncerrando...")

finally:
    ser.close()
    csv_file.close()
    plt.ioff()
    plt.show()
