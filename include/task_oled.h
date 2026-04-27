#ifndef TASK_OLED_H
#define TASK_OLED_H

#include <Arduino.h>
#include "global.h"

// Structure of the message passed through the FreeRTOS Queue
typedef struct {
    int16_t x;
    int16_t y;
    char content[40]; // Maximum characters per update line
} OledMessage_t;

// Main task function
void Task_OLED(void *pvParameters);

/**
 * @brief Public interface to update a specific area on the OLED
 * @param x Horizontal position (0-127)
 * @param y Vertical position (0-63)
 * @param msg Content string to display
 */
void sendToOLED(int16_t x, int16_t y, const char* msg);

#endif