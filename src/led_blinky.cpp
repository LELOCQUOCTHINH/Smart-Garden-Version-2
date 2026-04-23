#include "led_blinky.h"

void led_blinky(void *pvParameters){
  pinMode(LED_GPIO, OUTPUT);
  
  int current_blinking_interval = 1000U;
  while(1) {
    if(xSemaphoreTake(xMutexBlinkingInterval, (TickType_t)10) == pdTRUE) 
    {
        // ----- (CRITICAL SECTION) -----
        current_blinking_interval = blinkingInterval;
        // return the mutex after updating the state
        xSemaphoreGive(xMutexBlinkingInterval); 
        // ------------------------------------------------
    } 
    else 
    {
        Serial.println("⚠️ ERROR: cannot get Mutex, skip reading blinking interval!");
    }

    digitalWrite(LED_GPIO, HIGH);  // turn the LED ON
    vTaskDelay(current_blinking_interval);
    digitalWrite(LED_GPIO, LOW);  // turn the LED OFF
    vTaskDelay(current_blinking_interval);
  }
}