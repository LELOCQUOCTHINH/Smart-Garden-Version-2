#include "led_blinky.h"

void led_blinky(void *pvParameters){
  pinMode(LED_GPIO, OUTPUT);
  
  int current_blinking_interval = 1000U;
  float local_temp = 25.0;               // Biến lưu nhiệt độ cục bộ trong task

  while(1) {
    // ------------------------------------------------
    // 1. DÙNG SEMAPHORE (MUTEX) ĐỂ ĐỒNG BỘ TASK
    // ------------------------------------------------
    // Đợi tối đa 10 ticks để lấy quyền truy cập biến nhiệt độ toàn cục
    if(xSemaphoreTake(xMutexTempHumi, (TickType_t)10) == pdTRUE) 
    {
        // (CRITICAL SECTION) - An toàn sao chép dữ liệu
        local_temp = glob_temperature; 
        
        // Trả lại Mutex ngay lập tức để Task Temp có thể ghi dữ liệu mới vào
        xSemaphoreGive(xMutexTempHumi); 
    } 
    else 
    {
        Serial.println("⚠️ [LED TASK] Không lấy được Mutex, sử dụng nhiệt độ cũ!");
    }

    // ------------------------------------------------
    // 2. Tìm kiếm Interval tương ứng với mảng cấu hình do User thiết lập
    // ------------------------------------------------
    if(xSemaphoreTake(xMutexLedStates, (TickType_t)10) == pdTRUE) {
        if (numLedStates > 0) {
            bool found = false;
            // Duyệt qua danh sách ngưỡng (Giả sử User/JS đã sắp xếp tăng dần)
            for (int i = 0; i < numLedStates; i++) {
                if (local_temp < ledStates[i].tempThreshold) {
                    current_blinking_interval = ledStates[i].interval;
                    found = true;
                    break;
                }
            }
            // Nếu nhiệt độ cao hơn tất cả các ngưỡng, dùng state cuối cùng
            if (!found) {
                current_blinking_interval = ledStates[numLedStates - 1].interval;
            }
        }
        xSemaphoreGive(xMutexLedStates); 
    }

    // ------------------------------------------------
    // 3. THỰC THI NHẤP NHÁY
    // ------------------------------------------------
    digitalWrite(LED_GPIO, HIGH);  
    vTaskDelay(pdMS_TO_TICKS(current_blinking_interval));
    digitalWrite(LED_GPIO, LOW);  
    vTaskDelay(pdMS_TO_TICKS(current_blinking_interval));
  }
}