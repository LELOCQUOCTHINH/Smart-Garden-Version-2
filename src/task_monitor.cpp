#include "task_monitor.h"
#include "task_oled.h"

void Task_Monitor(void *pvParameters) {
    SystemState_t lastState = STATE_NORMAL;

    while(1) {
        SystemState_t newState = STATE_NORMAL;

        if (xSemaphoreTake(my_ctx->mutex, portMAX_DELAY) == pdTRUE) {
            // ĐỒNG BỘ DỮ LIỆU TỪ CẢM BIẾN VÀO my_ctx TRƯỚC KHI SO SÁNH
            if(xSemaphoreTake(xMutexTempHumi, (TickType_t)10) == pdTRUE) {
                my_ctx->temp = glob_temperature;
                my_ctx->humi = glob_humidity;
                xSemaphoreGive(xMutexTempHumi);
            } else {
                Serial.println("⚠️ [MONITOR] Không lấy được Mutex TempHumi, skip cập nhật temp/humi!");
            }
            
            if(xSemaphoreTake(xMutexSoilMoisture, (TickType_t)10) == pdTRUE) {
                my_ctx->soil = glob_soil_moisture;
                xSemaphoreGive(xMutexSoilMoisture);
            } else {
                Serial.println("⚠️ [MONITOR] Không lấy được Mutex Soil, skip cập nhật soil moisture!");
            }

            // Logic kiểm tra ngưỡng (Critical ưu tiên trước)
            // 1. Kiểm tra mức NGUY HIỂM (CRITICAL) trước
            if (my_ctx->temp > my_ctx->limits.temp_crit || 
                my_ctx->humi < my_ctx->limits.humi_crit || 
                my_ctx->soil < my_ctx->limits.soil_crit) 
            {
                newState = STATE_CRITICAL;
            } 
            // 2. Kiểm tra mức CẢNH BÁO (WARNING)
            else if (my_ctx->temp > my_ctx->limits.temp_warn || 
                     my_ctx->humi < my_ctx->limits.humi_warn || 
                     my_ctx->soil < my_ctx->limits.soil_warn) 
            {
                newState = STATE_WARNING;
            }
            
            my_ctx->currentState = newState;
            xSemaphoreGive(my_ctx->mutex);
        }

        // Nếu trạng thái thay đổi, "Raise" semaphore để các task khác biết
        if (newState != lastState) {
            xSemaphoreGive(my_ctx->stateSem); 
            lastState = newState;

            // Cập nhật OLED hàng 40
            const char* stateText = (newState == STATE_CRITICAL) ? "STATUS: CRITICAL!" : 
                                    (newState == STATE_WARNING)  ? "STATUS: WARNING   " : 
                                                                   "STATUS: NORMAL    ";
            sendToOLED(0, 40, stateText);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}