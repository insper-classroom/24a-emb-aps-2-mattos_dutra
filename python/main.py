import serial
import uinput
import alsaaudio

# Inicialização e configuração
ser = serial.Serial('/dev/rfcomm0', 9600, timeout=0.1)
mixer = alsaaudio.Mixer('Speaker', cardindex=1)
device = uinput.Device([
    uinput.KEY_X, uinput.KEY_Z, uinput.KEY_C, uinput.KEY_V,
    uinput.KEY_LEFT, uinput.KEY_RIGHT, uinput.KEY_UP, uinput.KEY_DOWN, uinput.KEY_ENTER
])

# Mapeamento dos botões
buttons = [
    uinput.KEY_X, uinput.KEY_Z, uinput.KEY_C, uinput.KEY_V,
    uinput.KEY_LEFT, uinput.KEY_RIGHT, uinput.KEY_UP, uinput.KEY_DOWN, uinput.KEY_ENTER
]

# Cabeçalho esperado para os dados do volume
HEADER_VOLUME = 0xAA
last_volume = -1

print("Iniciando a leitura dos estados dos botões e volume...")

try:
    while True:
        header = ser.read(1)
        if header and header[0] == 0xFF:
            button_states = ser.read(9)
            separator = ser.read(1)
            if separator and separator[0] == 0xFE:
                volume_byte = ser.read(1)
                if volume_byte:
                    new_volume = volume_byte[0]
                    if new_volume != last_volume and new_volume != 0:
                        mixer.setvolume(new_volume-5)
                        print(f"Volume ajustado para: {new_volume-5}%")
                        last_volume = new_volume-5

            for i, state in enumerate(button_states):
                if state == 1:
                    device.emit(buttons[i], 1)
                else:
                    device.emit(buttons[i], 0)

except KeyboardInterrupt:
    print("Program terminated by user")
except Exception as e:
    print(f"An error occurred: {e}")
finally:
    ser.close()
