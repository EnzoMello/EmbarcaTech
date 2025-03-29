#ifndef Buzzer_H
#define Buzzer_H

#include "pico/stdlib.h"
#define BUZZER_PIN 10
#define TEMPERATURE_ALERT_LIMIT 5.0f


void setup_buzzer();

void buzzer_alert(float discrepancy);

#endif

