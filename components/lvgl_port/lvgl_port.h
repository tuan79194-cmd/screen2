#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include <stdbool.h>
#include <stdint.h>

// Hàm duy nhất được gọi từ main.c
void lvgl_port_init(void);

// ==========================================
// THÊM 2 HÀM KHÓA LUỒNG BẢO VỆ LVGL
// ==========================================
bool lvgl_port_lock(uint32_t timeout_ms);
void lvgl_port_unlock(void);

#endif // LVGL_PORT_H