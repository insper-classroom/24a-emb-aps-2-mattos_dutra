#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define POT_PIN 27
#define START_STOP_PIN 7
#define LED_PIN 9
#define ENTER_PIN 26

SemaphoreHandle_t system_running_semaphore;

typedef struct {
    char button_states[9];
    uint8_t volume;
} ControlData;

const uint pins[] = {18, 19, 20, 21, 12, 10, 13, 11, ENTER_PIN};

QueueHandle_t xQueue;

void setup_gpio() {
    gpio_init(START_STOP_PIN);
    gpio_set_dir(START_STOP_PIN, GPIO_IN);
    gpio_pull_up(START_STOP_PIN);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(ENTER_PIN);
    gpio_set_dir(ENTER_PIN, GPIO_IN);
    gpio_pull_up(ENTER_PIN);
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

void control_task(void *params) {
    while (true) {
        if (!gpio_get(START_STOP_PIN)) {
            if (xSemaphoreTake(system_running_semaphore, (TickType_t)10) == pdTRUE) {
                if (uxSemaphoreGetCount(system_running_semaphore) == 0) {
                    xSemaphoreGive(system_running_semaphore);
                    gpio_put(LED_PIN, 1);
                } else {
                    xSemaphoreTake(system_running_semaphore, (TickType_t)10);
                    gpio_put(LED_PIN, 0);
                }
            }
            while (!gpio_get(START_STOP_PIN));
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void adc_task(void *params) {
    ControlData control_data = {0};
    while (true) {
        if (uxSemaphoreGetCount(system_running_semaphore) > 0) {
            // ADC reading and volume control logic here...
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void button_task(void *params) {
    ControlData control_data;
    memset(&control_data, 0, sizeof(control_data));
    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
    }
    while (true) {
        if (uxSemaphoreGetCount(system_running_semaphore) > 0) {
            for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
                // Button reading logic here...
            }
            xQueueSend(xQueue, &control_data, portMAX_DELAY);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void uart_task(void *params) {
    ControlData control_data;
    while (true) {
        if (xQueueReceive(xQueue, &control_data, portMAX_DELAY)) {
            // UART sending logic here...
        }
    }
}

void write_package(ControlData *data) {
    for (int i = 0; i < 9; i++) {
        uart_putc_raw(UART_ID, data->button_states[i]);
    }
    uart_putc_raw(UART_ID, data->volume);
}

int main() {
    stdio_init_all();
    setup_uart();
    setup_adc();
    setup_gpio();

    system_running_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(system_running_semaphore);  // Start as running

    xQueue = xQueueCreate(10, sizeof(ControlData));

    xTaskCreate(control_task, "Control Task", 256, NULL, 1, NULL);
    xTaskCreate(button_task, "Button Task", 256, NULL, 1, NULL);
    xTaskCreate(adc_task, "ADC Task", 256, NULL, 1, NULL);
    xTaskCreate(uart_task, "UART Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();
    while (true) {
        tight_loop_contents();
    }
}
