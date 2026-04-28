#ifndef __LED_BLINKY__
#define __LED_BLINKY__
#include <Arduino.h>

#define LED_GPIO 48

#define MAX_LED_STATES 10
// Cấu trúc định nghĩa 1 trạng thái
struct LedState {
    float tempThreshold;
    int interval;
};

#include "global.h"

void led_blinky(void *pvParameters);


#endif