#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#define SDA_PIN 11
#define SCL_PIN 12

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "task_oled.h"

extern float glob_temperature;
extern float glob_humidity;
extern int glob_soil_moisture;

extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;
extern String LOCAL_SERVER;

extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;

extern volatile int neo_state;
extern SemaphoreHandle_t xMutexNeoState;

extern volatile uint16_t blinkingInterval;
extern SemaphoreHandle_t xMutexBlinkingInterval;
#endif