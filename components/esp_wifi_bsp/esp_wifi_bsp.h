#ifndef ESP_WIFI_BSP_H
#define ESP_WIFI_BSP_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h" 

// Khởi tạo phần cứng Wi-Fi ở chế độ Bắt Wi-Fi (Station Mode)
void espwifi_Init(void);

// Khởi tạo phần cứng Wi-Fi ở chế độ Tự phát Wi-Fi (Access Point Mode)
void espwifi_Init_AP(void);

// Hàm mới: Ra lệnh kết nối tới một mạng cụ thể (Sẽ được gọi từ LVGL)
// void espwifi_connect_to(const char *ssid, const char *password);

#endif