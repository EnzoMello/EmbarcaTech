#ifndef Display_H
#define Display_H

#include "pico/stdlib.h"
#include "ssd1306.h" // Controla o display OLED SSD1306 para exibir informações na tela

// Definições do display SSD1306
static ssd1306_t display;
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
#define I2C_SDA 14
#define I2C_SCL 15

void setup_display();

void display_message(const char *message);
void display_temperatures(float sensor_temp, float server_temp, const char* alert_msg);


#endif