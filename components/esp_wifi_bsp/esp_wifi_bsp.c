#include <stdio.h>
#include <string.h>
#include "esp_wifi_bsp.h"
#include "esp_event.h" 
#include "nvs_flash.h"
#include "esp_log.h"
#include "web_server_bsp.h"

static const char *TAG = "WIFI_BSP";
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void espwifi_Init(void)
{
    nvs_flash_init();                    
    esp_netif_init();                    
    esp_event_loop_create_default();     
    esp_netif_create_default_wifi_sta(); 
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); 
    esp_wifi_init(&cfg);                                 
    
    esp_event_handler_instance_t Instance_WIFI_IP;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &Instance_WIFI_IP);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &Instance_WIFI_IP);
    
    // Sử dụng biến từ Menuconfig thay vì gõ cứng chuỗi "PDCN"
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    
    esp_wifi_set_mode(WIFI_MODE_STA);               
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config); 
    esp_wifi_start(); // Bật ăng-ten (sự kiện WIFI_EVENT_STA_START sẽ được gọi sau dòng này)
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "Da bat Wifi, dang ket noi den: %s\n", CONFIG_WIFI_SSID);
        esp_wifi_connect(); // Tự động kết nối ngay khi khởi động
    }
    else if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        char ip[25];
        uint32_t pxip = event->ip_info.ip.addr;
        sprintf(ip, "%d.%d.%d.%d", (uint8_t)(pxip), (uint8_t)(pxip >> 8), (uint8_t)(pxip >> 16), (uint8_t)(pxip >> 24));
        ESP_LOGI(TAG, "KET NOI THANH CONG! IP: %s", ip);
        // --- GỌI TỔNG ĐÀI WEB SERVER RA LÀM VIỆC ---
        web_server_start();
    }
    else if(event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Mat mang hoac sai pass! Dang thu ket noi lai...\n");
        esp_wifi_connect(); // Tự động kết nối lại nếu rớt mạng
    }
}