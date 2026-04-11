#ifndef LED_BSP_H
#define LED_BSP_H

#include <stdbool.h>

// Hàm khởi tạo chân cắm LED
void led_bsp_init(void);

// Hàm điều khiển bật/tắt LED (true = Bật, false = Tắt)
void led_bsp_set_state(bool on);

#endif // LED_BSP_H