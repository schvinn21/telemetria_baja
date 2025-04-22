import serial
import matplotlib.pyplot as plt
from collections import deque
import re

# CONFIGURAÇÕES
PORTA_SERIAL = 'COM9'     # ← troque conforme necessário
BAUDRATE = 115200
TAMANHO_JANELA = 100       # número de pontos exibidos

# Conecta à porta serial
ser = serial.Serial(PORTA_SERIAL, BAUDRATE)
print(f"Conectado à {PORTA_SERIAL}...")

# Buffers dos eixos
x_data = deque(maxlen=TAMANHO_JANELA)
y_data = deque(maxlen=TAMANHO_JANELA)
z_data = deque(maxlen=TAMANHO_JANELA)

# Configura o gráfico
plt.ion()
fig, ax = plt.subplots()
linha_x, = ax.plot([], [], label='X', color='red')
linha_y, = ax.plot([], [], label='Y', color='green')
linha_z, = ax.plot([], [], label='Z', color='blue')

ax.set_ylim(-20000, 20000)  # ajuste conforme seu sensor
ax.set_title("Aceleração MPU6050 (X, Y, Z)")
ax.set_xlabel("Amostras")
ax.set_ylabel("Valor")
ax.legend()

padrao = re.compile(r"Accel X:\s*(-?\d+),\s*Y:\s*(-?\d+),\s*Z:\s*(-?\d+)")

while True:
    try:
        linha = ser.readline().decode('utf-8').strip()
        match = padrao.search(linha)

        if match:
            ax_val = int(match.group(1))
            ay_val = int(match.group(2))
            az_val = int(match.group(3))

            x_data.append(ax_val)
            y_data.append(ay_val)
            z_data.append(az_val)

            linha_x.set_ydata(x_data)
            linha_x.set_xdata(range(len(x_data)))
            linha_y.set_ydata(y_data)
            linha_y.set_xdata(range(len(y_data)))
            linha_z.set_ydata(z_data)
            linha_z.set_xdata(range(len(z_data)))

            ax.relim()
            ax.autoscale_view()
            plt.draw()
            plt.pause(0.01)

    except Exception as e:
        print("Erro:", e)
