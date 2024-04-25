import serial
import uinput
import alsaaudio

# Inicialização e configuração
ser = serial.Serial('/dev/rfcomm0', 9600, timeout=0.1)
mixer = alsaaudio.Mixer('Speaker', cardindex=1)
device = uinput.Device([
    uinput.KEY_X, uinput.KEY_Z, uinput.KEY_C, uinput.KEY_V,
    uinput.KEY_LEFT, uinput.KEY_RIGHT, uinput.KEY_UP, uinput.KEY_DOWN
])

# Mapeamento dos botões
buttons = [
    uinput.KEY_X, uinput.KEY_Z, uinput.KEY_C, uinput.KEY_V,
    uinput.KEY_LEFT, uinput.KEY_RIGHT, uinput.KEY_UP, uinput.KEY_DOWN
]

# Cabeçalho esperado para os dados do volume
HEADER_VOLUME = 0xAA
last_volume = -1

print("Iniciando a leitura dos estados dos botões e volume...")

try:
    while True:
        header = ser.read(1)  # Lê o cabeçalho
        if header and header[0] == 0xFF:  # Confirma que é o cabeçalho esperado
            button_states = ser.read(8)  # Lê 8 bytes dos estados dos botões
            separator = ser.read(1)  # Lê o separador
            if separator and separator[0] == 0xFE:
                volume_byte = ser.read(1)  # Lê o byte do volume
                if volume_byte:
                    new_volume = volume_byte[0]  # Converte byte para inteiro diretamente

                    if abs(new_volume - last_volume) > 1:
                        mixer.setvolume(new_volume)
                        print(f"Volume: {new_volume}%")
                        last_volume = new_volume

            # Interpreta os bytes dos estados dos botões
            for i, state in enumerate(button_states):
                if state == 1:
                    device.emit(buttons[i], 1)  # Pressiona o botão
                else:
                    device.emit(buttons[i], 0)  # Solta o botão

except KeyboardInterrupt:
    print("Program terminated by user")
except Exception as e:
    print(f"An error occurred: {e}")
finally:
    ser.close()
