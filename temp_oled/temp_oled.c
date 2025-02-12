#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "ssd1306.h" // Biblioteca SSD1306 para Pico SDK

// Definições para o display SSD1306
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display
#define I2C_SDA 14          // Pino SDA
#define I2C_SCL 15          // Pino SCL

// Instância do display SSD1306
ssd1306_t display;

// Função para calcular a temperatura interna com base nos valores do sensor
float read_internal_temperature() {
    // Lê o valor bruto do ADC
    uint16_t adc_value = adc_read();

    // Converte o valor do ADC em tensão
    float voltage = adc_value * (3.3f / (1 << 12)); // 12-bit ADC resolution

    // Calcula a temperatura em graus Celsius
    float temperature_celsius = 27.0f - (voltage - 0.706f) / 0.001721f;

    return temperature_celsius;
}

// Função para exibir texto no display OLED
void display_temperature(float temperature) {
    char buffer[32]; // Buffer para armazenar o texto

    // Limpa o display
    ssd1306_clear(&display);

    // Formata a temperatura em uma string e exibe no display
    snprintf(buffer, sizeof(buffer), "Temp: %.2f C", temperature);
    ssd1306_draw_string(&display, 0, 0, 1, buffer);

    // Atualiza o display
    ssd1306_show(&display);
}

int main() {
    // Inicializa a UART para saída no terminal
    stdio_init_all();

    // Inicializa o ADC e seleciona o canal 4 (sensor de temperatura interno)
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    // Inicializa I2C no canal 1
    i2c_init(i2c1, 400 * 1000); // 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display OLED
    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, i2c1)) {
        printf("Falha ao inicializar o display SSD1306\n");
        return 1; // Sai do programa
    }

    printf("Display SSD1306 inicializado com sucesso!\n");

    while (true) {
        // Lê e calcula a temperatura
        float temperatureC = read_internal_temperature();

        // Exibe a temperatura no console (opcional)
        printf("Internal Temperature: %.2f °C\n", temperatureC);

        // Exibe a temperatura no display OLED
        display_temperature(temperatureC);

        // Espera 1 segundo antes de medir novamente
        sleep_ms(1000);
    }

    return 0;
}