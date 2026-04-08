#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lvgl_port.h" // Chỉ cần include cổng chào
#include "esp_wifi_bsp.h"

void app_main(void)
{

    // 1. Gọi quản gia ra setup mọi thứ (Màn hình, Cảm ứng, LVGL)
    lvgl_port_init();
    // Kịch bản 1: Để mạch bắt Wi-Fi nhà bạn
    // espwifi_Init(); 

    // Kịch bản 2: Tự phát Wi-Fi để mang ra đồng
    espwifi_Init_AP();

    // 2. Bắt đầu không gian sáng tạo UI của riêng bạn
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(btn, 100, 50);
    // THÊM DÒNG NÀY ĐỂ TÔ MÀU ĐỎ CHO NÚT
    // 1. Màu khi để bình thường (Đỏ)
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);

    // 2. Màu khi bị bấm xuống (Xanh Lá)
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "CLICK ME");
    lv_obj_center(label);

    // 3. Vòng lặp duy trì hệ thống
    while (1) {
        lv_timer_handler(); 
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}