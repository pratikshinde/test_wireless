idf_component_register(SRCS "main.c" "lora_mesh.c" "web_server.c" "config_manager.c" "mesh_protocol.c" "rfm95w.c" "encryption.c"
                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS "."
                       REQUIRES nvs_flash esp_wifi esp_http_server driver mbedtls esp_timer)

target_add_binary_data(${COMPONENT_TARGET} "web/dashboard.html" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "web/lora_config.html" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "web/devices.html" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "web/styles.css" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "web/script.js" TEXT)
