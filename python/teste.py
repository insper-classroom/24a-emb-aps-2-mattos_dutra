import serial
import uinput
from struct import unpack
import alsaaudio

ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)
device = uinput.Device([
    uinput.KEY_X, uinput.KEY_Z, uinput.KEY_C, uinput.KEY_V,
    uinput.KEY_LEFT, uinput.KEY_RIGHT, uinput.KEY_UP, uinput.KEY_DOWN
])

# Define a lista de teclas de acordo com a ordem dos botões em button_data
buttons = [
    uinput.KEY_X,    # Soco Forte
    uinput.KEY_Z,    # Soco Fraco
    uinput.KEY_C,    # Chute Forte
    uinput.KEY_V,    # Chute Fraco
    uinput.KEY_LEFT, # Mover para a esquerda
    uinput.KEY_RIGHT,# Mover para a direita
    uinput.KEY_UP,   # Mover para cima
    uinput.KEY_DOWN  # Mover para baixo
]

# Especifique o mixer e o dispositivo corretos
mixer_name = 'Master'  # Nome do mixer para volume geral
card_index = 1  # Índice do dispositivo 'sofhdadsp' que tem o mixer 'Master'

try:
    mixer = alsaaudio.Mixer(mixer_name, cardindex=card_index)
    print("Iniciando a leitura dos estados dos botões e ajuste de volume...")

    while True:
        data = ser.read(10)  # Lê 8 estados dos botões + 2 bytes do volume
        if len(data) == 10:
            button_data = data[:8]
            volume_data = data[8:10]
            volume_level = unpack('h', volume_data)[0]  # Converte bytes em short int
            volume_percent = (volume_level / 1023.0) * 100  # Converte valor ADC em percentual
            mixer.setvolume(int(volume_percent))  # Ajusta o volume
            print("Volume ajustado para:", int(volume_percent), "%")

            for i, state in enumerate(button_data):
                device.emit(buttons[i], 1 if state == 1 else 0)  # Emite eventos de teclado

except KeyboardInterrupt:
    print("Program terminated by user")
except alsaaudio.ALSAAudioError as e:
    print(f"ALSAAudioError: {e}")
except Exception as e:
    print(f"An error occurred: {e}")
finally:
    ser.close()
