#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "lvgl.h"

// Các file Driver phần cứng của bạn
#include "lcd_bsp.h"   
#include "esp_touch.h" 

#include "lvgl_port.h"

static const char *TAG = "LVGL_PORT";

// =========================================================================
// CÁC HÀM CALLBACK (CHỈ DÙNG NỘI BỘ TRONG FILE NÀY NÊN CÓ CHỮ "STATIC")
// =========================================================================

static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) disp_drv->user_data;
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(disp_drv);
}

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    uint16_t touch_x = 0;
    uint16_t touch_y = 0;
    
    // Sử dụng hàm getTouch() từ touch_bsp.c của bạn
    bool is_pressed = esp_touch_read(&touch_x, &touch_y);

    if (is_pressed) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch_x;
        data->point.y = touch_y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

static void lv_tick_task(void *arg)
{
    lv_tick_inc(2); 
}

// =========================================================================
// HÀM KHỞI TẠO CHÍNH (ĐƯỢC GỌI TỪ MAIN.C)
// =========================================================================

void lvgl_port_init(void)
{
    ESP_LOGI(TAG, "Bắt đầu khởi tạo hệ thống đồ họa và phần cứng...");

    // 1. Khởi tạo toàn bộ phần cứng (Màn hình + I2C + Cảm ứng)
    esp_lcd_panel_handle_t panel_handle = lcd_bsp_init();
    esp_touch_init();
    // 2. Khởi tạo Lõi LVGL
    lv_init();

    // 3. Cấp phát Buffer Full Màn Hình (Chống xé hình tối đa)
    size_t draw_buffer_sz = 170 * 320; 
    lv_color_t *buf1 = heap_caps_malloc(draw_buffer_sz * sizeof(lv_color_t), MALLOC_CAP_DMA);
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf1, NULL, draw_buffer_sz);

    // 4. Đăng ký Cấu hình Hiển Thị cho LVGL
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 170;
    disp_drv.ver_res = 320;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &disp_buf;      
    disp_drv.user_data = panel_handle;
    lv_disp_drv_register(&disp_drv);

    // 5. Đăng ký Cấu hình Cảm Ứng cho LVGL
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    // 6. Khởi động Timer tạo nhịp tim 2ms
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "lv_tick"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 2 * 1000));

    ESP_LOGI(TAG, "Porting LVGL thành công!");
}