#include "task_relay_controller.h"

void Task_RelayController(void *pvParameters) {
    RelayEvent event;
    // Tạo Mutex và Queue cho Relay
    xMutexRelays = xSemaphoreCreateMutex();
    glob_relayQueue = xQueueCreate(10, sizeof(RelayEvent));
    
    for(int i = 0; i < MAX_RELAYS; i++) {
        glob_relays[i].active = false;
    }

    while(1) {
        // Đợi có tin nhắn nhảy vào Queue thì mới thức dậy làm việc
        if (xQueueReceive(glob_relayQueue, &event, portMAX_DELAY) == pdTRUE) {
            
            if (xSemaphoreTake(xMutexRelays, portMAX_DELAY) == pdTRUE) {
                for (int i = 0; i < MAX_RELAYS; i++) {
                    if (glob_relays[i].active) {
                        pinMode(glob_relays[i].gpio, OUTPUT);
                        
                        // 1. Nếu là lệnh từ Web (Bật/tắt tay)
                        if (event.eventType == 0) {
                            if (glob_relays[i].mode == 0) { 
                                digitalWrite(glob_relays[i].gpio, glob_relays[i].state ? HIGH : LOW);
                            }
                        }
                        // 2. Nếu là lệnh từ AI (TinyML)
                        else if (event.eventType == 1) {
                            if (glob_relays[i].mode == 1) { 
                                glob_relays[i].state = event.aiState; 
                                digitalWrite(glob_relays[i].gpio, glob_relays[i].state ? HIGH : LOW);
                            }
                        }
                    }
                }
                xSemaphoreGive(xMutexRelays);
            }
        }
    }
}