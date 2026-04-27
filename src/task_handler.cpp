#include <task_handler.h>

void handleWebSocketMessage(String message)
{
    Serial.println(message);
    StaticJsonDocument<1024> doc;

    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        Serial.println("❌ Lỗi parse JSON!");
        return;
    }
    
    if (doc["page"] == "device")
    {
        JsonObject value = doc["value"];
        if (!value.containsKey("gpio") || !value.containsKey("status"))
        {
            Serial.println("⚠️ JSON thiếu thông tin gpio hoặc status");
            return;
        }

        int gpio = value["gpio"];
        String status = value["status"].as<String>();

        Serial.printf("⚙️ Điều khiển GPIO %d → %s\n", gpio, status.c_str());
        pinMode(gpio, OUTPUT);
        if (status.equalsIgnoreCase("ON"))
        {
            digitalWrite(gpio, HIGH);
            Serial.printf("🔆 GPIO %d ON\n", gpio);
        }
        else if (status.equalsIgnoreCase("OFF"))
        {
            digitalWrite(gpio, LOW);
            Serial.printf("💤 GPIO %d OFF\n", gpio);
        }
    }
    else if (doc["page"] == "setting")
    {
        String WIFI_SSID = doc["value"]["ssid"].as<String>();
        String WIFI_PASS = doc["value"]["password"].as<String>();
        String CORE_IOT_TOKEN = doc["value"]["token"].as<String>();
        String CORE_IOT_SERVER = doc["value"]["server"].as<String>();
        String CORE_IOT_PORT = doc["value"]["port"].as<String>();
        String LOCAL_SERVER = doc["value"]["local_server"].as<String>();

        Serial.println("📥 Nhận cấu hình từ WebSocket:");
        Serial.println("SSID: " + WIFI_SSID);
        Serial.println("PASS: " + WIFI_PASS);
        Serial.println("TOKEN: " + CORE_IOT_TOKEN);
        Serial.println("SERVER: " + CORE_IOT_SERVER);
        Serial.println("PORT: " + CORE_IOT_PORT);
        Serial.println("LOCAL_SERVER: " + LOCAL_SERVER);

        // 👉 Gọi hàm lưu cấu hình
        Save_info_File(WIFI_SSID, WIFI_PASS, CORE_IOT_TOKEN, CORE_IOT_SERVER, CORE_IOT_PORT, LOCAL_SERVER);

        // Phản hồi lại client (tùy chọn)
        String msg = "{\"status\":\"ok\",\"page\":\"setting_saved\"}";
        ws.textAll(msg);
    }

    else if (doc["page"] == "led_config")
    {
        // Ép kiểu nó thành một mảng (JsonArray)
        JsonArray states = doc["value"].as<JsonArray>();
        
        if (xSemaphoreTake(xMutexLedStates, portMAX_DELAY) == pdTRUE) {
            numLedStates = 0;
            
            for (JsonObject state : states) {
                if (numLedStates < MAX_LED_STATES) {
                    ledStates[numLedStates].tempThreshold = state["temp"];
                    ledStates[numLedStates].interval = state["interval"];
                    numLedStates++;
                }
            }
            xSemaphoreGive(xMutexLedStates);
            Serial.println("✅ [WEB] Đã cập nhật cấu hình LED Blinky mới từ Web!");
        }
    }
}
