#include "LED.h"


void setup_LED() {
    gpio_init(LED_R_PIN);
    gpio_init(LED_B_PIN);
    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
}