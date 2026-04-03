# Khai báo file nguồn, file header và thư viện cần thiết (driver chứa I2C)
idf_component_register(SRCS "esp_touch.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver freertos)