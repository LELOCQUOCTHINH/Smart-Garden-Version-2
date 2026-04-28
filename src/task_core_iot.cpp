
#include "task_core_iot.h"

constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

// constexpr char LED_STATE_ATTR[] = "ledState";

// volatile int ledMode = 0;
// volatile bool ledState = false;

// constexpr uint16_t BLINKING_INTERVAL_MS_MIN = 10U;
// constexpr uint16_t BLINKING_INTERVAL_MS_MAX = 60000U;
// // volatile uint16_t blinkingInterval = 1000U;

// constexpr int16_t telemetrySendInterval = 10000U;
// constexpr char BLINKING_INTERVAL_ATTR[] = "blinkingInterval";

// Khai báo các Key cho Shared Attributes
constexpr char TELEMETRY_INTERVAL_ATTR[] = "telemetryInterval";
constexpr char TINYML_INTERVAL_ATTR[] = "tinymlInterval";
constexpr char SENSOR_INTERVAL_ATTR[] = "sensorInterval";

constexpr std::array<const char *, 3U> SHARED_ATTRIBUTES_LIST = {
    // LED_STATE_ATTR,
    // BLINKING_INTERVAL_ATTR,
    TELEMETRY_INTERVAL_ATTR,
    TINYML_INTERVAL_ATTR,
    SENSOR_INTERVAL_ATTR
};

void processSharedAttributes(const Shared_Attribute_Data &data)
{
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        // if (strcmp(it->key().c_str(), BLINKING_INTERVAL_ATTR) == 0)
        // {
        //     const uint16_t new_interval = it->value().as<uint16_t>();
        //     if (new_interval >= BLINKING_INTERVAL_MS_MIN && new_interval <= BLINKING_INTERVAL_MS_MAX)
        //     {
        //         if(xSemaphoreTake(xMutexBlinkingInterval, (TickType_t)10) == pdTRUE) 
        //         {
        //             // ----- (CRITICAL SECTION) -----
        //             blinkingInterval = new_interval;
        //             // return the mutex after updating the state
        //             xSemaphoreGive(xMutexBlinkingInterval); 
        //             // ------------------------------------------------
        //         } 
        //         else 
        //         {
        //             Serial.println("⚠️ ERROR: cannot get Mutex, skip updating blinking interval!");
        //         }
                
        //         Serial.print("Blinking interval is set to: ");
        //         Serial.println(new_interval);
        //     }
        // }
        
        String key = it->key().c_str();
        uint32_t new_interval = it->value().as<uint32_t>();
        
        // Khóa Mutex an toàn trước khi cập nhật
        if(xSemaphoreTake(xMutexIntervals, (TickType_t)10) == pdTRUE) 
        {
            if (key == TELEMETRY_INTERVAL_ATTR) {
                // Đảm bảo không set quá nhỏ làm ngập lụt Server
                if (new_interval >= 1000) glob_telemetry_interval = new_interval;
                Serial.printf("⏱️ Telemetry interval set to: %d ms\n", glob_telemetry_interval);
            }
            else if (key == TINYML_INTERVAL_ATTR) {
                if (new_interval >= 500) glob_tinyml_interval = new_interval;
                Serial.printf("⏱️ TinyML interval set to: %d ms\n", glob_tinyml_interval);
            }
            else if (key == SENSOR_INTERVAL_ATTR) {
                if (new_interval >= 500) glob_sensor_interval = new_interval;
                Serial.printf("⏱️ Sensor interval set to: %d ms\n", glob_sensor_interval);
            }
            
            xSemaphoreGive(xMutexIntervals); 
        } 
        else 
        {
            Serial.println("⚠️ ERROR: Cannot get Mutex, skip updating intervals!");
        }
    }
}

// RPC_Response getStateLED(const RPC_Data &data)
// {
//     Serial.println("Received getStateLED request from Server");
    
//     bool currentState = false;

//     if (xSemaphoreTake(xMutexNeoState, (TickType_t)10) == pdTRUE) 
//     {
//         // ----- (CRITICAL SECTION) -----
//         currentState = (neo_state == 1);
//         xSemaphoreGive(xMutexNeoState); 
//         // ------------------------------------------------
//     }
//     else 
//     {
//         Serial.println("⚠️ ERROR: Cannot get Mutex for Read, returning default state!");
//     }

//     Serial.print("Current LED state is: ");
//     Serial.println(currentState);

//     static StaticJsonDocument<JSON_OBJECT_SIZE(1)> doc;
//     doc.clear();
//     doc["getStateLED"] = currentState ? true : false;

//     return RPC_Response(doc.as<JsonVariant>());
// }

// RPC_Response setStateLED(const RPC_Data &data)
// {
//     Serial.println("Received LED state");
//     bool newState = data;
//     if (xSemaphoreTake(xMutexNeoState, (TickType_t)10) == pdTRUE) 
//     {
//         // ----- (CRITICAL SECTION) -----
//         neo_state = newState ? 1 : 0;
        
//         // return the mutex after updating the state
//         xSemaphoreGive(xMutexNeoState); 
//         // ------------------------------------------------
//         Serial.print("LED state change: ");
//         Serial.println(newState);
//     }
//     else 
//     {
//         Serial.println("⚠️ ERROR: cannot get Mutex, skip RPC command!");
//     }

//     static StaticJsonDocument<JSON_OBJECT_SIZE(1)> doc;
//     doc.clear();
//     doc["setStateLED"] = neo_state ? true : false;

//     return RPC_Response(doc.as<JsonVariant>());
// }

// const std::array<RPC_Callback, 2U> callbacks = {
//     RPC_Callback{"setStateLED", setStateLED},
//     RPC_Callback{"getStateLED", getStateLED}
// };

const Shared_Attribute_Callback attributes_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());
const Attribute_Request_Callback attribute_shared_request_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());

void CORE_IOT_sendata(String mode, String feed, String data)
{
    Serial.printf("🌐 [CoreIoT] Gửi %s -> [%s]: %s\n", mode.c_str(), feed.c_str(), data.c_str());
    
    if (mode == "attribute")
    {
        tb.sendAttributeData(feed.c_str(), data);
    }
    else if (mode == "telemetry")
    {
        float value = data.toFloat();
        tb.sendTelemetryData(feed.c_str(), value);
    }
    else
    {
        // handle unknown mode
        Serial.println("⚠️ [CoreIoT] Unknown mode!");
    }
}

void CORE_IOT_reconnect()
{
    if (!tb.connected())
    {
        bool isConnected = false;

        // 1. Try connect to Local Server
        if (LOCAL_SERVER.length() > 0) 
        {
            Serial.print("🌐 Attempting MQTT connection to Local Server: ");
            Serial.println(LOCAL_SERVER);
            
            if (tb.connect(LOCAL_SERVER.c_str(), CORE_IOT_TOKEN.c_str(), CORE_IOT_PORT.toInt(), "ESP32Client", "")) 
            {
                Serial.println("✅ CONNECTED to Local Server!");
                isConnected = true;
            }
        }

        // 2. if local connection failed, try connect to CoreIOT Server
        if (!isConnected) 
        {
            Serial.print("⚠️ Local failed/empty. Falling back to CoreIOT: ");
            Serial.println(CORE_IOT_SERVER);
            
            if (tb.connect(CORE_IOT_SERVER.c_str(), CORE_IOT_TOKEN.c_str(), CORE_IOT_PORT.toInt(), "ESP32Client", "")) 
            {
                Serial.println("✅ CONNECTED to CoreIOT Server!");
                isConnected = true;
            }
        }

        // 3. if both connections failed
        if (!isConnected) 
        {
            Serial.println("❌ Both connections failed. Retrying in next loop...");
            return;
        }

        // ==============================================================
        // if connected to either server, subscribe to RPC and Shared Attributes
        // ==============================================================

        tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());

        
        // Serial.println("Subscribing for RPC...");
        // // if (!tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend()))
        // // {
        // //     // Serial.println("Failed to subscribe for RPC");
        // //     return;
        // // }
        // Serial.println("Subscribe done");

        Serial.println("Subscribing for receive Shared Attributes...");

        if (!tb.Shared_Attributes_Subscribe(attributes_callback))
        {
            // Serial.println("Failed to subscribe for shared attribute updates");
            return;
        }

        Serial.println("Subscribe done");

        if (!tb.Shared_Attributes_Request(attribute_shared_request_callback))
        {
            // Serial.println("Failed to request for shared attributes");
            return;
        }
        tb.sendAttributeData("localIp", WiFi.localIP().toString().c_str());
    }
    else if (tb.connected())
    {
        tb.loop();
    }
}

void iot_monitor_task(void *pvParameters)
{
    unsigned long last_iot_send = 0;
    while (1)
    {
        if (WiFi.status() == WL_CONNECTED && check_info_File(1))
        {
            CORE_IOT_reconnect();
            uint32_t current_tele_interval = 10000;

            if (xSemaphoreTake(xMutexIntervals, (TickType_t)10) == pdTRUE) {
                current_tele_interval = glob_telemetry_interval;
                xSemaphoreGive(xMutexIntervals);
            } else {
                Serial.println("⚠️ ERROR: cannot get Mutex for Intervals, using default telemetry interval!");
            }

            if (millis() - last_iot_send >= current_tele_interval) 
            {
                last_iot_send = millis();

                float currentTemp = 0;
                float currentHumi = 0;
                float currentSoilMoisture = 0;
                if(xSemaphoreTake(xMutexTempHumi, (TickType_t)10) == pdTRUE) 
                {
                    // ----- (CRITICAL SECTION) -----
                    currentTemp = glob_temperature;
                    currentHumi = glob_humidity;
                    xSemaphoreGive(xMutexTempHumi); 
                    // ------------------------------------------------

                    // Serial.printf("Current Temp: %.2f, Current Humi: %.2f\n", currentTemp, currentHumi);
                }
                else 
                {
                    Serial.println("⚠️ ERROR: cannot get Mutex for Temp/Humi, sending default values!");
                }

                if(xSemaphoreTake(xMutexSoilMoisture, (TickType_t)10) == pdTRUE) 
                {
                    // ----- (CRITICAL SECTION) -----
                    currentSoilMoisture = glob_soil_moisture;
                    xSemaphoreGive(xMutexSoilMoisture); 
                    // ------------------------------------------------

                    // Serial.printf("Current Soil Moisture: %.2f\n", currentSoilMoisture);
                }
                else 
                {
                    Serial.println("⚠️ ERROR: cannot get Mutex for Soil Moisture, sending default value!");
                }

                String payload = "{\"deviceName\":\"ESP32_YoloUNO\",";
                payload += "\"temperature\":" + String(currentTemp) + ",";
                payload += "\"humidity\":" + String(currentHumi) + ",";
                payload += "\"soil_moisture\":" + String(currentSoilMoisture) + "}";

                // Dùng hàm cấp cao của ThingsBoard để gửi chuỗi JSON tự tạo
                tb.sendTelemetryJson(payload.c_str());
                
                Serial.println("Published raw payload: " + payload);
                // Serial.println("📤 Đã push Telemetry lên CoreIoT");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}