import serial
import uinput
import time

# Configurações do dispositivo serial
ser = serial.Serial('/dev/ttyACM0', 115200)  # Ajuste o dispositivo conforme necessário

# Criar dispositivo de entrada com eventos de tecla
device = uinput.Device([
    uinput.KEY_X,  # Soco Forte
    uinput.KEY_Z,  # Soco Fraco
    uinput.KEY_C,  # Chute Forte
    uinput.KEY_V   # Chute Fraco
])

# Mapeamento dos botões para teclas
button_to_key = {
    b'X': uinput.KEY_X,
    b'Y': uinput.KEY_Z,
    b'A': uinput.KEY_C,
    b'B': uinput.KEY_V
}

try:
    while True:
        data = ser.read()  # Lê um byte
        if data in button_to_key:
            key = button_to_key[data]
            device.emit_click(key)  # Simula o pressionamento e liberação da tecla
            print(f'Pressed: {data.decode()} mapped to {key}')
        time.sleep(0.1)  # Um pequeno atraso para garantir a estabilidade da entrada

except KeyboardInterrupt:
    print("Program terminated by user")
except Exception as e:
    print(f"An error occurred: {e}")
finally:
    ser.close()
