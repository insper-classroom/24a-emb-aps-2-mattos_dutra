#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include <stdio.h>
#include <stdbool.h>


// Definir o número da UART e os pinos
#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define POT_PIN 27

typedef struct {
    char button_states[8];
    uint8_t volume;
} ControlData;

const uint pins[] = {18, 19, 20, 21, 12, 10, 13, 11};
QueueHandle_t xQueue;

void write_package(ControlData *data) {
    for (int i = 0; i < 8; i++) {
        uart_putc_raw(UART_ID, data->button_states[i]);
    }
    uart_putc_raw(UART_ID, data->volume);
}

void setup_uart() {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}

void setup_adc() {
    adc_init();
    adc_gpio_init(POT_PIN);
    adc_select_input(1);
}


volatile bool volume_ready = false;

void adc_task(void *params) {
    ControlData control_data;
    while (true) {
        uint16_t raw = adc_read();
        control_data.volume = raw * 100 / 4095;
        if (!volume_ready) {
            xQueueSend(xQueue, &control_data, portMAX_DELAY);
            volume_ready = true;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void button_task(void *params) {
    ControlData control_data;
    uint8_t last_state[8] = {0};
    memset(&control_data, 0, sizeof(control_data));

    for (int i = 0; i < 8; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
    }

    while (true) {
        for (int i = 0; i < 8; i++) {
            uint8_t current_state = gpio_get(pins[i]) ? 0 : 1;
            if (current_state != last_state[i]) {
                vTaskDelay(pdMS_TO_TICKS(20)); // Debounce delay
                current_state = gpio_get(pins[i]) ? 0 : 1; // Read again after delay
            }
            control_data.button_states[i] = current_state;
            last_state[i] = current_state;
        }
        xQueueSend(xQueue, &control_data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void uart_task(void *params) {
    ControlData control_data;
    while (true) {
        if (xQueueReceive(xQueue, &control_data, portMAX_DELAY)) {
            uart_putc_raw(UART_ID, 0xFF);  // Cabeçalho para indicar início de novo pacote
            for (int i = 0; i < 8; i++) {
                uart_putc_raw(UART_ID, control_data.button_states[i]);
            }
            uart_putc_raw(UART_ID, 0xFE);  // Separador
            uart_putc_raw(UART_ID, control_data.volume);
        }
    }
}

int main() {
    stdio_init_all();
    setup_uart();
    setup_adc();

    xQueue = xQueueCreate(10, sizeof(ControlData));

    xTaskCreate(button_task, "Button Task", 256, NULL, 1, NULL);
    xTaskCreate(adc_task, "ADC Task", 256, NULL, 1, NULL);
    xTaskCreate(uart_task, "UART Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();
    while (true) {
        tight_loop_contents();
    }
}