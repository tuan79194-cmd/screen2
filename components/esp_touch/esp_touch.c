#include "esp_touch.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "TOUCH_BSP";

void esp_touch_init(void)
{
    ESP_LOGI(TAG, "1. Cấu hình phần cứng I2C cho Cảm ứng...");
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = TOUCH_I2C_SDA,
        .scl_io_num = TOUCH_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = TOUCH_I2C_FREQ,
    };
    
    // Nạp cấu hình và kích hoạt Driver I2C
    ESP_ERROR_CHECK(i2c_param_config(TOUCH_I2C_NUM, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(TOUCH_I2C_NUM, i2c_conf.mode, 0, 0, 0));

    ESP_LOGI(TAG, "2. Đánh thức chip cảm ứng CST816T...");
    // Mảng chứa: [Địa chỉ thanh ghi là 0x00] và [Dữ liệu cần nạp là 0x00]
    uint8_t init_data[2] = {0x00, 0x00}; 
    
    // Đẩy lệnh qua đường I2C (Timeout 1000ms)
    esp_err_t err = i2c_master_write_to_device(TOUCH_I2C_NUM, TOUCH_I2C_ADDR, init_data, 2, 1000 / portTICK_PERIOD_MS);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "=> Khởi tạo chip cảm ứng thành công!");
    } else {
        ESP_LOGE(TAG, "=> Lỗi: Không tìm thấy chip cảm ứng (Kiểm tra lại dây SDA/SCL). Mã lỗi: %s", esp_err_to_name(err));
    }
}

bool esp_touch_read(uint16_t *x, uint16_t *y)
{
    uint8_t reg_addr = 0x00; // Địa chỉ thanh ghi bắt đầu đọc
    uint8_t tp_temp[7];      // Mảng chứa 7 byte dữ liệu trả về từ chip
    
    // Đọc 1 lúc 7 byte từ chip cảm ứng về mảng tp_temp
    esp_err_t err = i2c_master_write_read_device(TOUCH_I2C_NUM, TOUCH_I2C_ADDR, 
                                                 &reg_addr, 1, 
                                                 tp_temp, 7, 
                                                 1000 / portTICK_PERIOD_MS);
    
    if (err == ESP_OK) {
        uint8_t touch_points = tp_temp[2]; // Byte số 2 chứa lượng điểm đang chạm
        
        if (touch_points > 0) {
            // Phép toán bitwise (|) ghép nửa trên và nửa dưới của tọa độ 
            *x = ((uint16_t)(tp_temp[3] & 0x0F) << 8) | tp_temp[4];
            *y = ((uint16_t)(tp_temp[5] & 0x0F) << 8) | tp_temp[6];
            return true; // Báo hiệu đã đọc được tọa độ
        }
    }
    
    return false; // Không có chạm hoặc đường truyền bị nhiễu
}