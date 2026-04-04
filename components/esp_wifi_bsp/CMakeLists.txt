idf_component_register(SRCS "esp_wifi_bsp.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_wifi esp_event nvs_flash esp_netif freertos web_server_bsp)