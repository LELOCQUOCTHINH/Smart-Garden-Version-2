#include "neo_control.h"


void neo_control_RPC(void *pvParameters){

    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    // Set all pixels to off to start
    strip.clear();
    strip.show();

    float local_humidity = 60.0; // Default safe value

    while(1) {        
        // ------------------------------------------------
        // 1. SEMAPHORE SYNCHRONIZATION
        // ------------------------------------------------
        // Safely read the global humidity updated by the DHT task
        if (xSemaphoreTake(xMutexTempHumi, (TickType_t)10) == pdTRUE) 
        {
            local_humidity = glob_humidity;
            xSemaphoreGive(xMutexTempHumi); 
        } 
        else 
        {
            Serial.println("⚠️ [NEO TASK] Cannot get Mutex, using previous humidity!");
        }

        // ------------------------------------------------
        // 2. HUMIDITY TO COLOR MAPPING LOGIC (3 Levels)
        // ------------------------------------------------
        uint32_t color = strip.Color(0, 0, 0); // Default Off

        if (local_humidity < 40.0) {
            // Level 1: Dry Environment (< 40%) -> Red Color
            color = strip.Color(255, 0, 0);
        } 
        else if (local_humidity >= 40.0 && local_humidity <= 70.0) {
            // Level 2: Optimal/Comfortable (40% - 70%) -> Green Color
            color = strip.Color(0, 255, 0);
        } 
        else {
            // Level 3: Highly Humid (> 70%) -> Blue Color
            color = strip.Color(0, 0, 255);
        }

        // ------------------------------------------------
        // 3. DISPLAY COLOR
        // ------------------------------------------------
        // Set pixel 0 to the calculated color. 
        // Note: For Yolo UNO, NeoPixel is usually bright, so you might want 
        // to dim it by using lower values like (50, 0, 0) instead of 255.
        strip.setPixelColor(0, color); 
        strip.show(); // Push data to the LED

        // Task runs every 500 milliseconds to check for updates
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}