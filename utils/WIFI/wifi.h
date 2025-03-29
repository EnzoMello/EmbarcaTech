#ifndef WIFI_H
#define WIFI_H

#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/cyw43_arch.h" // Controla o módulo Wi-Fi CYW43439 da Raspberry Pi Pico W
#define WIFI_SSID "moto g(9) power_6168"
#define WIFI_PASSWORD "ENZOMELO1000"


void init_wifi();

#endif