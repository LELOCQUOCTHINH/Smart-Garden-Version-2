#include "task_temp_humi_monitor.h"
DHT20 dht20;
// LiquidCrystal_I2C lcd(33,16,2);


void task_temp_humi_monitor(void *pvParameters){

    dht20.begin();

    while (1){
        /* code */
        
        dht20.read();
        float temperature = 0;
        float humidity = 0;

        if(xSemaphoreTake(xMutexTempHumi, (TickType_t)10) == pdTRUE) {
            // Update global variables for temperature and humidity
            // Reading temperature in Celsius
            temperature = dht20.getTemperature();
            // Reading humidity
            humidity = dht20.getHumidity();
            xSemaphoreGive(xMutexTempHumi);
        } else {
            Serial.println("⚠️ ERROR: cannot get Mutex, skip updating temperature and humidity!");
        }
        
        // Check if any reads failed and exit early
        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            temperature = humidity =  -1;
            //return;
        }

        //Update global variables for temperature and humidity
        glob_temperature = temperature;
        glob_humidity = humidity;

        // Print the results
        
        // Serial.print("Humidity: ");
        // Serial.print(humidity);
        // Serial.print("%  Temperature: ");
        // Serial.print(temperature);
        // Serial.println("°C");

        char tempBuf[32];
        sprintf(tempBuf, "Temp:%.1f\xF7" "C", temperature);
        // Hiển thị nhiệt độ ở dòng thứ 3 (y=20), không làm mất dòng Wifi ở trên
        sendToOLED(0, 20, tempBuf);
        
        sprintf(tempBuf, "Humi:%.1f%%", humidity);
        sendToOLED(68, 20, tempBuf);
        
        vTaskDelay(5000);
    }
    
}