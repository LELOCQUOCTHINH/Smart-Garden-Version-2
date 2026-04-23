#include "global.h"

#include "led_blinky.h"
#include "neo_control.h"
#include "temp_humi_monitor.h"
#include "tinyml.h"

// include task
#include "task_check_info.h"
#include "task_toggle_boot.h"
#include "task_wifi.h"
#include "task_webserver.h"
#include "task_core_iot.h"

void setup()
{
  Serial.begin(115200);
  check_info_File(0);

  xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL);
  xTaskCreate(neo_control_RPC, "Task NEO Control", 2048, NULL, 2, NULL);
  xTaskCreate(temp_humi_monitor, "Task TEMP HUMI Monitor", 2048, NULL, 2, NULL);
  xTaskCreate(tiny_ml_task, "Tiny ML Task", 2048, NULL, 2, NULL);
  xTaskCreate(Task_Toggle_BOOT, "Task_Toggle_BOOT", 4096, NULL, 2, NULL);
  xTaskCreate(wifi_web_task, "Task WiFi Webserver", 8192, NULL, 2, NULL);
  xTaskCreate(iot_monitor_task, "Task IoT Monitor", 4096, NULL, 2, NULL);
}

void loop()
{
}