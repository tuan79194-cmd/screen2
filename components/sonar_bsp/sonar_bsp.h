#ifndef SONAR_BSP_H
#define SONAR_BSP_H

#include <stdint.h>

// Hàm khởi tạo các chân GPIO cho cảm biến
void sonar_bsp_init(void);

// Hàm kích hoạt đo và trả về khoảng cách (đơn vị: cm)
// Trả về -1.0 nếu bị lỗi (quá thời gian chờ / không thấy vật cản)
float sonar_bsp_read_distance_cm(void);

#endif // SONAR_BSP_H