#include "global.h"
float glob_temperature = 0;
float glob_humidity = 0;
int glob_soil_moisture = 0;

String WIFI_SSID;
String WIFI_PASS;
String CORE_IOT_TOKEN;
String CORE_IOT_SERVER;
String CORE_IOT_PORT;
String LOCAL_SERVER;

String ssid = "ESP32-YOUR NETWORK HERE!!!";
String password = "12345678";
String wifi_ssid = "abcde";
String wifi_password = "123456789";
boolean isWifiConnected = false;
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();

// volatile int neo_state = 0; // Biến toàn cục để lưu trạng thái của NeoPixel
// SemaphoreHandle_t xMutexNeoState = xSemaphoreCreateMutex(); // Mutex để bảo vệ truy cập vào neo_state

// volatile uint16_t blinkingInterval = 1000U; // Biến toàn cục để lưu khoảng thời gian nháy của LED, mặc định là 1000ms
// SemaphoreHandle_t xMutexBlinkingInterval = xSemaphoreCreateMutex(); // Mutex để bảo vệ truy cập vào blinkingInterval

SemaphoreHandle_t xMutexTempHumi = xSemaphoreCreateMutex(); // Mutex để bảo vệ truy cập vào glob_temperature và glob_humidity
SemaphoreHandle_t xMutexSoilMoisture = xSemaphoreCreateMutex(); // Mutex để bảo vệ truy cập vào glob_soil_moisture

//----------------------------------------------led blinky config----------------------------------------------
LedState ledStates[MAX_LED_STATES] = {
    {25.0, 2000},
    {33.0, 500},
    {999.0, 100} // Ngưỡng cao nhất để hứng các giá trị vượt mốc 33
};
int numLedStates = 3;

SemaphoreHandle_t xMutexLedStates = xSemaphoreCreateMutex();

//----------------------------------------------NeoPixel control config---------------------------------------------
NeoState neoStates[MAX_NEO_STATES] = {
    {40.0, 255, 0, 0},    // < 40%: Đỏ
    {70.0, 0, 255, 0},    // < 70%: Xanh lá
    {999.0, 0, 0, 255}    // Khác: Xanh dương
};
int numNeoStates = 3;
SemaphoreHandle_t xMutexNeoStates = xSemaphoreCreateMutex();

//-----------------------------------------------CRITICAL WARNING----------------------------------------------
SystemContext *my_ctx = new SystemContext();

//-----------------------------------------------RELAY CONTROLLER config----------------------------------------------
RelayInfo glob_relays[MAX_RELAYS];
QueueHandle_t glob_relayQueue = NULL;
SemaphoreHandle_t xMutexRelays = NULL;

//-----------------------------------------------SHARED ATTRIBUTES HANDLER----------------------------------------------
uint32_t glob_telemetry_interval = 10000;
uint32_t glob_tinyml_interval = 5000;
uint32_t glob_sensor_interval = 1000;
SemaphoreHandle_t xMutexIntervals = xSemaphoreCreateMutex();