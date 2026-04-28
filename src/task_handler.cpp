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

    // save LED config from Web
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
            
            Save_LED_Config();
            Serial.println("✅ [WEB] Đã cập nhật cấu hình LED Blinky mới từ Web!");
        }
    }

    // load LED config lên Web
    else if (doc["page"] == "request_led_config") 
    {
        StaticJsonDocument<1024> resDoc;
        resDoc["page"] = "led_config_data"; // Đánh dấu đây là dữ liệu LED
        JsonArray array = resDoc.createNestedArray("value");
        
        if (xSemaphoreTake(xMutexLedStates, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < numLedStates; i++) {
                JsonObject state = array.createNestedObject();
                state["temp"] = ledStates[i].tempThreshold;
                state["interval"] = ledStates[i].interval;
            }
            xSemaphoreGive(xMutexLedStates);
        }
        
        String response;
        serializeJson(resDoc, response);
        ws.textAll(response); // Gửi mảng LED hiện tại ngược lại cho giao diện Web
    }
    
    // save Neo config from Web
    else if (doc["page"] == "neo_config")
    {
        JsonArray states = doc["value"].as<JsonArray>();
        if (xSemaphoreTake(xMutexNeoStates, portMAX_DELAY) == pdTRUE) {
            numNeoStates = 0;
            for (JsonObject state : states) {
                if (numNeoStates < MAX_NEO_STATES) {
                    neoStates[numNeoStates].humiThreshold = state["humi"];
                    neoStates[numNeoStates].r = state["r"];
                    neoStates[numNeoStates].g = state["g"];
                    neoStates[numNeoStates].b = state["b"];
                    numNeoStates++;
                }
            }
            xSemaphoreGive(xMutexNeoStates);
            Save_Neo_Config(); // Lưu xuống Flash
            Serial.println("✅ [WEB] Đã cập nhật cấu hình NeoPixel mới từ Web!");
        }
    }

    //load Neo config lên Web
    else if (doc["page"] == "request_neo_config") 
    {
        StaticJsonDocument<1024> resDoc;
        resDoc["page"] = "neo_config_data";
        JsonArray array = resDoc.createNestedArray("value");
        if (xSemaphoreTake(xMutexNeoStates, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < numNeoStates; i++) {
                JsonObject state = array.createNestedObject();
                state["humi"] = neoStates[i].humiThreshold;
                state["r"] = neoStates[i].r;
                state["g"] = neoStates[i].g;
                state["b"] = neoStates[i].b;
            }
            xSemaphoreGive(xMutexNeoStates);
        }
        String response;
        serializeJson(resDoc, response);
        ws.textAll(response);
    }

    else if (doc["page"] == "threshold_config") {
        JsonObject val = doc["value"];
        if (xSemaphoreTake(my_ctx->mutex, portMAX_DELAY) == pdTRUE) {
            my_ctx->limits.temp_warn = val["tw"];
            my_ctx->limits.temp_crit = val["tc"];
            my_ctx->limits.humi_warn = val["hw"];
            my_ctx->limits.humi_crit = val["hc"];
            my_ctx->limits.soil_warn = val["sw"];
            my_ctx->limits.soil_crit = val["sc"];
            xSemaphoreGive(my_ctx->mutex);
            Save_Thresholds(my_ctx); // Lưu lại ngay
            // ws.textAll("{\"page\":\"threshold_saved\"}");
            Serial.println("✅ [WEB] Đã cập nhật cấu hình thresholds mới từ Web!");
        }
    }

    else if (doc["page"] == "request_threshold_config") {
        StaticJsonDocument<512> res;
        res["page"] = "threshold_config_data";
        JsonObject val = res.createNestedObject("value");
        val["tw"] = my_ctx->limits.temp_warn;
        val["tc"] = my_ctx->limits.temp_crit;
        val["hw"] = my_ctx->limits.humi_warn;
        val["hc"] = my_ctx->limits.humi_crit;
        val["sw"] = my_ctx->limits.soil_warn;
        val["sc"] = my_ctx->limits.soil_crit;
        String out;
        serializeJson(res, out);
        ws.textAll(out);
    }
}
