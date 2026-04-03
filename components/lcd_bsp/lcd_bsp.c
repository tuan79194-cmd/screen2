#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_sh8601.h"
#include "lcd_bsp.h" // Nhúng header vừa tạo ở trên
#include "sh8601_cmds.h"

// --- ĐỊNH NGHĨA MÃ MÀU RGB565 (Đã lật Byte) ---
#define COLOR_RED       0x00F8
#define COLOR_GREEN     0xE007
#define COLOR_BLUE      0x1F00
#define COLOR_YELLOW    0xE0FF
#define COLOR_CYAN      0xFF07
#define COLOR_MAGENTA   0x1FF8
#define COLOR_ORANGE    0x20FD
#define COLOR_PINK      0x19FE
#define COLOR_WHITE     0xFFFF
#define COLOR_BLACK     0x0000

static const char *TAG = "LCD_BSP";

static void init_spi_bus(void) {
    ESP_LOGI(TAG, "1. Cấu hình phần cứng SPI Bus...");
    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_SCLK,
        .mosi_io_num = LCD_MOSI,
        .miso_io_num = -1,         
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 170 * 320 * 2 + 8
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
}

static esp_lcd_panel_io_handle_t init_panel_io(void) {
    ESP_LOGI(TAG, "2. Gắn kết màn hình vào SPI (Panel IO)...");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_DC,
        .cs_gpio_num = LCD_CS,
        .pclk_hz = 20 * 1000 * 1000, 
        .spi_mode = 0,               
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,           
        .lcd_param_bits = 8,         
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
    return io_handle;
}

static esp_lcd_panel_handle_t init_sh8601_driver(esp_lcd_panel_io_handle_t io_handle) {
    ESP_LOGI(TAG, "3. Cài đặt Driver SH8601...");
    sh8601_vendor_config_t vendor_config = {
        .init_cmds = lcd_init_cmds, 
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]), 
    };

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB, 
        .bits_per_pixel = 16,             
        .vendor_config = &vendor_config,            
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,     
    };
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle)); 
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle)); 

    return panel_handle;
}

esp_lcd_panel_handle_t lcd_bsp_init(void)
{
    init_spi_bus();
    esp_lcd_panel_io_handle_t io_handle = init_panel_io();
    esp_lcd_panel_handle_t panel_handle = init_sh8601_driver(io_handle);

    ESP_LOGI(TAG, "4. Đổ màu để xóa sọc đen...");
    // Sửa lỗi lệch khung hình và bật hiển thị
    esp_lcd_panel_set_gap(panel_handle, 35, 0);
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    uint16_t *color_data = (uint16_t *)heap_caps_malloc(170 * 320 * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (color_data != NULL) {
        for (int i = 0; i < 170 * 320; i++) {
            color_data[i] = COLOR_CYAN; // Màu xanh lá
        }
        esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 170, 320, color_data);
        free(color_data);
    }

    // Trả về handle để dùng cho LVGL sau này
    return panel_handle;
}