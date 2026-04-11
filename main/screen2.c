#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lvgl_port.h" // Chỉ cần include cổng chào
#include "esp_wifi_bsp.h"
#include "esp_log.h"
#include "led_bsp.h"
// LV_FONT_DECLARE(my_font_vietnam);

static const char *TAG = "screen2";

// =========================================================
// KHAI BÁO CÁC BIẾN TOÀN CỤC (UI OBJECTS)
// =========================================================
lv_obj_t * scr_home;        // Màn hình 1 : Trang chủ (Wi-Fi)
lv_obj_t * scr_control;     // Màn hình 2 : Điều khiển đèn

lv_obj_t * my_wifi_btn = NULL;
lv_obj_t * my_wifi_label = NULL;

lv_obj_t * ui_led = NULL;   // Cái đèn ảo trên màn hình

// =========================================================
// CÁC HÀM XỬ LÝ SỰ KIỆN CHUYỂN TRANG
// =========================================================

static void btn_go_control_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_scr_load_anim(scr_control, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
    }
}

static void btn_go_home_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_scr_load_anim(scr_home, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
    }
}

// =========================================================
// CÁC HÀM XỬ LÝ SỰ KIỆN NÚT BẤM
// =========================================================

// 1. Sự kiện Wi-Fi
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

// Hàm cập nhật UI Wi-Fi (Được gọi từ luồng mạng)
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

// 2. Sư kiện bật / tắt đèn
static void btn_turn_on_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        // Cập nhật UI: Cho đèn ảo sáng lên
        lv_led_on(ui_led);
        led_bsp_set_state(true); // <--- RA LỆNH MẠCH THẬT BẬT
    }
}

static void btn_turn_off_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        // Cập nhật UI: Cho đèn ảo tắt đi
        lv_led_off(ui_led);
        led_bsp_set_state(false); // <--- RA LỆNH MẠCH THẬT TẮT
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

    led_bsp_init(); // <--- GỌI QUẢN GIA BẬT ĐIỆN RA SETUP CHÂN GPIO

 // ==========================================
    // KHỞI TẠO SCREEN 1: TRANG CHỦ (WI-FI)
    // ==========================================
    scr_home = lv_obj_create(NULL);
    
    // Nút Wi-Fi (Dịch lên trên một chút để nhường chỗ cho nút chuyển trang)
    my_wifi_btn = lv_btn_create(scr_home);
    lv_obj_align(my_wifi_btn, LV_ALIGN_CENTER, 0, -40); // Căn giữa, đẩy lên 40px
    lv_obj_set_size(my_wifi_btn, 140, 50);
    my_wifi_label = lv_label_create(my_wifi_btn);
    lv_label_set_text(my_wifi_label, "Bat Wi-Fi");
    lv_obj_center(my_wifi_label);
    lv_obj_add_event_cb(my_wifi_btn, wifi_connect_event_handler, LV_EVENT_ALL, NULL);

    // Nút chuyển sang trang Điều khiển
    lv_obj_t * btn_go_ctrl = lv_btn_create(scr_home);
    lv_obj_align(btn_go_ctrl, LV_ALIGN_CENTER, 0, 40); // Căn giữa, đẩy xuống 40px
    lv_obj_set_size(btn_go_ctrl, 140, 50);
    lv_obj_set_style_bg_color(btn_go_ctrl, lv_color_hex(0x2980B9), 0); // Màu xanh dương
    lv_obj_t * lbl_go_ctrl = lv_label_create(btn_go_ctrl);
    lv_label_set_text(lbl_go_ctrl, "Dieu Khien Den ->");
    lv_obj_center(lbl_go_ctrl);
    lv_obj_add_event_cb(btn_go_ctrl, btn_go_control_cb, LV_EVENT_ALL, NULL);

    // ==========================================
    // KHỞI TẠO SCREEN 2: TRANG ĐIỀU KHIỂN ĐÈN
    // ==========================================
    scr_control = lv_obj_create(NULL);

    // Nút Quay Về (Để ở góc trên cùng bên trái)
    lv_obj_t * btn_back = lv_btn_create(scr_control);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_set_size(btn_back, 80, 35);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x7F8C8D), 0); // Màu xám
    lv_obj_t * lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "<- Back");
    lv_obj_center(lbl_back);
    lv_obj_add_event_cb(btn_back, btn_go_home_cb, LV_EVENT_ALL, NULL);

// 1. Tạo CÁI ĐÈN ẢO (LED Widget)
    ui_led = lv_led_create(scr_control);
    lv_obj_align(ui_led, LV_ALIGN_CENTER, 0, -60); // Căn giữa, đẩy lên trên
    lv_obj_set_size(ui_led, 60, 60);               // Đèn to 60x60 pixel
    lv_led_set_color(ui_led, lv_color_hex(0xFF0000)); // Đặt đèn màu ĐỎ
    lv_led_off(ui_led);                            // Mặc định ban đầu là tắt (màu tối)

    // 2. Tạo Nút BẬT (ON) - Đặt bên trái
    lv_obj_t * btn_on = lv_btn_create(scr_control);
    lv_obj_align(btn_on, LV_ALIGN_CENTER, -40, 40); // Đẩy sang trái 40px, xuống 40px
    lv_obj_set_size(btn_on, 70, 50);
    lv_obj_set_style_bg_color(btn_on, lv_color_hex(0xE74C3C), 0); // Đỏ
    lv_obj_t * lbl_on = lv_label_create(btn_on);
    lv_label_set_text(lbl_on, "ON");
    lv_obj_center(lbl_on);
    lv_obj_add_event_cb(btn_on, btn_turn_on_cb, LV_EVENT_ALL, NULL);

    // 3. Tạo Nút TẮT (OFF) - Đặt bên phải
    lv_obj_t * btn_off = lv_btn_create(scr_control);
    lv_obj_align(btn_off, LV_ALIGN_CENTER, 40, 40); // Đẩy sang phải 40px, xuống 40px
    lv_obj_set_size(btn_off, 70, 50);
    lv_obj_set_style_bg_color(btn_off, lv_color_hex(0x34495E), 0); // Tối màu
    lv_obj_t * lbl_off = lv_label_create(btn_off);
    lv_label_set_text(lbl_off, "OFF");
    lv_obj_center(lbl_off);
    lv_obj_add_event_cb(btn_off, btn_turn_off_cb, LV_EVENT_ALL, NULL);

    // ==========================================
    // HIỂN THỊ MÀN HÌNH ĐẦU TIÊN
    // ==========================================
    lv_scr_load(scr_home);
    // 3. Vòng lặp duy trì hệ thống
    while (1) {
        // Xin chìa khóa
        if (lvgl_port_lock(0)) {
            lv_timer_handler(); // Cho phép LVGL vẽ màn hình
            lvgl_port_unlock(); // Vẽ xong thì trả chìa khóa lại ngay
        }
        vTaskDelay(pdMS_TO_TICKS(60));
    }
}