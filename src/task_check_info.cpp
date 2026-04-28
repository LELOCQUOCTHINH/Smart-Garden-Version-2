#include "task_check_info.h"

void Load_info_File()
{
  File file = LittleFS.open("/info.dat", "r");
  if (!file)
  {
    return;
  }
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, file);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
  }
  else
  {
    WIFI_SSID = strdup(doc["WIFI_SSID"]);
    WIFI_PASS = strdup(doc["WIFI_PASS"]);
    CORE_IOT_TOKEN = strdup(doc["CORE_IOT_TOKEN"]);
    CORE_IOT_SERVER = strdup(doc["CORE_IOT_SERVER"]);
    CORE_IOT_PORT = strdup(doc["CORE_IOT_PORT"]);

    if (doc.containsKey("LOCAL_SERVER")) {
      LOCAL_SERVER = strdup(doc["LOCAL_SERVER"]);
    } else {
      Serial.println("⚠️ Warning: LOCAL_SERVER not found in config, set to empty string");
      LOCAL_SERVER = "";
    }
  }
  file.close();
}

void Delete_info_File()
{
  if (LittleFS.exists("/info.dat"))
  {
    LittleFS.remove("/info.dat");
  }
  ESP.restart();
}

void Save_info_File(String wifi_ssid, String wifi_pass, String CORE_IOT_TOKEN, String CORE_IOT_SERVER, String CORE_IOT_PORT, String LOCAL_SERVER)
{
  Serial.println(wifi_ssid);
  Serial.println(wifi_pass);

  DynamicJsonDocument doc(4096);
  doc["WIFI_SSID"] = wifi_ssid;
  doc["WIFI_PASS"] = wifi_pass;
  doc["CORE_IOT_TOKEN"] = CORE_IOT_TOKEN;
  doc["CORE_IOT_SERVER"] = CORE_IOT_SERVER;
  doc["CORE_IOT_PORT"] = CORE_IOT_PORT;
  doc["LOCAL_SERVER"] = LOCAL_SERVER;

  File configFile = LittleFS.open("/info.dat", "w");
  if (configFile)
  {
    serializeJson(doc, configFile);
    configFile.close();
  }
  else
  {
    Serial.println('Unable to save the configuration.');
  }
  ESP.restart();
};

bool check_info_File(bool check)
{
  if (!check)
  {
    if (!LittleFS.begin(true))
    {
      Serial.println("❌ Lỗi khởi động LittleFS!");
      return false;
    }
    Load_info_File();
    Load_LED_Config();
    Load_Neo_Config();
    Load_Thresholds(my_ctx);
    Load_Relay_Config();
  }
  
  if (WIFI_SSID.isEmpty() && WIFI_PASS.isEmpty())
  {
    if (!check)
    {
      startAP();
    }
    return false;
  }
  return true;
}

// 1. Hàm lưu mảng trạng thái LED xuống File
void Save_LED_Config() {
    File file = LittleFS.open("/led_config.json", "w");
    if (!file) {
        Serial.println("❌ Lỗi: Không thể mở file led_config.json để ghi!");
        return;
    }
    
    StaticJsonDocument<1024> doc;
    JsonArray array = doc.to<JsonArray>();
    
    // Khóa Mutex an toàn để đọc mảng từ RAM
    if (xSemaphoreTake(xMutexLedStates, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < numLedStates; i++) {
            JsonObject state = array.createNestedObject();
            state["temp"] = ledStates[i].tempThreshold;
            state["interval"] = ledStates[i].interval;
        }
        xSemaphoreGive(xMutexLedStates);
    }
    
    serializeJson(doc, file);
    file.close();
    Serial.println("💾 Đã lưu cấu hình LED vào LittleFS thành công.");
}

// 2. Hàm đọc mảng trạng thái LED từ File lên RAM
void Load_LED_Config() {
    if (!LittleFS.exists("/led_config.json")) {
        Serial.println("⚠️ Không tìm thấy file led_config.json, sử dụng LED config mặc định trên RAM.");
        return;
    }
    
    File file = LittleFS.open("/led_config.json", "r");
    if (!file) return;

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("❌ Lỗi parse JSON từ file led_config.json!");
        return;
    }

    JsonArray array = doc.as<JsonArray>();
    
    if (xSemaphoreTake(xMutexLedStates, portMAX_DELAY) == pdTRUE) {
        numLedStates = 0;
        for (JsonObject state : array) {
            if (numLedStates < MAX_LED_STATES) {
                ledStates[numLedStates].tempThreshold = state["temp"];
                ledStates[numLedStates].interval = state["interval"];
                numLedStates++;
            }
        }
        xSemaphoreGive(xMutexLedStates);
        Serial.println("📂 Đã tải cấu hình LED từ LittleFS lên RAM.");
    }
}

// 1. Lưu Neo config
void Save_Neo_Config() {
    File file = LittleFS.open("/neo_config.json", "w");
    if (!file) return;
    StaticJsonDocument<1024> doc;
    JsonArray array = doc.to<JsonArray>();
    
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
    serializeJson(doc, file);
    file.close();
    Serial.println("💾 [FS] Đã lưu cấu hình NeoPixel.");
}

// 2. Tải Neo config lên RAM
void Load_Neo_Config() {
    if (!LittleFS.exists("/neo_config.json")) return;
    File file = LittleFS.open("/neo_config.json", "r");
    if (!file) return;

    StaticJsonDocument<1024> doc;
    if (deserializeJson(doc, file)) return;
    file.close();

    JsonArray array = doc.as<JsonArray>();
    if (xSemaphoreTake(xMutexNeoStates, portMAX_DELAY) == pdTRUE) {
        numNeoStates = 0; 
        for (JsonObject state : array) {
            if (numNeoStates < MAX_NEO_STATES) {
                neoStates[numNeoStates].humiThreshold = state["humi"];
                neoStates[numNeoStates].r = state["r"];
                neoStates[numNeoStates].g = state["g"];
                neoStates[numNeoStates].b = state["b"];
                numNeoStates++;
            }
        }
        xSemaphoreGive(xMutexNeoStates);
        Serial.printf("📂 [FS] Đã tải %d trạng thái NeoPixel.\n", numNeoStates);
    }
}

// 1. Lưu ngưỡng cảnh báo
void Save_Thresholds(SystemContext *my_ctx) {
    File file = LittleFS.open("/thresholds.json", "w");
    if (!file) return;

    StaticJsonDocument<512> doc;
    if (xSemaphoreTake(my_ctx->mutex, portMAX_DELAY) == pdTRUE) {
        doc["tw"] = my_ctx->limits.temp_warn;
        doc["tc"] = my_ctx->limits.temp_crit;
        doc["hw"] = my_ctx->limits.humi_warn;
        doc["hc"] = my_ctx->limits.humi_crit;
        doc["sw"] = my_ctx->limits.soil_warn;
        doc["sc"] = my_ctx->limits.soil_crit;
        xSemaphoreGive(my_ctx->mutex);
    }
    serializeJson(doc, file);
    file.close();
}

// 2. Tải ngưỡng cảnh báo lên RAM
void Load_Thresholds(SystemContext *my_ctx) {
    if (!LittleFS.exists("/thresholds.json")) return;
    File file = LittleFS.open("/thresholds.json", "r");
    if (!file) return;

    StaticJsonDocument<512> doc;
    deserializeJson(doc, file);
    file.close();

    if (xSemaphoreTake(my_ctx->mutex, portMAX_DELAY) == pdTRUE) {
        my_ctx->limits.temp_warn = doc["tw"] | 35.0;
        my_ctx->limits.temp_crit = doc["tc"] | 45.0;
        my_ctx->limits.humi_warn = doc["hw"] | 70.0;
        my_ctx->limits.humi_crit = doc["hc"] | 85.0;
        my_ctx->limits.soil_warn = doc["sw"] | 2000;
        my_ctx->limits.soil_crit = doc["sc"] | 3500;
        xSemaphoreGive(my_ctx->mutex);
    }
}

// 1. Hàm lưu cấu hình Relay xuống Flash
void Save_Relay_Config() {
    File file = LittleFS.open("/relay_config.json", "w");
    if (!file) return;

    StaticJsonDocument<1024> doc;
    JsonArray array = doc.to<JsonArray>();

    if (xSemaphoreTake(xMutexRelays, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < MAX_RELAYS; i++) {
            if (glob_relays[i].active) {
                JsonObject relay = array.createNestedObject();
                relay["name"] = glob_relays[i].name;
                relay["gpio"] = glob_relays[i].gpio;
                relay["mode"] = glob_relays[i].mode;
                relay["state"] = glob_relays[i].state;
            }
        }
        xSemaphoreGive(xMutexRelays);
    }
    serializeJson(doc, file);
    file.close();
    Serial.println("💾 [FS] Đã lưu cấu hình Relay vào LittleFS.");
}

// 2. Hàm đọc cấu hình Relay lên RAM lúc khởi động
void Load_Relay_Config() {
    if (!LittleFS.exists("/relay_config.json")) return;
    File file = LittleFS.open("/relay_config.json", "r");
    if (!file) return;

    StaticJsonDocument<1024> doc;
    if (deserializeJson(doc, file)) { file.close(); return; }
    file.close();

    JsonArray array = doc.as<JsonArray>();

    if (xSemaphoreTake(xMutexRelays, portMAX_DELAY) == pdTRUE) {
        // Làm sạch bộ nhớ tạm
        for(int i = 0; i < MAX_RELAYS; i++) glob_relays[i].active = false;

        int slot = 0;
        for (JsonObject relay : array) {
            if (slot < MAX_RELAYS) {
                glob_relays[slot].active = true;
                // Sao chép tên an toàn
                strlcpy(glob_relays[slot].name, relay["name"] | "Thiết bị", sizeof(glob_relays[slot].name));
                glob_relays[slot].gpio = relay["gpio"];
                glob_relays[slot].mode = relay["mode"];
                glob_relays[slot].state = relay["state"];
                
                // Cực kỳ quan trọng: Khôi phục ngay trạng thái phần cứng lúc vừa khởi động
                pinMode(glob_relays[slot].gpio, OUTPUT);
                digitalWrite(glob_relays[slot].gpio, glob_relays[slot].state ? HIGH : LOW);
                
                slot++;
            }
        }
        xSemaphoreGive(xMutexRelays);
        Serial.printf("📂 [FS] Đã tải %d Relay từ LittleFS lên RAM.\n", slot);
    }
}