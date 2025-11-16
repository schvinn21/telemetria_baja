import serial
import matplotlib.pyplot as plt
from collections import deque
import csv
import time

# === CONFIGURAÇÕES ===
PORTA_SERIAL = 'COM9'        # ajuste para a porta do ESP32
BAUDRATE = 115200
TAMANHO_JANELA = 200         # número de pontos exibidos no gráfico
ARQUIVO_CSV = 'dados_rpm_com_filtrado.csv'

# === CONEXÃO SERIAL ===
ser = serial.Serial(PORTA_SERIAL, BAUDRATE)
print(f"Conectado à {PORTA_SERIAL}...")

# === CSV ===
csv_file = open(ARQUIVO_CSV, mode='w', newline='', encoding='utf-8')
csv_writer = csv.writer(csv_file, delimiter=';')
csv_writer.writerow(['timestamp_ms', 'rpm_raw', 'rpm_filt'])
csv_file.flush()

# === GRÁFICO ===
plt.ion()
fig, ax = plt.subplots()

linha_raw, = ax.plot([], [], label='RPM bruto', color='tab:red')
linha_filt, = ax.plot([], [], label='RPM filtrado', color='tab:blue')

ax.set_xlabel("Amostras")
ax.set_ylabel("RPM")
ax.set_title("Leitura de Rotação (bruto x filtrado)")
ax.legend(loc="upper right")

# Buffers
rpm_raw_buf = deque(maxlen=TAMANHO_JANELA)
rpm_filt_buf = deque(maxlen=TAMANHO_JANELA)
amostra_idx = 0
t0 = time.time()

print("Lendo dados... (Ctrl+C para parar)")

try:
    while True:
        linha = ser.readline().decode('utf-8', errors='ignore').strip()

        # ignora logs do ESP-IDF
        if not linha or linha.startswith(("I (", "E (", "W (", "D (", "V (")):
            continue

        # espera formato: raw;filt
        partes = linha.split(';')
        if len(partes) < 2:
            print("Linha inesperada:", linha)
            continue

        try:
            rpm_raw = float(partes[0])
            rpm_filt = float(partes[1])
        except ValueError:
            print("Falha ao converter:", linha)
            continue

        # timestamp em ms
        timestamp_ms = int((time.time() - t0) * 1000)

        # escreve no CSV
        csv_writer.writerow([timestamp_ms, rpm_raw, rpm_filt])
        csv_file.flush()

        # atualiza buffers e gráfico
        amostra_idx += 1
        rpm_raw_buf.append(rpm_raw)
        rpm_filt_buf.append(rpm_filt)

        linha_raw.set_xdata(range(len(rpm_raw_buf)))
        linha_raw.set_ydata(rpm_raw_buf)

        linha_filt.set_xdata(range(len(rpm_filt_buf)))
        linha_filt.set_ydata(rpm_filt_buf)

        ax.relim()
        ax.autoscale_view()

        plt.draw()
        plt.pause(0.01)

except KeyboardInterrupt:
    print("\nEncerrando...")

finally:
    ser.close()
    csv_file.close()
    plt.ioff()
    plt.show()
