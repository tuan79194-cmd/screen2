#ifndef LCD_BSP_H
#define LCD_BSP_H

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

// --- CẤU HÌNH CHÂN GPIO ---
#define LCD_HOST    SPI2_HOST 
#define LCD_MOSI    4   
#define LCD_SCLK    5   
#define LCD_CS      7   
#define LCD_DC      6   
#define LCD_RST     14  

// Khai báo hàm khởi tạo màn hình. 
// Lưu ý: Hàm này sẽ trả về panel_handle để sau này cấu hình cho LVGL.
esp_lcd_panel_handle_t lcd_bsp_init(void);

#endif // LCD_BSP_H