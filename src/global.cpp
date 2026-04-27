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

volatile int neo_state = 0; // Biến toàn cục để lưu trạng thái của NeoPixel
SemaphoreHandle_t xMutexNeoState = xSemaphoreCreateMutex(); // Mutex để bảo vệ truy cập vào neo_state

volatile uint16_t blinkingInterval = 1000U; // Biến toàn cục để lưu khoảng thời gian nháy của LED, mặc định là 1000ms
SemaphoreHandle_t xMutexBlinkingInterval = xSemaphoreCreateMutex(); // Mutex để bảo vệ truy cập vào blinkingInterval

SemaphoreHandle_t xMutexTempHumi = xSemaphoreCreateMutex(); // Mutex để bảo vệ truy cập vào glob_temperature và glob_humidity
SemaphoreHandle_t xMutexSoilMoisture = xSemaphoreCreateMutex(); // Mutex để bảo vệ truy cập vào glob_soil_moisture

LedState ledStates[MAX_LED_STATES] = {
    {25.0, 2000},
    {33.0, 500},
    {999.0, 100} // Ngưỡng cao nhất để hứng các giá trị vượt mốc 33
};
int numLedStates = 3;

SemaphoreHandle_t xMutexLedStates = xSemaphoreCreateMutex();