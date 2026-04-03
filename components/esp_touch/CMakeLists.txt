# Khai báo file nguồn, file header và thư viện cần thiết (driver chứa I2C)
idf_component_register(SRCS "touch_bsp.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver freertos)