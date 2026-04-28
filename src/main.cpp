#include "global.h"

// include task
#include "led_blinky.h"
#include "neo_control.h"
#include "task_temp_humi_monitor.h"
#include "tinyml.h"
#include "task_oled.h"
#include "task_soil_moisture.h"
#include "task_check_info.h"
#include "task_toggle_boot.h"
#include "task_wifi.h"
#include "task_webserver.h"
#include "task_core_iot.h"
#include "task_monitor.h"
#include "task_relay_controller.h"

void setup()
{
  Wire.begin(SDA_PIN, SCL_PIN); //setup I2C pins for OLED and DHT20
  Serial.begin(115200);
  
  xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL);
  xTaskCreate(Task_OLED, "Task OLED", 2048, NULL, 2, NULL);
  xTaskCreate(task_temp_humi_monitor, "Task TEMP HUMI Monitor", 4096, NULL, 2, NULL);
  xTaskCreate(Task_SoilMoisture, "Task Soil Moisture", 2048, NULL, 2, NULL);
  xTaskCreate(Task_RelayController, "Task Relay Controller", 2048, NULL, 2, NULL);
  xTaskCreate(Task_Monitor, "Monitor Status", 2048, NULL, 2, NULL);

  xTaskCreate(neo_control_RPC, "Task NEO Control", 2048, NULL, 2, NULL);
  xTaskCreate(tiny_ml_task, "Tiny ML Task", 2048, NULL, 2, NULL);
  xTaskCreate(Task_Toggle_BOOT, "Task_Toggle_BOOT", 4096, NULL, 2, NULL);

  check_info_File(0);
  xTaskCreate(wifi_web_task, "Task WiFi Webserver", 8192, NULL, 2, NULL);
  xTaskCreate(iot_monitor_task, "Task IoT Monitor", 4096, NULL, 2, NULL);
  
}

void loop()
{
}