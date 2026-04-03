#ifndef ESP_TOUCH_H
#define ESP_TOUCH_H

#include <stdint.h>
#include <stdbool.h>

// --- CẤU HÌNH CHÂN I2C (Thay đổi theo thiết kế mạch của bạn) ---
// *Lưu ý: Bạn hãy kiểm tra sơ đồ chân (Pinout) của màn hình ESP32-C6 để điền đúng số chân
#define TOUCH_I2C_SDA       18      // Điền chân SDA thực tế
#define TOUCH_I2C_SCL       8       // Điền chân SCL thực tế
#define TOUCH_I2C_NUM       0      // Sử dụng bộ I2C master số 0 của ESP32
#define TOUCH_I2C_FREQ      200000 // Tốc độ chuẩn 400kHz
#define TOUCH_I2C_ADDR      0x15   // Địa chỉ I2C của chip cảm ứng

// Hàm khởi tạo phần cứng I2C và đánh thức chip cảm ứng
void esp_touch_init(void);

// Hàm đọc tọa độ (Trả về true nếu có chạm, false nếu không)
bool esp_touch_read(uint16_t *x, uint16_t *y);

#endif // TOUCH_BSP_H