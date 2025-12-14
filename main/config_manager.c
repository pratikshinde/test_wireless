// config_manager.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "config_manager.h"
#include "nvs_keys.h"

static const char *TAG = "CONFIG";

// Default configuration
static lora_config_t default_config = {
    .node_id = 1,
    .node_name = "LoRa-Node",
    .frequency = 868000000,     // 868 MHz
    .spread_factor = 7,         // SF7 default
    .bandwidth = 125000,        // 125 kHz
    .coding_rate = 5,           // 4/5
    .tx_power = 17,             // dBm
    .sync_word = 0x12,
    .ping_interval = 30,        // seconds
    .beacon_interval = 60,      // seconds
    .route_timeout = 300,       // seconds
    .max_hops = 5,
    .enable_ack = true,
    .ack_timeout = 1000,        // ms
    .enable_self_healing = false,
    .healing_timeout = 30,      // seconds
    .wifi_ssid = "LoRa-Mesh",
    .wifi_password = "lora1234",
    .enable_web_server = false,
    .web_port = 80,
    .enable_encryption = false,
    .enable_crc = true,
    .enable_ldro = true,
    .preamble_length = 8,
    .symbol_timeout = 5
};

// Default AES key and IV (dummy values - should be changed in production)
static const uint8_t default_aes_key[16] = {
    0xA9, 0x6E, 0xD7, 0x0C, 0x1F, 0x2A, 0x8B, 0x3D,
    0x4E, 0x9F, 0x50, 0xC1, 0x72, 0xB3, 0x7C, 0xFC
};

static const uint8_t default_iv[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
};

// Forward declarations for static functions
static esp_err_t load_config_from_nvs(lora_config_t *config);
static esp_err_t save_config_to_nvs(const lora_config_t *config);

// Initialize configuration system
esp_err_t config_init(void) {
    ESP_LOGI(TAG, "Initializing configuration system");
    
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_LOGW(TAG, "NVS partition invalid, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    // Check if configuration exists, if not save defaults
    lora_config_t config;
    err = load_config_from_nvs(&config);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "No configuration found in NVS, saving defaults");
        memcpy(default_config.aes_key, default_aes_key, sizeof(default_config.aes_key));
        memcpy(default_config.iv, default_iv, sizeof(default_config.iv));
        err = save_config_to_nvs(&default_config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to save default configuration: %s", esp_err_to_name(err));
            return err;
        }
        ESP_LOGI(TAG, "Default configuration saved");
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load configuration: %s", esp_err_to_name(err));
        return err;
    } else {
        ESP_LOGI(TAG, "Configuration loaded successfully");
    }
    
    return ESP_OK;
}

// Save configuration to NVS
static esp_err_t save_config_to_nvs(const lora_config_t *config) {
    nvs_handle_t handle;
    esp_err_t err;
    
    ESP_LOGD(TAG, "Saving configuration to NVS");
    
    // Open NVS
    err = nvs_open("lora_config", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    // Save all configuration fields
    err = nvs_set_u8(handle, KEY_NODE_ID, config->node_id);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_str(handle, KEY_NODE_NAME, config->node_name);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u32(handle, KEY_FREQUENCY, config->frequency);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u8(handle, KEY_SPREAD_FACTOR, config->spread_factor);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u32(handle, KEY_BANDWIDTH, config->bandwidth);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u8(handle, KEY_CODING_RATE, config->coding_rate);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_i8(handle, KEY_TX_POWER, config->tx_power);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u8(handle, KEY_SYNC_WORD, config->sync_word);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u16(handle, KEY_PING_INTERVAL, config->ping_interval);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u16(handle, KEY_BEACON_INTERVAL, config->beacon_interval);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u16(handle, KEY_ROUTE_TIMEOUT, config->route_timeout);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u8(handle, KEY_MAX_HOPS, config->max_hops);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u8(handle, KEY_ENABLE_ACK, config->enable_ack);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u16(handle, KEY_ACK_TIMEOUT, config->ack_timeout);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u8(handle, KEY_ENABLE_HEALING, config->enable_self_healing);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u16(handle, KEY_HEALING_TIMEOUT, config->healing_timeout);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_str(handle, KEY_WIFI_SSID, config->wifi_ssid);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_str(handle, KEY_WIFI_PASSWORD, config->wifi_password);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u8(handle, KEY_ENABLE_WEB, config->enable_web_server);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u16(handle, KEY_WEB_PORT, config->web_port);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u8(handle, KEY_ENABLE_ENC, config->enable_encryption);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_blob(handle, KEY_AES_KEY, config->aes_key, sizeof(config->aes_key));
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_blob(handle, KEY_IV, config->iv, sizeof(config->iv));
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u8(handle, KEY_ENABLE_CRC, config->enable_crc);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u8(handle, KEY_ENABLE_LDRO, config->enable_ldro);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u16(handle, KEY_PREAMBLE_LEN, config->preamble_length);
    if (err != ESP_OK) goto cleanup;
    
    err = nvs_set_u16(handle, KEY_SYMBOL_TIMEOUT, config->symbol_timeout);
    if (err != ESP_OK) goto cleanup;
    
    // Commit changes
    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        goto cleanup;
    }
    
    ESP_LOGI(TAG, "Configuration saved to NVS successfully");
    
cleanup:
    nvs_close(handle);
    return err;
}

// Load configuration from NVS
static esp_err_t load_config_from_nvs(lora_config_t *config) {
    nvs_handle_t handle;
    esp_err_t err;
    size_t required_size;
    
    ESP_LOGD(TAG, "Loading configuration from NVS");
    
    // Open NVS
    err = nvs_open("lora_config", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    // Load all configuration fields
    err = nvs_get_u8(handle, KEY_NODE_ID, &config->node_id);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->node_id = default_config.node_id;
    else if (err != ESP_OK) goto cleanup;
    
    required_size = sizeof(config->node_name);
    err = nvs_get_str(handle, KEY_NODE_NAME, config->node_name, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) strcpy(config->node_name, default_config.node_name);
    else if (err != ESP_OK) goto cleanup;
    
    err = nvs_get_u32(handle, KEY_FREQUENCY, &config->frequency);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->frequency = default_config.frequency;
    else if (err != ESP_OK) goto cleanup;
    
    err = nvs_get_u8(handle, KEY_SPREAD_FACTOR, &config->spread_factor);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->spread_factor = default_config.spread_factor;
    else if (err != ESP_OK) goto cleanup;
    
    err = nvs_get_u32(handle, KEY_BANDWIDTH, &config->bandwidth);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->bandwidth = default_config.bandwidth;
    else if (err != ESP_OK) goto cleanup;
    
    err = nvs_get_u8(handle, KEY_CODING_RATE, &config->coding_rate);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->coding_rate = default_config.coding_rate;
    else if (err != ESP_OK) goto cleanup;
    
    err = nvs_get_i8(handle, KEY_TX_POWER, &config->tx_power);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->tx_power = default_config.tx_power;
    else if (err != ESP_OK) goto cleanup;
    
    err = nvs_get_u8(handle, KEY_SYNC_WORD, &config->sync_word);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->sync_word = default_config.sync_word;
    else if (err != ESP_OK) goto cleanup;
    
    err = nvs_get_u16(handle, KEY_PING_INTERVAL, &config->ping_interval);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->ping_interval = default_config.ping_interval;
    else if (err != ESP_OK) goto cleanup;
    
    err = nvs_get_u16(handle, KEY_BEACON_INTERVAL, &config->beacon_interval);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->beacon_interval = default_config.beacon_interval;
    else if (err != ESP_OK) goto cleanup;
    
    err = nvs_get_u16(handle, KEY_ROUTE_TIMEOUT, &config->route_timeout);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->route_timeout = default_config.route_timeout;
    else if (err != ESP_OK) goto cleanup;
    
    err = nvs_get_u8(handle, KEY_MAX_HOPS, &config->max_hops);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->max_hops = default_config.max_hops;
    else if (err != ESP_OK) goto cleanup;
    
    uint8_t temp_bool;
    err = nvs_get_u8(handle, KEY_ENABLE_ACK, &temp_bool);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->enable_ack = default_config.enable_ack;
    else if (err != ESP_OK) goto cleanup;
    else config->enable_ack = (bool)temp_bool;
    
    err = nvs_get_u16(handle, KEY_ACK_TIMEOUT, &config->ack_timeout);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->ack_timeout = default_config.ack_timeout;
    else if (err != ESP_OK) goto cleanup;
    
    err = nvs_get_u8(handle, KEY_ENABLE_HEALING, &temp_bool);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->enable_self_healing = default_config.enable_self_healing;
    else if (err != ESP_OK) goto cleanup;
    else config->enable_self_healing = (bool)temp_bool;
    
    err = nvs_get_u16(handle, KEY_HEALING_TIMEOUT, &config->healing_timeout);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->healing_timeout = default_config.healing_timeout;
    else if (err != ESP_OK) goto cleanup;
    
    required_size = sizeof(config->wifi_ssid);
    err = nvs_get_str(handle, KEY_WIFI_SSID, config->wifi_ssid, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) strcpy(config->wifi_ssid, default_config.wifi_ssid);
    else if (err != ESP_OK) goto cleanup;
    
    required_size = sizeof(config->wifi_password);
    err = nvs_get_str(handle, KEY_WIFI_PASSWORD, config->wifi_password, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) strcpy(config->wifi_password, default_config.wifi_password);
    else if (err != ESP_OK) goto cleanup;
    
    err = nvs_get_u8(handle, KEY_ENABLE_WEB, &temp_bool);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->enable_web_server = default_config.enable_web_server;
    else if (err != ESP_OK) goto cleanup;
    else config->enable_web_server = (bool)temp_bool;
    
    err = nvs_get_u16(handle, KEY_WEB_PORT, &config->web_port);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->web_port = default_config.web_port;
    else if (err != ESP_OK) goto cleanup;
    
    err = nvs_get_u8(handle, KEY_ENABLE_ENC, &temp_bool);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->enable_encryption = default_config.enable_encryption;
    else if (err != ESP_OK) goto cleanup;
    else config->enable_encryption = (bool)temp_bool;
    
    required_size = sizeof(config->aes_key);
    err = nvs_get_blob(handle, KEY_AES_KEY, config->aes_key, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) memcpy(config->aes_key, default_config.aes_key, sizeof(config->aes_key));
    else if (err != ESP_OK) goto cleanup;
    
    required_size = sizeof(config->iv);
    err = nvs_get_blob(handle, KEY_IV, config->iv, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) memcpy(config->iv, default_config.iv, sizeof(config->iv));
    else if (err != ESP_OK) goto cleanup;
    
    err = nvs_get_u8(handle, KEY_ENABLE_CRC, &temp_bool);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->enable_crc = default_config.enable_crc;
    else if (err != ESP_OK) goto cleanup;
    else config->enable_crc = (bool)temp_bool;
    
    err = nvs_get_u8(handle, KEY_ENABLE_LDRO, &temp_bool);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->enable_ldro = default_config.enable_ldro;
    else if (err != ESP_OK) goto cleanup;
    else config->enable_ldro = (bool)temp_bool;
    
    err = nvs_get_u16(handle, KEY_PREAMBLE_LEN, &config->preamble_length);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->preamble_length = default_config.preamble_length;
    else if (err != ESP_OK) goto cleanup;
    
    err = nvs_get_u16(handle, KEY_SYMBOL_TIMEOUT, &config->symbol_timeout);
    if (err == ESP_ERR_NVS_NOT_FOUND) config->symbol_timeout = default_config.symbol_timeout;
    else if (err != ESP_OK) goto cleanup;
    
    ESP_LOGI(TAG, "Configuration loaded from NVS successfully");
    
cleanup:
    nvs_close(handle);
    return err;
}

// Get current configuration
esp_err_t config_load(lora_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Load configuration from NVS
    esp_err_t err = load_config_from_nvs(config);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load config from NVS, using defaults");
        *config = default_config;
        memcpy(config->aes_key, default_aes_key, sizeof(config->aes_key));
        memcpy(config->iv, default_iv, sizeof(config->iv));
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

// Save new configuration
esp_err_t config_save(const lora_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Save configuration to NVS
    esp_err_t err = save_config_to_nvs(config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save configuration to NVS");
        return err;
    }
    
    ESP_LOGI(TAG, "Configuration saved successfully");
    return ESP_OK;
}

// Load default configuration
esp_err_t config_load_defaults(lora_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Copy default configuration
    *config = default_config;
    
    // Copy default AES key and IV
    memcpy(config->aes_key, default_aes_key, sizeof(config->aes_key));
    memcpy(config->iv, default_iv, sizeof(config->iv));
    
    return ESP_OK;
}

// Debug function to show NVS contents
void debug_nvs_contents(void) {
    nvs_handle_t handle;
    esp_err_t err;
    
    ESP_LOGI(TAG, "=== NVS Configuration Debug ===");
    
    err = nvs_open("lora_config", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
        return;
    }
    
    // Test each key
    uint8_t u8_value;
    
    // Test spread_factor specifically
    err = nvs_get_u8(handle, KEY_SPREAD_FACTOR, &u8_value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "%s: KEY NOT FOUND in NVS", KEY_SPREAD_FACTOR);
    } else if (err == ESP_OK) {
        ESP_LOGI(TAG, "%s: FOUND = %d", KEY_SPREAD_FACTOR, u8_value);
    } else {
        ESP_LOGE(TAG, "%s: Error reading: %s", KEY_SPREAD_FACTOR, esp_err_to_name(err));
    }
    
    // Test other problematic keys
    err = nvs_get_u8(handle, KEY_ENABLE_HEALING, &u8_value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "%s: KEY NOT FOUND in NVS", KEY_ENABLE_HEALING);
    } else if (err == ESP_OK) {
        ESP_LOGI(TAG, "%s: FOUND = %d", KEY_ENABLE_HEALING, u8_value);
    } else {
        ESP_LOGE(TAG, "%s: Error reading: %s", KEY_ENABLE_HEALING, esp_err_to_name(err));
    }
    
    err = nvs_get_u8(handle, KEY_ENABLE_WEB, &u8_value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "%s: KEY NOT FOUND in NVS", KEY_ENABLE_WEB);
    } else if (err == ESP_OK) {
        ESP_LOGI(TAG, "%s: FOUND = %d", KEY_ENABLE_WEB, u8_value);
    } else {
        ESP_LOGE(TAG, "%s: Error reading: %s", KEY_ENABLE_WEB, esp_err_to_name(err));
    }
    
    err = nvs_get_u8(handle, KEY_ENABLE_ENC, &u8_value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "%s: KEY NOT FOUND in NVS", KEY_ENABLE_ENC);
    } else if (err == ESP_OK) {
        ESP_LOGI(TAG, "%s: FOUND = %d", KEY_ENABLE_ENC, u8_value);
    } else {
        ESP_LOGE(TAG, "%s: Error reading: %s", KEY_ENABLE_ENC, esp_err_to_name(err));
    }
    
    nvs_close(handle);
    ESP_LOGI(TAG, "=== End Debug ===");
}
void debug_nvs_content(void) {
    nvs_handle_t handle;
    esp_err_t err;
    
    ESP_LOGI(TAG, "=== NVS Configuration Debug ===");
    
    err = nvs_open("lora_config", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
        return;
    }
    
    // Test each key
    uint8_t u8_value;
    
    // Test spread_factor specifically
    err = nvs_get_u8(handle, KEY_SPREAD_FACTOR, &u8_value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "%s: KEY NOT FOUND in NVS", KEY_SPREAD_FACTOR);
    } else if (err == ESP_OK) {
        ESP_LOGI(TAG, "%s: FOUND = %d", KEY_SPREAD_FACTOR, u8_value);
    } else {
        ESP_LOGE(TAG, "%s: Error reading: %s", KEY_SPREAD_FACTOR, esp_err_to_name(err));
    }
    
    nvs_close(handle);
    ESP_LOGI(TAG, "=== End Debug ===");
}

// Print configuration
void config_print(const lora_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Cannot print NULL configuration");
        return;
    }
    
    ESP_LOGI(TAG, "=== Current Configuration ===");
    ESP_LOGI(TAG, "Node ID: %u", config->node_id);
    ESP_LOGI(TAG, "Node Name: %s", config->node_name);
    ESP_LOGI(TAG, "Frequency: %lu Hz", config->frequency);
    ESP_LOGI(TAG, "Spread Factor: SF%d", config->spread_factor);
    ESP_LOGI(TAG, "Bandwidth: %lu Hz", config->bandwidth);
    ESP_LOGI(TAG, "Coding Rate: 4/%d", config->coding_rate);
    ESP_LOGI(TAG, "TX Power: %d dBm", config->tx_power);
    ESP_LOGI(TAG, "Sync Word: 0x%02X", config->sync_word);
    ESP_LOGI(TAG, "Ping Interval: %u seconds", config->ping_interval);
    ESP_LOGI(TAG, "Beacon Interval: %u seconds", config->beacon_interval);
    ESP_LOGI(TAG, "Route Timeout: %u seconds", config->route_timeout);
    ESP_LOGI(TAG, "Max Hops: %u", config->max_hops);
    ESP_LOGI(TAG, "Enable ACK: %s", config->enable_ack ? "Yes" : "No");
    ESP_LOGI(TAG, "ACK Timeout: %u ms", config->ack_timeout);
    ESP_LOGI(TAG, "Enable Self-Healing: %s", config->enable_self_healing ? "Yes" : "No");
    ESP_LOGI(TAG, "Healing Timeout: %u seconds", config->healing_timeout);
    ESP_LOGI(TAG, "WiFi SSID: %s", config->wifi_ssid);
    ESP_LOGI(TAG, "WiFi Password: [PROTECTED]");
    ESP_LOGI(TAG, "Enable Web Server: %s", config->enable_web_server ? "Yes" : "No");
    ESP_LOGI(TAG, "Web Port: %u", config->web_port);
    ESP_LOGI(TAG, "Enable Encryption: %s", config->enable_encryption ? "Yes" : "No");
    ESP_LOGI(TAG, "Enable CRC: %s", config->enable_crc ? "Yes" : "No");
    ESP_LOGI(TAG, "Enable LDRO: %s", config->enable_ldro ? "Yes" : "No");
    ESP_LOGI(TAG, "Preamble Length: %u", config->preamble_length);
    ESP_LOGI(TAG, "Symbol Timeout: %u", config->symbol_timeout);
    ESP_LOGI(TAG, "=== End Configuration ===");
}