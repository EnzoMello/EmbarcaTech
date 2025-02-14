#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "hardware/uart.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"

// Definições do display SSD1306
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
#define I2C_SDA 14
#define I2C_SCL 15

// LED RGB
#define LED_R_PIN 13
#define LED_B_PIN 12
#define LED_G_PIN 11

// Definições de Rede
#define WIFI_SSID "moto g(9) power_6168"
#define WIFI_PASSWORD "ENZOMELO10"
#define SERVER_PORT 8050

// Limite de discrepância para alerta
#define TEMPERATURE_ALERT_LIMIT 5.0f  // Limite de 5°C para alerta

extern ssd1306_t display;
extern float server_temperature;

// Funções declaradas
float read_internal_temperature();
void display_temperatures(float sensor_temp, float server_temp, const char* alert_msg);
void control_led_alert(float sensor_temp, float server_temp, int *led_state, int *last_change_time);
err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
void tcp_server(void);

#endif // FUNCTIONS_H
