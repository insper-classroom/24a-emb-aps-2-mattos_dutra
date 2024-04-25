import serial
import uinput

ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)

device = uinput.Device([
    uinput.KEY_X,  # Soco Forte
    uinput.KEY_Z,  # Soco Fraco
    uinput.KEY_C,  # Chute Forte
    uinput.KEY_V,  # Chute Fraco
    uinput.KEY_LEFT,   # Mover para a esquerda
    uinput.KEY_RIGHT,  # Mover para a direita
    uinput.KEY_UP,     # Mover para cima
    uinput.KEY_DOWN    # Mover para baixo
])

buttons = [
    uinput.KEY_X,
    uinput.KEY_Z,
    uinput.KEY_C,
    uinput.KEY_V,
    uinput.KEY_LEFT,
    uinput.KEY_RIGHT,
    uinput.KEY_UP,
    uinput.KEY_DOWN
]

print("Iniciando a leitura dos estados dos botões...")

try:
    while True:
        data = ser.read(8)
        if len(data) == 8:
            print("Dados recebidos:", list(data))  # Mostra os dados brutos recebidos
            for i, state in enumerate(data):
                if state == 1:
                    device.emit(buttons[i], 1)  # Press the key
                else:
                    device.emit(buttons[i], 0)  # Release the key
                print(f"Botão {i} ({buttons[i]}): {'Pressionado' if state == 1 else 'Solto'}")
                
except KeyboardInterrupt:
    print("Program terminated by user")
except Exception as e:
    print(f"An error occurred: {e}")
finally:
    ser.close()