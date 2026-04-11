#ifndef LED_BSP_H
#define LED_BSP_H

#include <stdbool.h>
#include <stdint.h>

// Hàm khởi tạo chân cắm LED
void led_bsp_init(void);

// Hàm điều khiển bật/tắt LED (true = Bật, false = Tắt)
void led_bsp_set_state(bool on);

// HÀM MỚI: Nhận giá trị từ 0 đến 100 để đổi độ sáng
void led_bsp_set_brightness(uint8_t percent);

#endif // LED_BSP_H