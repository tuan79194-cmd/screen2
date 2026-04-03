idf_component_register(SRCS "lvgl_port.c"
                       INCLUDE_DIRS "."
                       REQUIRES lvgl esp_timer lcd_bsp esp_touch
                       )