#include "global.h"
float glob_temperature = 0;
float glob_humidity = 0;

String WIFI_SSID;
String WIFI_PASS;
String CORE_IOT_TOKEN;
String CORE_IOT_SERVER;
String CORE_IOT_PORT;

String ssid = "ESP32-YOUR NETWORK HERE!!!";
String password = "12345678";
String wifi_ssid = "abcde";
String wifi_password = "123456789";
boolean isWifiConnected = false;
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();

volatile int neo_state = 0; // Biến toàn cục để lưu trạng thái của NeoPixel
SemaphoreHandle_t xMutexNeoState = xSemaphoreCreateMutex(); // Mutex để bảo vệ truy cập vào neo_state