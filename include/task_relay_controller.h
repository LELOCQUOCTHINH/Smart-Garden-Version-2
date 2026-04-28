#ifndef TASK_RELAY_CONTROLLER_H
#define TASK_RELAY_CONTROLLER_H 

#define MAX_RELAYS 5

struct RelayInfo {
    char name[32];
    int gpio;
    int mode;  // 0: Manual (Thủ công), 1: AI (Tự động)
    int state; // 0: OFF, 1: ON
    bool active; // Đánh dấu slot này có đang được dùng không
};

// Cấu trúc tin nhắn gửi qua Queue
struct RelayEvent {
    int eventType; // 0 = Lệnh từ Web (Config/Manual), 1 = Lệnh từ TinyML (AI)
    int aiState;   // 0 = OFF, 1 = ON (Chỉ dùng khi eventType == 1)
};


#include "global.h"

void Task_RelayController(void *pvParameters);

#endif