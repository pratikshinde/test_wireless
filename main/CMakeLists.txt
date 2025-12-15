idf_component_register(
    SRCS "main.c" "lora_mesh.c" "web_server.c" "config_manager.c" 
          "mesh_protocol.c" "rfm95w.c" "encryption.c" "mesh_manager.c"
    PRIV_REQUIRES spi_flash
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_http_server driver mbedtls esp_timer
    EMBED_FILES 
        "web/dashboard.html"
        "web/lora_config.html"    # Must match C declaration: lora_config (underscore)
        "web/devices.html"
        "web/styles.css"
        "web/script.js"
)