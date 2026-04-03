#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lvgl_port.h" // Chỉ cần include cổng chào


void app_main(void)
{
    // 1. Gọi quản gia ra setup mọi thứ (Màn hình, Cảm ứng, LVGL)
    lvgl_port_init();

    // 2. Bắt đầu không gian sáng tạo UI của riêng bạn
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(btn, 100, 50);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "CLICK ME");
    lv_obj_center(label);

    // 3. Vòng lặp duy trì hệ thống
    while (1) {
        lv_timer_handler(); 
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}