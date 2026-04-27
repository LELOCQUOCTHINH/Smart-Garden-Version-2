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