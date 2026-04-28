#ifndef SYSTEM_CONTEXT_H
#define SYSTEM_CONTEXT_H

#include <Arduino.h>
#include <semphr.h>

typedef enum { STATE_NORMAL, STATE_WARNING, STATE_CRITICAL } SystemState_t;

struct Thresholds {
    float temp_warn, temp_crit;
    float humi_warn, humi_crit;
    int soil_warn, soil_crit;
};

// ĐÂY LÀ "TRÁI TIM" CỦA HỆ THỐNG - KHÔNG DÙNG BIẾN TOÀN CỤC
struct SystemContext {
    float temp, humi;
    int soil;
    SystemState_t currentState;
    Thresholds limits;
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();     // Bảo vệ dữ liệu cảm biến
    SemaphoreHandle_t stateSem = xSemaphoreCreateBinary();  // Semaphore để "raise" khi trạng thái thay đổi
};

void Task_Monitor(void *pvParameters);

#endif