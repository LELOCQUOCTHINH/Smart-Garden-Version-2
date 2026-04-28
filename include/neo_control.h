#ifndef __NEO_BLINKY__
#define __NEO_BLINKY__
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>


#define NEO_PIN 45
#define LED_COUNT 1 

#define MAX_NEO_STATES 10
// Cấu trúc trạng thái NeoPixel
struct NeoState {
    float humiThreshold;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

#include "global.h"
void neo_control_RPC(void *pvParameters);


#endif