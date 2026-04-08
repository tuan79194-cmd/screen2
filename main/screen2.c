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

// --- GIAI ĐOẠN 1: CÂY GIA PHẢ LVGL ---

    // Đời 1: Cụ Tổ (Màn hình hiện tại)
    lv_obj_t * main_screen = lv_scr_act();
    // Đổi màu nền màn hình thành màu xám nhạt cho dễ nhìn
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0xE0E0E0), 0);

    // Đời 2: Một cái Khung (Panel) - Con của màn hình
    lv_obj_t * panel = lv_obj_create(main_screen);
    lv_obj_set_size(panel, 140, 160);       // Rộng 140, Cao 160
    lv_obj_center(panel);                   // Đặt cái khung ra giữa màn hình

    // Đời 3a: Một Nút bấm (Button) - Con của Khung (Panel)
    lv_obj_t * btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 0); // Neo nút lên phía trên cùng của Khung
    lv_obj_set_width(btn, 100);

    // Đời 4: Một Dòng chữ (Label) - Con của Nút bấm
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "Bat Wi-Fi");
    lv_obj_center(label);                   // Đặt chữ vào giữa nút bấm

    // Đời 3b: Một Công tắc (Switch) - Cũng là Con của Khung (Panel)
    lv_obj_t * sw = lv_switch_create(panel);
    lv_obj_align(sw, LV_ALIGN_BOTTOM_MID, 0, 0); // Neo công tắc xuống đáy Khung
    

    // 3. Vòng lặp duy trì hệ thống
    while (1) {
        lv_timer_handler(); 
            vTaskDelay(pdMS_TO_TICKS(60));
    }
}