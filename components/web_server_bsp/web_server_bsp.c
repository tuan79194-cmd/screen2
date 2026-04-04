#include <stdio.h>
#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "WEB_SERVER";

// =========================================================
// 1. GIAO DIỆN WEBSITE (HTML + CSS + JS)
// =========================================================
// Lấy địa chỉ của file index.html đã được nhúng trong bộ nhớ Flash
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

// =========================================================
// 2. CÁC TUYẾN ĐƯỜNG (ROUTES) XỬ LÝ DỮ LIỆU
// =========================================================

// Tuyến 1: Khi trình duyệt truy cập địa chỉ IP gốc (GET /)
static esp_err_t index_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Co nguoi vua truy cap vao Website!");
    httpd_resp_set_type(req, "text/html");
    // Tính toán dung lượng của file HTML
    size_t html_len = index_html_end - index_html_start;
    
    // Gửi toàn bộ file lên trình duyệt
    httpd_resp_send(req, (const char *)index_html_start, html_len);
    return ESP_OK;
}

// Thêm khai báo mượn hàm từ lvgl_port.c
extern void* get_lvgl_buf(void);

// Tuyến 2: Khi Website yêu cầu tải ảnh (GET /screenshot.bmp)
static esp_err_t screenshot_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Website dang yeu cau chup anh!");
    
    // 1. Khai báo cho trình duyệt biết: "Đây là file ảnh BMP"
    httpd_resp_set_type(req, "image/bmp");

    // 2. Mảng 66 Bytes tiêu đề ảnh cho màn hình 170x320, RGB565, quét từ trên xuống (-320)
    const uint8_t bmp_header[66] = {
        'B', 'M',             // Chữ ký BMP
        0x42, 0xA9, 0x01, 0x00, // Tổng dung lượng file: 108,866 bytes
        0x00, 0x00, 0x00, 0x00, // Reserved
        0x42, 0x00, 0x00, 0x00, // Điểm bắt đầu dữ liệu ảnh (Byte thứ 66)

        0x28, 0x00, 0x00, 0x00, // DIB Header size (40 bytes)
        0xAA, 0x00, 0x00, 0x00, // Chiều rộng: 170 (0xAA)
        0xC0, 0xFE, 0xFF, 0xFF, // Chiều cao: -320 (0xFFFFFEC0 - Quét ngược từ trên xuống)
        0x01, 0x00,             // Số mặt phẳng (Planes: 1)
        0x10, 0x00,             // Số bit màu: 16-bit
        0x03, 0x00, 0x00, 0x00, // Chế độ màu RGB565 (BI_BITFIELDS)
        0x00, 0xA9, 0x01, 0x00, // Dung lượng điểm ảnh: 108,800 bytes
        0x00, 0x00, 0x00, 0x00, // X Pixels per meter
        0x00, 0x00, 0x00, 0x00, // Y Pixels per meter
        0x00, 0x00, 0x00, 0x00, // Colors used
        0x00, 0x00, 0x00, 0x00, // Important colors

        // Mặt nạ dịch bit (Bitmasks cho Đỏ, Xanh Lá, Xanh Dương)
        0x00, 0xF8, 0x00, 0x00, // Red
        0xE0, 0x07, 0x00, 0x00, // Green
        0x1F, 0x00, 0x00, 0x00  // Blue
    };

    // Lấy dữ liệu ảnh thẳng từ nhân LVGL
    uint8_t* frame_buffer = (uint8_t*)get_lvgl_buf();

if (frame_buffer != NULL) {
        // 1. Gửi 66 Bytes Header trước
        httpd_resp_send_chunk(req, (const char*)bmp_header, 66);
        
        // 2. BẮT ĐẦU VÒNG LẶP ĐẢO BYTE VÀ GỬI
        uint8_t chunk_buf[1024]; // Tạo cái xô 1KB
        int total_bytes = 170 * 320 * 2;
        int offset = 0;

        while (offset < total_bytes) {
            // Tính số lượng byte còn lại cần xử lý
            int send_size = (total_bytes - offset > sizeof(chunk_buf)) ? sizeof(chunk_buf) : (total_bytes - offset);
            
            // Đảo ngược 2 byte của từng Pixel cho vừa mắt Trình duyệt
            for (int i = 0; i < send_size; i += 2) {
                chunk_buf[i] = frame_buffer[offset + i + 1];     // Đưa byte cao xuống thấp
                chunk_buf[i + 1] = frame_buffer[offset + i];     // Đưa byte thấp lên cao
            }

            // Gửi cái xô này đi
            httpd_resp_send_chunk(req, (const char*)chunk_buf, send_size);
            
            offset += send_size;
        }
        
        // 3. Báo hiệu đã gửi xong toàn bộ file
        httpd_resp_send_chunk(req, NULL, 0);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Buffer ảnh trống!");
    }
    
    return ESP_OK;
}
// =========================================================
// 3. KHỞI TẠO WEB SERVER
// =========================================================
void web_server_start(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_LOGI(TAG, "Dang khoi dong Web Server tren Port %d...", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        // Đăng ký Tuyến 1
        httpd_uri_t uri_index = { .uri = "/", .method = HTTP_GET, .handler = index_get_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &uri_index);

        // Đăng ký Tuyến 2
        httpd_uri_t uri_screen = { .uri = "/screenshot.bmp", .method = HTTP_GET, .handler = screenshot_get_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &uri_screen);

        ESP_LOGI(TAG, "Web Server da san sang!");
    } else {
        ESP_LOGE(TAG, "Khong the khoi dong Web Server!");
    }
}