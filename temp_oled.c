#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "hardware/uart.h"

// Definições do display SSD1306
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
#define I2C_SDA 14
#define I2C_SCL 15

ssd1306_t display;

// Definições para comunicação Serial (UART)
#define UART_ID uart0
#define BAUD_RATE 115200
#define RX_PIN 8
#define TX_PIN 9

// Função para ler a temperatura interna
float read_internal_temperature() {
    uint16_t adc_value = adc_read();
    float voltage = adc_value * (3.3f / (1 << 12)); // 12-bit ADC resolution
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

// Exibir temperaturas no OLED
void display_temperatures(float sensor_temp, float server_temp) {
    char buffer[32];

    ssd1306_clear(&display);

    snprintf(buffer, sizeof(buffer), "Sensor: %.2f C", sensor_temp);
    ssd1306_draw_string(&display, 0, 0, 1, buffer);

    snprintf(buffer, sizeof(buffer), "Servidor: %.2f C", server_temp);
    ssd1306_draw_string(&display, 0, 16, 1, buffer);

    ssd1306_show(&display);
}

int main() {
    stdio_init_all();
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);

    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, i2c1)) {
        printf("Falha ao inicializar display SSD1306\n");
        return 1;
    }

    printf("Display SSD1306 iniciado com sucesso!\n");

    float server_temperature = 0.0f;
    char temp_buffer[32];
    int index = 0;

    while (true) {
        // Enviar temperatura da placa ao servidor
        float sensor_temperature = read_internal_temperature();
        printf("%.2f\n", sensor_temperature);  // ENVIA pela Serial

        // Ler dados do servidor pela Serial
        if (uart_is_readable(UART_ID)) {
            char c = uart_getc(UART_ID);

            if (c != '\n') {
                temp_buffer[index++] = c;
                temp_buffer[index] = '\0';
            } else {
                printf("Recebido do servidor: %s\n", temp_buffer);
                server_temperature = atof(temp_buffer);
                index = 0;
            }
        }

        // Exibir no OLED
        display_temperatures(sensor_temperature, server_temperature);

        sleep_ms(2000);
    }

    return 0;
}
