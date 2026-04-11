#include "led_bsp.h"
#include "driver/gpio.h"

// Định nghĩa chân cắm (Đổi số 22 thành chân khác nếu bạn cắm sai)
#define LED_PIN GPIO_NUM_22 

void led_bsp_init(void) {
    // 1. Reset chân về trạng thái mặc định cho an toàn
    gpio_reset_pin(LED_PIN);
    
    // 2. Cấu hình chân này thành Cổng Xuất Tín Hiệu (OUTPUT)
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    // 3. Ép mức 0 (Tắt đèn) ngay khi vừa khởi động
    gpio_set_level(LED_PIN, 0); 
}

void led_bsp_set_state(bool on) {
    // Nếu 'on' là true -> xuất mức 1. Nếu false -> xuất mức 0.
    if (on) {
        gpio_set_level(LED_PIN, 1);
    } else {
        gpio_set_level(LED_PIN, 0);
    }
}