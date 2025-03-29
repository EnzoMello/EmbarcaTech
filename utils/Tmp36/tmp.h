#ifndef tmp_H
#define tmp_H

#include "pico/stdlib.h"
#include "hardware/adc.h" // Leituras dos pinos ADC da placa e seus valores

void tmp_init();

float get_temp();

#endif