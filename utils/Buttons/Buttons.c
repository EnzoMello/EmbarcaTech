#include "Buttons.h"

void setup_button() {
    gpio_init(BUTTON_HOT_PIN);
    gpio_set_dir(BUTTON_HOT_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_HOT_PIN);


    gpio_init(BUTTON_COLD_PIN);
    gpio_set_dir(BUTTON_COLD_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_COLD_PIN);
}