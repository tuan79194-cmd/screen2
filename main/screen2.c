#include "touch_bsp.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"          // Thư viện LVGL
#include "lcd_bsp.h"       // Module điều khiển hiển thị SPI của bạn
#include "touch_bsp.h"     // Module điều khiển cảm ứng I2C của bạn
#include "esp_timer.h"

// Chỉ cần gọi header này là đủ bộ công cụ màn hình
#include "lcd_bsp.h" 

static const char *TAG = "MAIN_APP";

// =========================================================================
// PHẦN 1: KEO DÁN HIỂN THỊ (LVGL -> SPI)
// =========================================================================
static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    // Lấy panel_handle từ user_data
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) disp_drv->user_data;
    
    // Đẩy mảng màu của LVGL ra màn hình thật qua SPI
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    
    // Báo cho LVGL biết là đã gửi xong
    lv_disp_flush_ready(disp_drv);
}

// =========================================================================
// PHẦN 2: KEO DÁN CẢM ỨNG (I2C -> LVGL)
// =========================================================================
static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    uint16_t touch_x = 0;
    uint16_t touch_y = 0;
    
    // Gọi hàm đọc I2C của bạn
    bool is_pressed = esp_touch_read(&touch_x, &touch_y);

    if (is_pressed) {
        data->state = LV_INDEV_STATE_PR; // Trạng thái: Đang ấn
        data->point.x = touch_x;         // Nạp tọa độ X cho LVGL
        data->point.y = touch_y;         // Nạp tọa độ Y cho LVGL
    } else {
        data->state = LV_INDEV_STATE_REL; // Trạng thái: Thả tay
    }
}

// =========================================================================
// PHẦN 3: NHỊP TIM CỦA LVGL
// =========================================================================
static void lv_tick_task(void *arg)
{
    // Báo cho LVGL biết thời gian trôi qua mỗi 2 mili-giây
    lv_tick_inc(2); 
}

// =========================================================================
// HÀM CHÍNH CỦA HỆ THỐNG
// =========================================================================
void app_main(void)
{
    // 1. Khởi tạo Phần Cứng (Đổ móng nhà)
    ESP_LOGI(TAG, "Khởi tạo màn hình và cảm ứng...");
    esp_lcd_panel_handle_t panel_handle = lcd_bsp_init();
    esp_touch_init();

    // 2. Khởi tạo Lõi LVGL
    lv_init();

    // 3. Cấp phát RAM làm Bộ Đệm Vẽ (Draw Buffer)
    // Cấp phát 20 dòng màn hình trong vùng nhớ DMA để truyền SPI siêu tốc
        size_t draw_buffer_sz = 170 * 320;
    lv_color_t *buf1 = heap_caps_malloc(draw_buffer_sz * sizeof(lv_color_t), MALLOC_CAP_DMA);
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf1, NULL, draw_buffer_sz);

    // 4. Đăng ký Cấu hình Hiển Thị cho LVGL
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 170;             // Chiều rộng màn hình
    disp_drv.ver_res = 320;             // Chiều dài màn hình
    disp_drv.flush_cb = disp_flush;     // Dán hàm hiển thị
    disp_drv.draw_buf = &disp_buf;      
    disp_drv.user_data = panel_handle;  // Truyền handle phần cứng vào
    lv_disp_drv_register(&disp_drv);

    // 5. Đăng ký Cấu hình Cảm Ứng cho LVGL
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;  // Dán hàm cảm ứng
    lv_indev_drv_register(&indev_drv);

    // 6. Khởi động Nhịp tim bằng Timer của ESP32
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "lv_tick"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 2 * 1000)); // Gọi mỗi 2000 micro-giây (2ms)

    // ====================================================
    // VẼ THỬ GIAO DIỆN (UI)
    // ====================================================
    // Tạo 1 nút bấm giữa màn hình
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(btn, 100, 50);

    // Thêm chữ "CLICK ME" lên nút bấm
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "CLICK ME");
    lv_obj_center(label);

    ESP_LOGI(TAG, "Hoàn tất Porting LVGL. Bắt đầu vòng lặp chính!");

    // 7. Vòng lặp duy trì LVGL
    while (1) {
        // Hàm này xử lý hiệu ứng đồ họa, nhấn nút...
        lv_timer_handler(); 
        vTaskDelay(pdMS_TO_TICKS(10)); // Cho CPU nghỉ 10ms để làm việc khác
    }
}