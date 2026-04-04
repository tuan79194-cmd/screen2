idf_component_register(SRCS "web_server_bsp.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_http_server log)