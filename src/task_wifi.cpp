#include "task_wifi.h"

void startAP()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(String(SSID_AP), String(PASS_AP));
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());

    String ipStr = "IP: " + WiFi.softAPIP().toString();
    
    // Gửi lên OLED ở 2 dòng khác nhau (Thêm khoảng trắng để ghi đè sạch chữ cũ nếu có)
    sendToOLED(0, 0, "Wifi: AP Mode       "); 
    sendToOLED(0, 10, ipStr.c_str());

    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
}

void startSTA()
{
    if (WIFI_SSID.isEmpty())
    {
        vTaskDelete(NULL);
    }

    WiFi.mode(WIFI_STA);

    if (WIFI_PASS.isEmpty())
    {
        WiFi.begin(WIFI_SSID.c_str());
    }
    else
    {
        WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
    }

    // Nên in ra trạng thái đang kết nối để UI không bị đơ
    sendToOLED(0, 0, "Wifi: Connecting... ");
    sendToOLED(0, 10, "                    "); // Xóa dòng IP cũ đi

    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // Lấy IP do Router cấp phát
    String ipStr = "IP: " + WiFi.localIP().toString();

    // Gửi lên OLED thông tin thành công
    sendToOLED(0, 0, "Wifi: STA Mode      ");
    sendToOLED(0, 10, ipStr.c_str());

    //Give a semaphore here
    xSemaphoreGive(xBinarySemaphoreInternet);
}

bool Wifi_reconnect()
{
    const wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED)
    {
        return true;
    }
    startSTA();
    return false;
}
