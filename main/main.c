#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

// Definição dos pinos dos botões
const uint SocoForte = 18;
const uint SocoFraco = 19;
const uint ChuteForte = 20;
const uint ChuteFraco = 21;

// Configuração inicial da UART
void setup_uart() {
    uart_init(uart0, 115200);
    gpio_set_function(0, GPIO_FUNC_UART);  // TX
    gpio_set_function(1, GPIO_FUNC_UART);  // RX
}

// Task para verificar os botões e enviar teclas
void button_task(void *params) {
    gpio_init(SocoForte);
    gpio_set_dir(SocoForte, GPIO_IN);
    gpio_pull_up(SocoForte);

    gpio_init(SocoFraco);
    gpio_set_dir(SocoFraco, GPIO_IN);
    gpio_pull_up(SocoFraco);

    gpio_init(ChuteForte);
    gpio_set_dir(ChuteForte, GPIO_IN);
    gpio_pull_up(ChuteForte);

    gpio_init(ChuteFraco);
    gpio_set_dir(ChuteFraco, GPIO_IN);
    gpio_pull_up(ChuteFraco);

    while (true) {
        if (gpio_get(SocoForte) == 0) {
            uart_putc(uart0, 'X');  // Soco Forte
        }
        if (gpio_get(SocoFraco) == 0) {
            uart_putc(uart0, 'Y');  // Soco Fraco
        }
        if (gpio_get(ChuteForte) == 0) {
            uart_putc(uart0, 'A');  // Chute Forte
        }
        if (gpio_get(ChuteFraco) == 0) {
            uart_putc(uart0, 'B');  // Chute Fraco
        }
        vTaskDelay(pdMS_TO_TICKS(100));  // Debouncing
    }
}

int main() {
    stdio_init_all();
    setup_uart();
    
    xTaskCreate(button_task, "Button Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true);  // Loop infinito
}
