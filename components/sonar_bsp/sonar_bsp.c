#include "sonar_bsp.h"
#include "driver/gpio.h"
#include "esp_timer.h" // Thư viện bấm giờ độ phân giải micro-giây
#include "esp_rom_sys.h" // Thư viện tạo trễ micro-giây

// Cấu hình chân như đã chốt
#define TRIG_PIN GPIO_NUM_20
#define ECHO_PIN GPIO_NUM_1

// Thời gian chờ tối đa (Timeout) - Khoảng 30000 us (30ms) 
// Tương đương tầm sóng bay đi bay về khoảng 5 mét. Quá thời gian này coi như lỗi/không có vật cản.
#define TIMEOUT_US 30000 

void sonar_bsp_init(void) {
    // 1. Cấu hình chân TRIG làm Đầu Ra (Output)
    gpio_reset_pin(TRIG_PIN);
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TRIG_PIN, 0); // Kéo xuống mức 0 để chờ

    // 2. Cấu hình chân ECHO làm Đầu Vào (Input)
    gpio_reset_pin(ECHO_PIN);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);
}

float sonar_bsp_read_distance_cm(void) {
    // BƯỚC 1: BẮN SÓNG (Tạo xung High 10 micro-giây ở chân Trig)
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10); 
    gpio_set_level(TRIG_PIN, 0);

    // BƯỚC 2: CHỜ CHÂN ECHO LÊN MỨC 1 (Cảm biến bắt đầu phát sóng)
    int64_t wait_start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0) {
        if (esp_timer_get_time() - wait_start > TIMEOUT_US) {
            return -1.0; // Báo lỗi Timeout nếu chờ quá lâu
        }
    }

    // BƯỚC 3: BẤM GIỜ (Bắt đầu tính thời gian chân Echo giữ mức 1)
    int64_t echo_start = esp_timer_get_time();
    
    // BƯỚC 4: CHỜ CHÂN ECHO XUỐNG MỨC 0 (Sóng đã dội về)
    while (gpio_get_level(ECHO_PIN) == 1) {
        if (esp_timer_get_time() - echo_start > TIMEOUT_US) {
            return -1.0; // Vượt quá tầm đo
        }
    }
    int64_t echo_end = esp_timer_get_time();

    // BƯỚC 5: TÍNH TOÁN KHOẢNG CÁCH
    // Thời gian bay (micro-giây)
    int64_t time_us = echo_end - echo_start; 
    
    // Vận tốc âm thanh: 0.0343 cm/us. Chia 2 vì sóng đi cả 2 chiều
    float distance_cm = (time_us * 0.0343) / 2.0;

    return distance_cm;
}