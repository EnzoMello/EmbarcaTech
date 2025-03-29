#include "Buzzer.h"

void setup_buzzer() {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);  // Começa desligado
}

void buzzer_alert(float discrepancy) {
    if (discrepancy > TEMPERATURE_ALERT_LIMIT) {
        gpio_put(BUZZER_PIN, 1);
        sleep_ms(50);
        gpio_put(BUZZER_PIN, 0);
        sleep_ms(50);
    } else if (discrepancy < -TEMPERATURE_ALERT_LIMIT) {
        gpio_put(BUZZER_PIN, 1);
        sleep_ms(100);
        gpio_put(BUZZER_PIN, 0);
        sleep_ms(100);
    
    } else {
    // Temperatura dentro do limite
    gpio_put(BUZZER_PIN, 0);
    }
}