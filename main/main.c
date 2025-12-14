#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "lora_mesh.h"
#include "web_server.h"
#include "config_manager.h"
#include "encryption.h"
#include "mesh_protocol.h"

static const char *TAG = "MAIN";

// Global configuration
static lora_config_t g_config;

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "LoRa Mesh Network Starting...");
    ESP_LOGI(TAG, "========================================");

    // Initialize NVS for configuration storage
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    //debug_nvs_content();
    // Initialize network stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Load configuration from NVS
    config_init();
    if (!config_load(&g_config)) {
        ESP_LOGW(TAG, "No saved config found, using defaults");
        config_load_defaults(&g_config);
        config_save(&g_config);
        // DEBUG: Print NVS after saving defaults
        //debug_nvs_content();
    }
    config_print(&g_config);

    // Initialize encryption if enabled
    if (g_config.enable_encryption) {
        encryption_init(g_config.aes_key, g_config.iv);
    }

    // Initialize LoRa hardware with loaded config
    if (!lora_mesh_init(&g_config)) {
        ESP_LOGE(TAG, "Failed to initialize LoRa mesh");
        return;
    }

    // Initialize mesh protocol
    mesh_protocol_init(&g_config);

    // Start WiFi Access Point
    wifi_init_softap(&g_config);

    // FIX: Always enable web server regardless of config
    ESP_LOGI(TAG, "=== FORCE ENABLING WEB SERVER ===");
    g_config.enable_web_server = true;
    g_config.web_port = 80;
    
    // Save to NVS so it persists
    config_save(&g_config);
    ESP_LOGI(TAG, "Web server forced enabled and saved to NVS");
    
    // Print config to verify
    ESP_LOGI(TAG, "enable_web_server: %s", g_config.enable_web_server ? "true" : "false");
    ESP_LOGI(TAG, "web_port: %d", g_config.web_port);
    
    // Start web server with embedded HTML
    web_server_start();
   
    // Start LoRa mesh task
    xTaskCreate(lora_mesh_task, "mesh_task", 8192, &g_config, 5, NULL);

    // Start mesh maintenance task
    xTaskCreate(mesh_maintenance_task, "mesh_maintenance", 4096, &g_config, 4, NULL);

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "System Started Successfully");
    ESP_LOGI(TAG, "Node ID: %d (%s)", g_config.node_id, 
             g_config.node_id == 1 ? "MASTER" : "SLAVE");
    ESP_LOGI(TAG, "Web UI: http://192.168.4.1");
    ESP_LOGI(TAG, "WiFi SSID: %s", g_config.wifi_ssid);
    ESP_LOGI(TAG, "Frequency: %.1f MHz", g_config.frequency / 1000000.0);
    ESP_LOGI(TAG, "Encryption: %s", g_config.enable_encryption ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "========================================");

    // Main idle loop
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}