#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lvgl_port.h" // Chỉ cần include cổng chào
#include "esp_wifi_bsp.h"
#include "esp_log.h"
// LV_FONT_DECLARE(my_font_vietnam);
// Biến toàn cục để lưu trữ Nút và Nhãn
lv_obj_t * my_wifi_btn = NULL;
lv_obj_t * my_wifi_label = NULL;

static const char *TAG = "screen2";

static void wifi_connect_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, ">>> LVGL: Ra lenh khoi dong Wi-Fi!\n");

        // --- BƯỚC A: CẬP NHẬT GIAO DIỆN (UI FEEDBACK) ---
        lv_obj_t * label = lv_obj_get_child(btn, 0); 
        lv_label_set_text(label, "Dang ket noi...");
        
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE); 
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x808080), 0); // Đổi màu xám

        // --- BƯỚC B: GỌI HÀM PHẦN CỨNG (HARDWARE TRIGGER) ---
        espwifi_connect_to("Xe Dap Dien Thanh Dat", "tuanhuyen");
    }
}

// Hàm "Đường dây nóng" để Wi-Fi gọi vào báo cáo kết quả
void ui_update_wifi_result(bool is_success) {
    if (my_wifi_label == NULL || my_wifi_btn == NULL) return;

    // QUAN TRỌNG: Khóa LVGL lại trước khi đổi giao diện từ luồng khác
    if (lvgl_port_lock(0)) {
        if (is_success) {
            lv_label_set_text(my_wifi_label, "Da Ket Noi!");
            lv_obj_set_style_bg_color(my_wifi_btn, lv_color_hex(0x27AE60), 0); // Xanh lá
        } else {
            lv_label_set_text(my_wifi_label, "Loi! Thu lai");
            lv_obj_set_style_bg_color(my_wifi_btn, lv_color_hex(0xE74C3C), 0); // Đỏ
            lv_obj_add_flag(my_wifi_btn, LV_OBJ_FLAG_CLICKABLE); // Mở khóa nút
        }
        // Mở khóa LVGL ra cho màn hình tiếp tục vẽ
        lvgl_port_unlock();
    }
}

// =========================================================
// BƯỚC 2: TRONG HÀM app_main (Giao diện)
// =========================================================

void app_main(void)
{

    // 1. Gọi quản gia ra setup mọi thứ (Màn hình, Cảm ứng, LVGL)
    lvgl_port_init();
    // Kịch bản 1: Chuẩn bị nền tảng Wi-Fi (chưa kết nối)
    espwifi_Init();

    // Kịch bản 2: Tự phát Wi-Fi để mang ra đồng
    // espwifi_Init_AP();

    // 3. TẠO GIAO DIỆN NÚT BẤM (Gộp chung vào biến toàn cục)
    // Khởi tạo khung nút
    my_wifi_btn = lv_btn_create(lv_scr_act());
    lv_obj_center(my_wifi_btn);
    lv_obj_set_size(my_wifi_btn, 200, 50); // Cố tình cho nút to ra xíu để chứa đủ chữ
    
    // Khởi tạo dòng chữ trong nút
    my_wifi_label = lv_label_create(my_wifi_btn);
    lv_label_set_text(my_wifi_label, "Bat Wi-Fi");
    lv_obj_center(my_wifi_label);

    // Gắn hàm sự kiện vào nút
    lv_obj_add_event_cb(my_wifi_btn, wifi_connect_event_handler, LV_EVENT_ALL, NULL);



    // 3. Vòng lặp duy trì hệ thống
    while (1) {
        // Xin chìa khóa
        if (lvgl_port_lock(0)) {
            lv_timer_handler(); // Cho phép LVGL vẽ màn hình
            lvgl_port_unlock(); // Vẽ xong thì trả chìa khóa lại ngay
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}