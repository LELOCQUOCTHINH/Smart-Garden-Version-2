#include "task_oled.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
static QueueHandle_t xQueueOled = NULL;

void sendToOLED(int16_t x, int16_t y, const char* msg) {
    if (xQueueOled == NULL) return;
    
    OledMessage_t newMsg;
    newMsg.x = x;
    newMsg.y = y;
    strncpy(newMsg.content, msg, sizeof(newMsg.content) - 1);
    newMsg.content[sizeof(newMsg.content) - 1] = '\0';
    
    // Non-blocking send
    xQueueSend(xQueueOled, &newMsg, 0); 
}

void Task_OLED(void *pvParameters) {
    xQueueOled = xQueueCreate(15, sizeof(OledMessage_t));
    
    bool oledAvailable = true;
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println("[OLED] Hardware not found. Entering dummy mode.");
        oledAvailable = false;
    } else {
        display.clearDisplay();
        display.display(); // Start with a clean screen
    }

    OledMessage_t receivedMsg;

    while(1) {
        if (xQueueReceive(xQueueOled, &receivedMsg, portMAX_DELAY) == pdPASS) {
            if (!oledAvailable) continue;

            // Set cursor to requested position
            display.setCursor(receivedMsg.x, receivedMsg.y);
            
            // CRITICAL: setTextColor(foreground, background) 
            // This clears the area behind the text automatically as it draws.
            display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
            display.setTextSize(1);
            
            // Print the content at the specific location
            display.print(receivedMsg.content);
            
            // Push changes to hardware without clearing the entire buffer
            display.display();
        }
    }
}