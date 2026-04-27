#include "task_soil_moisture.h"
#include "task_oled.h" // Required to call sendToOLED()

#ifndef P0_ADC
#define P0_ADC 1 // Replace '1' with the actual ADC pin
#endif

const int soilSensorPin = P0_ADC;

void Task_SoilMoisture(void *pvParameters) {
    // Variable to hold the formatted string for the OLED
    char oledBuffer[32];
    int sensorValue = 0;
    while(1) {
        // Read the analog value (0-4095 on ESP32 by default)
        sensorValue = analogRead(soilSensorPin);
        glob_soil_moisture = sensorValue; // Update global variable for soil moisture

        // Print to Serial for debugging
        // Serial.print("[SOIL TASK] ADC Value: ");
        // Serial.println(sensorValue);

        // Format the integer into a string.
        // We add padding spaces at the end ("   ") to ensure that if the value 
        // drops from 1000 to 99, the trailing zeros are visually overwritten.
        snprintf(oledBuffer, sizeof(oledBuffer), "Soil Moisture: %d    ", sensorValue);

        // Send to OLED at the requested coordinates: x = 0, y = 40
        sendToOLED(0, 40, oledBuffer);

        vTaskDelay(1000 / portTICK_PERIOD_MS); 
    }
}