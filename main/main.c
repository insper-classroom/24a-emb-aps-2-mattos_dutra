#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

// Definir o número da UART e os pinos
#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define POT_PIN 27
#define START_STOP_PIN 7
#define LED_PIN 9
#define ENTER_PIN 26

#define VOLUME_CHANGE_THRESHOLD 1  // Threshold para alteração de volume considerada significativa
#define VOLUME_RESET_THRESHOLD 10  // Número de leituras consecutivas próximas a zero para resetar o volume

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
    static bool ledState = false;  // Estado inicial do LED

    while (true) {
        if (!gpio_get(START_STOP_PIN)) {  // Verifica se o botão foi pressionado (ativo baixo)
            // Espera até que o botão seja liberado (debounce)
            while (!gpio_get(START_STOP_PIN));
            vTaskDelay(pdMS_TO_TICKS(200));    // Pequeno delay após debounce

            // Alterna o estado do LED
            ledState = !ledState;
            gpio_put(LED_PIN, ledState);

            if (ledState) {
                // Se o LED está aceso, significa que o sistema deve estar rodando, então libere o semáforo
                xSemaphoreGive(system_running_semaphore);
            } else {
                // Se o LED está apagado, o sistema deve parar, então pegue o semáforo para si
                xSemaphoreTake(system_running_semaphore, portMAX_DELAY);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));  // Verifica o botão a cada 100ms
    }
}




void adc_task(void *params) {
    static uint8_t last_sent_volume = 255;
    static int zero_readings_count = 0;
    ControlData control_data = {0};

    while (true) {
        if (xSemaphoreTake(system_running_semaphore, 0) == pdTRUE) {
            uint16_t raw = adc_read();
            uint8_t new_volume = raw * 100 / 4095;

            if (abs(new_volume - last_sent_volume) >= VOLUME_CHANGE_THRESHOLD) {
                control_data.volume = new_volume;
                last_sent_volume = new_volume;
                zero_readings_count = 0;
                xQueueSend(xQueue, &control_data, portMAX_DELAY);
            } else if (new_volume == 0) {
                zero_readings_count++;
                if (zero_readings_count >= VOLUME_RESET_THRESHOLD && last_sent_volume != 0) {
                    control_data.volume = new_volume;
                    last_sent_volume = new_volume;
                    zero_readings_count = 0;
                    xQueueSend(xQueue, &control_data, portMAX_DELAY);
                }
            } else {
                zero_readings_count = 0;
            }
            xSemaphoreGive(system_running_semaphore);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void button_task(void *params) {
    ControlData control_data;
    memset(&control_data, 0, sizeof(control_data));

    for (int i = 0; i < 9; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
    }

    uint8_t last_state[9] = {0};

    while (true) {
        if (xSemaphoreTake(system_running_semaphore, 0) == pdTRUE) {
            for (int i = 0; i < 9; i++) {
                uint8_t current_state = gpio_get(pins[i]) ? 0 : 1;
                if (current_state != last_state[i]) {
                    vTaskDelay(pdMS_TO_TICKS(20)); // Debounce delay
                    current_state = gpio_get(pins[i]) ? 0 : 1; // Read again after delay
                    control_data.button_states[i] = current_state;
                    last_state[i] = current_state;
                }
            }
            xQueueSend(xQueue, &control_data, portMAX_DELAY);
            xSemaphoreGive(system_running_semaphore);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void uart_task(void *params) {
    ControlData control_data;
    while (true) {
        if (xQueueReceive(xQueue, &control_data, portMAX_DELAY)) {
            uart_putc_raw(UART_ID, 0xFF);  // Start package header
            for (int i = 0; i < 9; i++) {
                uart_putc_raw(UART_ID, control_data.button_states[i]);
            }
            uart_putc_raw(UART_ID, 0xFE);  // Separator
            uart_putc_raw(UART_ID, control_data.volume);
        }
    }
}

void write_package(ControlData *data) {
    uart_putc_raw(UART_ID, 0xFF);  // Start package header
    for (int i = 0; i < 9; i++) {
        uart_putc_raw(UART_ID, data->button_states[i]);
    }
    uart_putc_raw(UART_ID, 0xFE);  // Separator
    uart_putc_raw(UART_ID, data->volume);
}

int main() {
    stdio_init_all();
    setup_uart();
    setup_adc();
    setup_gpio();

    system_running_semaphore = xSemaphoreCreateBinary();

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
