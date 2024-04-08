#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

const uint ADC_PIN_X = 26; 
const uint ADC_PIN_Y = 27;

QueueHandle_t xQueueAdc;

typedef struct {
    int axis;
    int val;
} adc_reading_t;

void x_task(void *params) {
    adc_reading_t x_reading = {.axis = 0};
    
    while (1) {
        adc_select_input(0);  
        if ((adc_read() - 2047) / 8 > -30 && (adc_read() - 2047) / 8 < 30) {
            x_reading.val = 0;
        } else {
            x_reading.val = (adc_read() - 2047) / 8; 
        }
        xQueueSend(xQueueAdc, &x_reading, portMAX_DELAY); 
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}

void y_task(void *params) {
    adc_reading_t y_reading = {.axis = 1};
    
    while (1) {
        adc_select_input(1); 
        if ((adc_read() - 2047) / 8 > -30 && (adc_read() - 2047) / 8 < 30) {
            y_reading.val = 0;
        } else {
            y_reading.val = (adc_read() - 2047) / 8; 
        }
        xQueueSend(xQueueAdc, &y_reading, portMAX_DELAY); 
        vTaskDelay(pdMS_TO_TICKS(100));  
    }
}

void uart_task(void *params) {
    adc_reading_t reading;
    
    while (1) {
        if (xQueueReceive(xQueueAdc, &reading, portMAX_DELAY)) {
            write_package(reading);
        }
    }
}

void write_package(adc_reading_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF ;

    uart_putc_raw(uart0, data.axis);
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb);
    uart_putc_raw(uart0, -1);
}
int main() {
    stdio_init_all(); 
    adc_init();        

    adc_gpio_init(ADC_PIN_X);
    adc_gpio_init(ADC_PIN_Y);

    xQueueAdc = xQueueCreate(10, sizeof(adc_reading_t));  

    xTaskCreate(x_task, "x_task", 256, NULL, 1, NULL);
    xTaskCreate(y_task, "y_task", 256, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart_task", 256, NULL, 1, NULL);

    vTaskStartScheduler(); 

    while (true);  
    return 0;
}