#ifndef ESP_WIFI_BSP_H
#define ESP_WIFI_BSP_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h" 

// Khởi tạo phần cứng Wi-Fi (Chỉ chạy 1 lần lúc bật máy)
void espwifi_Init(void);

// Hàm mới: Ra lệnh kết nối tới một mạng cụ thể (Sẽ được gọi từ LVGL)
void espwifi_connect_to(const char *ssid, const char *password);

#endif