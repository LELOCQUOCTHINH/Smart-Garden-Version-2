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
    
    else if (doc["page"] == "device") {
        String action = doc["action"].as<String>();
        JsonObject val = doc["value"];
        int gpio = val["gpio"];

        if (xSemaphoreTake(xMutexRelays, portMAX_DELAY) == pdTRUE) {
            // Thêm mới hoặc cập nhật Relay
            if (action == "update") {
                int slot = -1;
                for(int i = 0; i < MAX_RELAYS; i++) {
                    if(glob_relays[i].active && glob_relays[i].gpio == gpio) { slot = i; break; }
                    if(!glob_relays[i].active && slot == -1) { slot = i; }
                }
                
                if (slot != -1) {
                    glob_relays[slot].active = true;
                    glob_relays[slot].gpio = gpio;
                    glob_relays[slot].mode = val["mode"];
                    glob_relays[slot].state = val["state"];

                    if (val.containsKey("name")) {
                        String n = val["name"].as<String>();
                        strlcpy(glob_relays[slot].name, n.c_str(), sizeof(glob_relays[slot].name));
                    }
                }
            } 
            // Xóa Relay
            else if (action == "delete") {
                for(int i = 0; i < MAX_RELAYS; i++) {
                    if(glob_relays[i].active && glob_relays[i].gpio == gpio) {
                        glob_relays[i].active = false;
                        break;
                    }
                }
            }
            xSemaphoreGive(xMutexRelays);

            Save_Relay_Config(); // Lưu cấu hình mới xuống Flash
            
            // Bắn tín hiệu vào Queue để đánh thức Task Relay thực thi ngay lập tức
            RelayEvent ev = {0, 0}; // eventType = 0 (Lệnh Web)
            xQueueSend(glob_relayQueue, &ev, 0);
            
            Serial.printf("⚙️ Đã chuyển lệnh GPIO %d vào Queue\n", gpio);
        }
    }

    else if (doc["page"] == "request_relay_config") {
        StaticJsonDocument<1024> resDoc;
        resDoc["page"] = "relay_config_data";
        JsonArray array = resDoc.createNestedArray("value");
        
        if (xSemaphoreTake(xMutexRelays, portMAX_DELAY) == pdTRUE) {
            for (int i = 0; i < MAX_RELAYS; i++) {
                if (glob_relays[i].active) {
                    JsonObject relay = array.createNestedObject();
                    relay["id"] = glob_relays[i].gpio; // Ta dùng thẳng chân GPIO làm ID trên web
                    relay["name"] = glob_relays[i].name;
                    relay["gpio"] = glob_relays[i].gpio;
                    relay["mode"] = glob_relays[i].mode;
                    relay["state"] = glob_relays[i].state;
                }
            }
            xSemaphoreGive(xMutexRelays);
        }
        
        String response;
        serializeJson(resDoc, response);
        ws.textAll(response);
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
