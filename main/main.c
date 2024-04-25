#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hc06.h"
#include <stdio.h>

typedef struct {
    char states[8];
} ButtonStates;

const uint pins[] = {18, 19, 20, 21, 12, 10, 13, 11};

void setup_uart() {
    uart_init(uart1, 9600);
    gpio_set_function(HC06_TX_PIN, GPIO_FUNC_UART);  // TX
    gpio_set_function(HC06_RX_PIN, GPIO_FUNC_UART);  // RX
}

void button_task(void *params) {
    for (int i = 0; i < 8; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
    }

    ButtonStates states;
    while (true) {
        for (int i = 0; i < 8; i++) {
            states.states[i] = gpio_get(pins[i]) ? 0 : 1;  // 1 if pressed, 0 if released
        }

        uart_write_blocking(uart1, (const uint8_t*)&states, sizeof(states));
        vTaskDelay(pdMS_TO_TICKS(50)); // Short delay to manage sending rate
    }
}


int main() {
    stdio_init_all();
    setup_uart();
    xTaskCreate(button_task, "Button Task", 256, NULL, 1, NULL);
    vTaskStartScheduler();
    while (true);
}