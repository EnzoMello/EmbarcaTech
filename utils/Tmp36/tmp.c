#include "tmp.h"

void tmp_init(){
    adc_init(); // Inicializa o ADC
    adc_gpio_init(28); // Habilita o GPIO 28 como entrada analógica
    adc_select_input(2); // O GPIO 28 corresponde ao canal ADC2
};

float get_temp() {
    int num_readings = 10;  // Número de leituras para a média
    float sum = 0.0f;

    // Ler múltiplos valores e somá-los
    for (int i = 0; i < num_readings; i++) {
        uint16_t raw_value = adc_read();
        float voltage = (raw_value * 3.3f) / (1 << 12);
        sum += voltage;
        sleep_ms(10);  // Espera um pouco entre as leituras para evitar leituras muito rápidas
    }

    // Calcular a média das leituras
    float avg_voltage = sum / num_readings;

    // Aplicar um fator de escala para reduzir a sensibilidade
    float temperature = (avg_voltage - 0.5f) / (0.02f * 2); // Diminuir a sensibilidade multiplicando por 2

    return temperature;
}