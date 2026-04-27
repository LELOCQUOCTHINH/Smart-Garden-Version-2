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
    // 2. LOGIC 3 HÀNH VI LED THEO NHIỆT ĐỘ
    // ------------------------------------------------
    if (local_temp < 25.0) {
        // Hành vi 1: Lạnh -> LED chớp rất chậm (2000ms)
        current_blinking_interval = 2000;
    } 
    else if (local_temp >= 25.0 && local_temp < 33.0) {
        // Hành vi 2: Bình thường -> LED chớp vừa (500ms)
        current_blinking_interval = 500;
    } 
    else {
        // Hành vi 3: Nóng (Cảnh báo) -> LED chớp cực nhanh (100ms)
        current_blinking_interval = 100;
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