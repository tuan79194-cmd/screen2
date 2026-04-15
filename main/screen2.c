#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lvgl_port.h" // Chỉ cần include cổng chào
#include "esp_wifi_bsp.h"
#include "esp_log.h"
#include "led_bsp.h"
#include "sonar_bsp.h"
#include "esp_timer.h" // Để lấy thời gian tính delta T
#include <math.h>      // Để dùng hàm fabs() tính giá trị tuyệt đối
// LV_FONT_DECLARE(my_font_vietnam);

static const char *TAG = "screen2";

// =========================================================
// KHAI BÁO CÁC BIẾN TOÀN CỤC (UI OBJECTS)
// =========================================================
lv_obj_t * scr_home;        // Màn hình 1 : Trang chủ (Wi-Fi)
lv_obj_t * scr_control;     // Màn hình 2 : Điều khiển đèn
lv_obj_t * scr_speed;       // Màn hình 3 : Radar Tốc độ (Bên trái)

lv_obj_t * my_wifi_btn = NULL;
lv_obj_t * my_wifi_label = NULL;

lv_obj_t * ui_led = NULL;   // Cái đèn ảo trên màn hình
lv_obj_t * ui_arc_label = NULL; // Biến lưu cái nhãn phần trăm của vòng tròn

// --- Biến của màn hình Radar Tốc độ ---
lv_obj_t * ui_meter;                    // Đồng hồ tốc độ
lv_meter_indicator_t * speed_needle;    // Cây kim của đồng hồ
lv_obj_t * ui_speed_label;              // Chữ hiển thị tốc độ số (Digital)
lv_obj_t * ui_dist_label;               // Khoảng cách thô
lv_obj_t * ui_max_speed_label;          // Kỷ lục vận tốc lớn nhất
lv_obj_t * ui_radar_led;                // Đèn trạng thái Radar

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

// Từ Home sang Speed Radar (Qua Trái) -> Màn hình mới trượt từ trái sang (MOVE_RIGHT)
static void btn_go_speed_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_scr_load_anim(scr_speed, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
    }
}

// Từ Speed Radar về Home (Qua Phải) -> Màn hình home trượt từ phải sang (MOVE_LEFT)
static void btn_home_from_speed_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_scr_load_anim(scr_home, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
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

// ---------------------------------------------------------
// SỰ KIỆN KÉO VÒNG TRÒN ARC
// ---------------------------------------------------------
static void arc_value_changed_cb(lv_event_t * e) {
    lv_obj_t * arc = lv_event_get_target(e);

    // Lấy giá trị hiện tại của Arc (từ 0 đến 100)
    int value = lv_arc_get_value(arc);

    // Cập nhật con số hiển thị trên nhãn
    lv_label_set_text_fmt(ui_arc_label, "%d%%", value);

    // In ra terminal để tiện theo dõi
    ESP_LOGI(TAG, "Gia tri do sang dang vuot: %d%%", value);

    // GỌI HÀM PHẦN CỨNG ĐỂ THAY ĐỔI ĐỘ SÁNG LED
    led_bsp_set_brightness((uint8_t)value);
}

// =========================================================
// TASK XỬ LÝ RADAR ĐO TỐC ĐỘ
// =========================================================
// =========================================================
// TASK XỬ LÝ RADAR ĐO TỐC ĐỘ (ĐÃ FIX DELAY + THÊM BỘ LỌC + LOG)
// =========================================================
// Bỏ hết các biến tính vận tốc đi, chúng ta chỉ làm mượt KHOẢNG CÁCH
float smoothed_dist = 100.0; // Khởi tạo mốc 100cm

void radar_task(void *pvParameters) {
    while (1) {
        // Đọc khoảng cách thô từ cảm biến
        float current_dist = sonar_bsp_read_distance_cm();

        // CHỈNH SỬA: Đưa phần tính toán ra ngoài khối LVGL để logic độc lập với giao diện
        if (current_dist > 0.0 && current_dist <= 200.0) { // Chỉ lấy số liệu dưới 2m
            // LỌC MỀM KHOẢNG CÁCH
            smoothed_dist = (0.2 * current_dist) + (0.8 * smoothed_dist);
            
            // THÊM LOG Ở ĐÂY: In ra màn hình console để đối chiếu
            ESP_LOGI(TAG, "Raw: %6.2f cm | Smoothed: %6.2f cm", current_dist, smoothed_dist);
        } else {
            // Log khi vượt tầm đo hoặc bị lỗi
            ESP_LOGW(TAG, "Out of range or Error: %.2f cm", current_dist);
        }

        // Cập nhật giao diện LVGL
        if (lvgl_port_lock(0)) {
            if (current_dist > 0.0 && current_dist <= 200.0) {
                lv_led_set_color(ui_radar_led, lv_color_hex(0x00FF00)); // Đèn Xanh
                
                // Ép kim đồng hồ hiển thị KHOẢNG CÁCH
                lv_meter_set_indicator_value(ui_meter, speed_needle, (int32_t)smoothed_dist);
                
                // Cập nhật chữ to ở giữa đồng hồ
                lv_label_set_text_fmt(ui_speed_label, "%.1f cm", smoothed_dist);
                lv_label_set_text(ui_dist_label, "Distance Meter");
            } else {
                // Quá tầm đo -> Đèn Đỏ, chữ MAX
                lv_led_set_color(ui_radar_led, lv_color_hex(0xE74C3C));
                lv_label_set_text(ui_speed_label, "MAX");
            }
            lvgl_port_unlock();
        }
        
        // Thời gian chờ 50ms cho mỗi lần quét
        vTaskDelay(pdMS_TO_TICKS(500));
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

    // THÊM 2 DÒNG NÀY VÀO ĐÂY:
    sonar_bsp_init(); // Khởi tạo chân TRIG, ECHO cho cảm biến siêu âm

    // ==========================================
    // KHỞI TẠO SCREEN 1: TRANG CHỦ (WI-FI)
    // ==========================================
    scr_home = lv_obj_create(NULL);
    
// Nút sang trái (Radar Tốc độ)
    lv_obj_t * btn_go_speed = lv_btn_create(scr_home);
    lv_obj_align(btn_go_speed, LV_ALIGN_CENTER, 0, -60); 
    lv_obj_set_size(btn_go_speed, 180, 40);
    lv_obj_set_style_bg_color(btn_go_speed, lv_color_hex(0x8E44AD), 0); // Màu tím Radar
    lv_obj_t * lbl_go_speed = lv_label_create(btn_go_speed);
    lv_label_set_text(lbl_go_speed, "<- Radar Khoang cach");
    lv_obj_center(lbl_go_speed);
    lv_obj_add_event_cb(btn_go_speed, btn_go_speed_cb, LV_EVENT_ALL, NULL);

    // Nút Wi-Fi (Căn giữa)
    my_wifi_btn = lv_btn_create(scr_home);
    lv_obj_align(my_wifi_btn, LV_ALIGN_CENTER, 0, 0); 
    lv_obj_set_size(my_wifi_btn, 180, 40);
    my_wifi_label = lv_label_create(my_wifi_btn);
    lv_label_set_text(my_wifi_label, "Bat Wi-Fi");
    lv_obj_center(my_wifi_label);
    lv_obj_add_event_cb(my_wifi_btn, wifi_connect_event_handler, LV_EVENT_ALL, NULL);

    // Nút sang phải (Điều khiển đèn)
    lv_obj_t * btn_go_ctrl = lv_btn_create(scr_home);
    lv_obj_align(btn_go_ctrl, LV_ALIGN_CENTER, 0, 60); 
    lv_obj_set_size(btn_go_ctrl, 180, 40);
    lv_obj_set_style_bg_color(btn_go_ctrl, lv_color_hex(0x2980B9), 0); 
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
    // KHỞI TẠO SCREEN 3: TRANG RADAR TỐC ĐỘ (ĐÃ FIX ĐÈ CHỮ)
    // ==========================================
    scr_speed = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_speed, lv_color_hex(0x1E1E1E), 0); 

    // 1. Tiêu đề & LED Trạng thái (Căn ra giữa trên cùng - TOP_MID)
    // Gom nhóm để không bị nút bấm đè lên
    lv_obj_t * title_speed = lv_label_create(scr_speed);
    lv_label_set_text(title_speed, "SPEED RADAR");
    lv_obj_set_style_text_color(title_speed, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title_speed, LV_ALIGN_TOP_MID, -10, 10); // Lệch trái 10px để nhường chỗ cho LED

    ui_radar_led = lv_led_create(scr_speed);
    lv_obj_set_size(ui_radar_led, 12, 12);
    // Đặt LED ngay sau chữ "SPEED RADAR"
    lv_obj_align_to(ui_radar_led, title_speed, LV_ALIGN_OUT_RIGHT_MID, 10, 0); 
    lv_led_set_color(ui_radar_led, lv_color_hex(0x00FF00));
    lv_led_on(ui_radar_led);

    // 2. Nút Quay Về (Chuyển xuống dưới cùng - BOTTOM_MID)
    // Vừa giúp UI thoáng đãng, vừa giúp người dùng dễ thao tác một tay
    lv_obj_t * btn_back_speed = lv_btn_create(scr_speed);
    lv_obj_set_size(btn_back_speed, 100, 35);
    // Đặt ở đáy màn hình, cách mép dưới 45px (để không đè lên Dist/Max label)
    lv_obj_align(btn_back_speed, LV_ALIGN_BOTTOM_MID, 0, -45); 
    lv_obj_set_style_bg_color(btn_back_speed, lv_color_hex(0x7F8C8D), 0);
    lv_obj_t * lbl_back_spd = lv_label_create(btn_back_speed);
    lv_label_set_text(lbl_back_spd, "Home");
    lv_obj_center(lbl_back_spd);
    lv_obj_add_event_cb(btn_back_speed, btn_home_from_speed_cb, LV_EVENT_ALL, NULL);

    // 3. Tạo Đồng hồ kim (LV_METER) - Dời lên một chút để cân đối
    ui_meter = lv_meter_create(scr_speed);
    lv_obj_align(ui_meter, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_size(ui_meter, 170, 170); // Thu nhỏ 1 chút (từ 180 xuống 170) để thoáng màn hình

    // --- Giữ nguyên phần tạo Scale và Kim (Needle) như cũ ---
    lv_meter_scale_t * scale = lv_meter_add_scale(ui_meter);
    lv_meter_set_scale_ticks(ui_meter, scale, 51, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(ui_meter, scale, 10, 4, 15, lv_color_black(), 10);
    lv_meter_set_scale_range(ui_meter, scale, 0, 100, 270, 135);
    
    speed_needle = lv_meter_add_needle_line(ui_meter, scale, 4, lv_color_hex(0xE74C3C), -10);
    lv_meter_set_indicator_value(ui_meter, speed_needle, 0);

    // 4. Các Label thông số (Giữ nguyên nhưng kiểm tra lại Align)
    ui_speed_label = lv_label_create(scr_speed);
    lv_label_set_text(ui_speed_label, "0.0");
    lv_obj_set_style_text_color(ui_speed_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(ui_speed_label, LV_ALIGN_CENTER, 0, 45);

    ui_dist_label = lv_label_create(scr_speed);
    lv_label_set_text(ui_dist_label, "Dist: -- cm");
    lv_obj_set_style_text_font(ui_dist_label, &lv_font_montserrat_12, 0); // Dùng font nhỏ cho tinh tế
    lv_obj_align(ui_dist_label, LV_ALIGN_BOTTOM_LEFT, 10, -10);

    ui_max_speed_label = lv_label_create(scr_speed);
    lv_label_set_text(ui_max_speed_label, "Max: 0");
    lv_obj_set_style_text_font(ui_max_speed_label, &lv_font_montserrat_12, 0);
    lv_obj_align(ui_max_speed_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);



    // ==========================================
    // HIỂN THỊ MÀN HÌNH ĐẦU TIÊN
    // ==========================================

    // ---------------------------------------------------------
    // 4. THÊM VÒNG TRÒN CHỈNH ĐỘ SÁNG (Bị đẩy xuống để phải cuộn)
    // ---------------------------------------------------------
    lv_obj_t * ui_arc = lv_arc_create(scr_control);
    lv_obj_set_size(ui_arc, 150, 150); // Đường kính 150px
    lv_arc_set_range(ui_arc, 0, 100);  // Dải giá trị từ 0 đến 100
    lv_arc_set_value(ui_arc, 50);      // Bắt đầu ở mức 50%
    // Cố tình đẩy Y xuống 180 pixel. Do màn hình nhỏ, nó sẽ tràn xuống dưới
    // Tạo ra thanh cuộn (scrollbar) để bạn vuốt lên xem.
    lv_obj_align(ui_arc, LV_ALIGN_CENTER, 0, 180);
    // Gắn sự kiện khi bị thay đổi giá trị
    lv_obj_add_event_cb(ui_arc, arc_value_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // 5. Tạo chữ hiển thị phần trăm (đặt vòng tròn làm parent)
    ui_arc_label = lv_label_create(ui_arc);
    lv_label_set_text(ui_arc_label, "50%");

    // Hàm này sẽ tự động căn giữa chữ theo vòng tròn cha của nó
    lv_obj_center(ui_arc_label);

    // THÊM DÒNG NÀY ĐỂ ĐỔI FONT CHỮ TO HƠN (Ví dụ: Size 24)
    lv_obj_set_style_text_font(ui_arc_label, &lv_font_montserrat_24, 0);

    // ==========================================
    // KHỞI ĐỘNG TASK NGẦM SAU KHI UI ĐÃ TẠO XONG
    // ==========================================
    xTaskCreate(radar_task, "radar_task", 4096, NULL, 5, NULL);

    lv_scr_load(scr_home);
    
    // 3. Vòng lặp duy trì hệ thống
    while (1) {
        // Xin chìa khóa
        if (lvgl_port_lock(0)) {
            lv_timer_handler(); // Cho phép LVGL vẽ màn hình
            lvgl_port_unlock(); // Vẽ xong thì trả chìa khóa lại ngay
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}