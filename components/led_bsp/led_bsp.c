#include "led_bsp.h"
#include "driver/ledc.h" // THAY GPIO BẰNG LEDC

// Định nghĩa chân cắm (Đổi số 22 thành chân khác nếu bạn cắm sai)
#define LED_PIN GPIO_NUM_22 

// --- CẤU HÌNH THÔNG SỐ PWM ---
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          LED_PIN // Chân số 22
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT // Độ phân giải 8-bit (0-255)
#define LEDC_FREQUENCY          5000 // Tần số 5000 Hz

// Biến nội bộ để nhớ trạng thái hiện tại
static bool is_led_on = false;
static uint8_t current_brightness = 50; // Mặc định khi bật lên là sáng 50%

// Hàm phụ: Tính toán và áp dụng điện áp thực tế ra chân cắm
static void update_hardware(void) {
    if (!is_led_on) {
        // ĐÈN TẮT -> CHIP XUẤT 0V (Mức 0)
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    } else {
        // ĐÈN BẬT -> CHIP XUẤT PWM (0-255)
        uint32_t duty = (current_brightness * 255) / 100;
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    }
    // Cập nhật cấu hình ngay lập tức
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

void led_bsp_init(void) {
// 1. Cấu hình Timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // 2. Cấu hình Channel gắn với chân 22
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Ép mức 0 (Tắt) ngay khi khởi động
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
    
    is_led_on = false;
}

void led_bsp_set_state(bool on) {
    is_led_on = on;
    update_hardware();
}

void led_bsp_set_brightness(uint8_t percent) {
    if (percent > 100) percent = 100; // Bảo vệ: Không cho vượt quá 100
    current_brightness = percent;
    
    // Nếu đèn đang trạng thái BẬT, lập tức đổi độ sáng thực tế
    if (is_led_on) {
        update_hardware();
    }
}