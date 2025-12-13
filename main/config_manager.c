#include "config_manager.h"
#include "esp_log.h"
#include "nvs.h"
#include "esp_random.h"
#include <string.h>
#include "esp_timer.h"
static const char *TAG = "CONFIG";
static const char *NVS_NAMESPACE = "lora_mesh";

// ==================== GLOBAL VARIABLES ====================
static node_info_t g_nodes[20];      // Maximum 20 nodes
static uint8_t g_node_count = 0;

static route_info_t g_routes[20];    // Maximum 20 routes
static uint8_t g_route_count = 0;

static lora_config_t *g_current_config = NULL;

// ==================== FORWARD DECLARATIONS ====================
// Add this section to declare functions that are called before they're defined
static void mesh_remove_routes_via_node(uint8_t node_id);

// ==================== CONFIGURATION MANAGEMENT ====================

// Initialize configuration system
void config_init(void) {
    ESP_LOGI(TAG, "Configuration system initialized");
    mesh_init_nodes();
    mesh_init_routes();
}

// Load default configuration
void config_load_defaults(lora_config_t *config) {
    // Node identity
    config->node_id = 1;
    strcpy(config->node_name, "LoRa-Master");
    
    // LoRa parameters (868MHz EU band)
    config->frequency = 868000000;
    config->spreading_factor = 7;
    config->bandwidth = 125000;
    config->coding_rate = 5;
    config->tx_power = 17;
    config->sync_word = 0x12;
    
    // Network parameters
    config->ping_interval = 30;
    config->beacon_interval = 60;
    config->route_timeout = 300;
    config->max_hops = 5;
    config->enable_ack = true;
    config->ack_timeout = 1000;
    
    // Mesh parameters
    config->enable_self_healing = true;
    config->healing_timeout = 30;
    
    // WiFi AP
    if (config->node_id == 1) {
        strcpy(config->wifi_ssid, "LoRa-Master");
    } else {
        snprintf(config->wifi_ssid, sizeof(config->wifi_ssid), "LoRa-Slave-%d", config->node_id);
    }
    strcpy(config->wifi_password, "12345678");
    
    // Web server
    config->enable_web_server = true;
    config->web_port = 80;
    
    // Encryption (generate random key)
    esp_fill_random(config->aes_key, 16);
    esp_fill_random(config->iv, 16);
    config->enable_encryption = true;
    
    // Advanced
    config->enable_crc = true;
    config->enable_ldro = true;
    config->preamble_length = 8;
    config->symbol_timeout = 5;
    
    g_current_config = config;
}
// Debug function to print all NVS content
void debug_nvs_content(void) {
    nvs_handle_t handle;
    esp_err_t err;
    
    ESP_LOGI(TAG, "=== NVS DEBUG DUMP ===");
    
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
        return;
    }
    
    // 1. Print all keys with their types and values
    ESP_LOGI(TAG, "--- All Keys in NVS ---");
    
    // Node Identity
    uint8_t node_id = 0;
    err = nvs_get_u8(handle, "node_id", &node_id);
    ESP_LOGI(TAG, "node_id: %s = %u", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", node_id);
    
    char node_name[32] = {0};
    size_t len = sizeof(node_name);
    err = nvs_get_str(handle, "node_name", node_name, &len);
    ESP_LOGI(TAG, "node_name: %s = '%s'", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", node_name);
    
    // LoRa Parameters
    uint32_t frequency = 0;
    err = nvs_get_u32(handle, "frequency", &frequency);
    ESP_LOGI(TAG, "frequency: %s = %lu Hz (%.1f MHz)", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", 
             frequency, frequency / 1000000.0);
    
    uint8_t spreading_factor = 0;
    err = nvs_get_u8(handle, "spreading_factor", &spreading_factor);
    ESP_LOGI(TAG, "spreading_factor: %s = %u", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", spreading_factor);
    
    uint32_t bandwidth = 0;
    err = nvs_get_u32(handle, "bandwidth", &bandwidth);
    ESP_LOGI(TAG, "bandwidth: %s = %lu Hz", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", bandwidth);
    
    uint8_t coding_rate = 0;
    err = nvs_get_u8(handle, "coding_rate", &coding_rate);
    ESP_LOGI(TAG, "coding_rate: %s = %u (4/%u)", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", coding_rate, coding_rate);
    
    int8_t tx_power = 0;
    err = nvs_get_i8(handle, "tx_power", &tx_power);
    ESP_LOGI(TAG, "tx_power: %s = %d dBm", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", tx_power);
    
    uint8_t sync_word = 0;
    err = nvs_get_u8(handle, "sync_word", &sync_word);
    ESP_LOGI(TAG, "sync_word: %s = 0x%02X", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", sync_word);
    
    // Network Parameters
    uint16_t ping_interval = 0;
    err = nvs_get_u16(handle, "ping_interval", &ping_interval);
    ESP_LOGI(TAG, "ping_interval: %s = %u seconds", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", ping_interval);
    
    uint16_t beacon_interval = 0;
    err = nvs_get_u16(handle, "beacon_interval", &beacon_interval);
    ESP_LOGI(TAG, "beacon_interval: %s = %u seconds", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", beacon_interval);
    
    uint16_t route_timeout = 0;
    err = nvs_get_u16(handle, "route_timeout", &route_timeout);
    ESP_LOGI(TAG, "route_timeout: %s = %u seconds", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", route_timeout);
    
    uint8_t max_hops = 0;
    err = nvs_get_u8(handle, "max_hops", &max_hops);
    ESP_LOGI(TAG, "max_hops: %s = %u", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", max_hops);
    
    uint8_t enable_ack = 0;
    err = nvs_get_u8(handle, "enable_ack", &enable_ack);
    ESP_LOGI(TAG, "enable_ack: %s = %s", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", 
             enable_ack ? "true" : "false");
    
    uint16_t ack_timeout = 0;
    err = nvs_get_u16(handle, "ack_timeout", &ack_timeout);
    ESP_LOGI(TAG, "ack_timeout: %s = %u ms", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", ack_timeout);
    
    // Mesh Parameters
    uint8_t enable_self_healing = 0;
    err = nvs_get_u8(handle, "enable_self_healing", &enable_self_healing);
    ESP_LOGI(TAG, "enable_self_healing: %s = %s", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", 
             enable_self_healing ? "true" : "false");
    
    uint16_t healing_timeout = 0;
    err = nvs_get_u16(handle, "healing_timeout", &healing_timeout);
    ESP_LOGI(TAG, "healing_timeout: %s = %u seconds", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", healing_timeout);
    
    // WiFi AP Parameters
    char wifi_ssid[32] = {0};
    len = sizeof(wifi_ssid);
    err = nvs_get_str(handle, "wifi_ssid", wifi_ssid, &len);
    ESP_LOGI(TAG, "wifi_ssid: %s = '%s'", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", wifi_ssid);
    
    char wifi_password[32] = {0};
    len = sizeof(wifi_password);
    err = nvs_get_str(handle, "wifi_password", wifi_password, &len);
    ESP_LOGI(TAG, "wifi_password: %s = '%s'", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", 
             err == ESP_OK ? (strlen(wifi_password) > 0 ? "***SET***" : "empty") : "NOT FOUND");
    
    // Web Server
    uint8_t enable_web_server = 0;
    err = nvs_get_u8(handle, "enable_web_server", &enable_web_server);
    ESP_LOGI(TAG, "enable_web_server: %s = %s", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", 
             enable_web_server ? "true" : "false");
    
    uint16_t web_port = 0;
    err = nvs_get_u16(handle, "web_port", &web_port);
    ESP_LOGI(TAG, "web_port: %s = %u", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", web_port);
    
    // Encryption - CRITICAL FOR DEBUGGING
    uint8_t enable_encryption = 0;
    err = nvs_get_u8(handle, "enable_encryption", &enable_encryption);
    ESP_LOGI(TAG, "enable_encryption: %s = %s", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", 
             enable_encryption ? "true" : "false");
    
    uint8_t aes_key[16] = {0};
    len = sizeof(aes_key);
    err = nvs_get_blob(handle, "aes_key", aes_key, &len);
    ESP_LOGI(TAG, "aes_key: %s (len: %d)", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", len);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "  Key hex: %02X%02X%02X%02X...%02X%02X%02X%02X",
                aes_key[0], aes_key[1], aes_key[2], aes_key[3],
                aes_key[12], aes_key[13], aes_key[14], aes_key[15]);
    }
    
    uint8_t iv[16] = {0};
    len = sizeof(iv);
    err = nvs_get_blob(handle, "iv", iv, &len);
    ESP_LOGI(TAG, "iv: %s (len: %d)", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", len);
    
    // Advanced LoRa Parameters
    uint8_t enable_crc = 0;
    err = nvs_get_u8(handle, "enable_crc", &enable_crc);
    ESP_LOGI(TAG, "enable_crc: %s = %s", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", 
             enable_crc ? "true" : "false");
    
    uint8_t enable_ldro = 0;
    err = nvs_get_u8(handle, "enable_ldro", &enable_ldro);
    ESP_LOGI(TAG, "enable_ldro: %s = %s", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", 
             enable_ldro ? "true" : "false");
    
    uint16_t preamble_length = 0;
    err = nvs_get_u16(handle, "preamble_length", &preamble_length);
    ESP_LOGI(TAG, "preamble_length: %s = %u", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", preamble_length);
    
    uint16_t symbol_timeout = 0;
    err = nvs_get_u16(handle, "symbol_timeout", &symbol_timeout);
    ESP_LOGI(TAG, "symbol_timeout: %s = %u", 
             err == ESP_OK ? "FOUND" : "NOT FOUND", symbol_timeout);
    
    // 2. List all keys in NVS (iterate through)
    ESP_LOGI(TAG, "--- Listing All Keys ---");
    nvs_iterator_t it = NULL;
    esp_err_t res = nvs_entry_find(NVS_DEFAULT_PART_NAME, NVS_NAMESPACE, NVS_TYPE_ANY, &it);
    
    int key_count = 0;
    while (res == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        
        ESP_LOGI(TAG, "Key #%d: '%s', Type: %s, Size: %d", 
                 ++key_count, 
                 info.key,
                 info.type == NVS_TYPE_U8 ? "uint8" :
                 info.type == NVS_TYPE_I8 ? "int8" :
                 info.type == NVS_TYPE_U16 ? "uint16" :
                 info.type == NVS_TYPE_I16 ? "int16" :
                 info.type == NVS_TYPE_U32 ? "uint32" :
                 info.type == NVS_TYPE_I32 ? "int32" :
                 info.type == NVS_TYPE_STR ? "string" :
                 info.type == NVS_TYPE_BLOB ? "blob" : "unknown"
                 );
        
        res = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
    
    ESP_LOGI(TAG, "Total keys found: %d", key_count);
    ESP_LOGI(TAG, "=== END NVS DEBUG ===");
    
    nvs_close(handle);
}


// Save configuration to NVS
bool config_save(lora_config_t *config) {
    nvs_handle_t handle;
    esp_err_t err;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
        return false;
    }
    
    // Save all configuration parameters
    nvs_set_u8(handle, "node_id", config->node_id);
    nvs_set_str(handle, "node_name", config->node_name);
    
    nvs_set_u32(handle, "frequency", config->frequency);
    nvs_set_u8(handle, "spreading_factor", config->spreading_factor);
    nvs_set_u32(handle, "bandwidth", config->bandwidth);
    nvs_set_u8(handle, "coding_rate", config->coding_rate);
    nvs_set_i8(handle, "tx_power", config->tx_power);
    nvs_set_u8(handle, "sync_word", config->sync_word);
    
    nvs_set_u16(handle, "ping_interval", config->ping_interval);
    nvs_set_u16(handle, "beacon_interval", config->beacon_interval);
    nvs_set_u16(handle, "route_timeout", config->route_timeout);
    nvs_set_u8(handle, "max_hops", config->max_hops);
    nvs_set_u8(handle, "enable_ack", config->enable_ack);
    nvs_set_u16(handle, "ack_timeout", config->ack_timeout);
    
    nvs_set_u8(handle, "enable_self_healing", config->enable_self_healing);
    nvs_set_u16(handle, "healing_timeout", config->healing_timeout);
    
    nvs_set_str(handle, "wifi_ssid", config->wifi_ssid);
    nvs_set_str(handle, "wifi_password", config->wifi_password);
    
    nvs_set_u8(handle, "enable_web_server", config->enable_web_server);
    nvs_set_u16(handle, "web_port", config->web_port);
    
    nvs_set_blob(handle, "aes_key", config->aes_key, 16);
    nvs_set_blob(handle, "iv", config->iv, 16);
    nvs_set_u8(handle, "enable_encryption", config->enable_encryption);
    
    nvs_set_u8(handle, "enable_crc", config->enable_crc);
    nvs_set_u8(handle, "enable_ldro", config->enable_ldro);
    nvs_set_u16(handle, "preamble_length", config->preamble_length);
    nvs_set_u16(handle, "symbol_timeout", config->symbol_timeout);
    
    err = nvs_commit(handle);
    nvs_close(handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving config: %s", esp_err_to_name(err));
        return false;
    }
    
    ESP_LOGI(TAG, "Configuration saved to NVS");
    return true;
}

// Load configuration from NVS
bool config_load(lora_config_t *config) {
    nvs_handle_t handle;
    esp_err_t err;
    size_t len;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
        return false;
    }
    
    // Load all configuration parameters
    nvs_get_u8(handle, "node_id", &config->node_id);
    
    len = sizeof(config->node_name);
    nvs_get_str(handle, "node_name", config->node_name, &len);
    
    nvs_get_u32(handle, "frequency", &config->frequency);
    err = nvs_get_u8(handle, "spreading_factor", &config->spreading_factor);
    if (err != ESP_OK) {
        config->spreading_factor = 7;  // Default if not found
        ESP_LOGW(TAG, "SF not found in NVS, using default SF7");
    }
    nvs_get_u32(handle, "bandwidth", &config->bandwidth);
    nvs_get_u8(handle, "coding_rate", &config->coding_rate);
    nvs_get_i8(handle, "tx_power", &config->tx_power);
    nvs_get_u8(handle, "sync_word", &config->sync_word);
    
    nvs_get_u16(handle, "ping_interval", &config->ping_interval);
    nvs_get_u16(handle, "beacon_interval", &config->beacon_interval);
    nvs_get_u16(handle, "route_timeout", &config->route_timeout);
    nvs_get_u8(handle, "max_hops", &config->max_hops);
    nvs_get_u8(handle, "enable_ack", (uint8_t*)&config->enable_ack);
    nvs_get_u16(handle, "ack_timeout", &config->ack_timeout);
    
    nvs_get_u8(handle, "enable_self_healing", (uint8_t*)&config->enable_self_healing);
    nvs_get_u16(handle, "healing_timeout", &config->healing_timeout);
    
    len = sizeof(config->wifi_ssid);
    nvs_get_str(handle, "wifi_ssid", config->wifi_ssid, &len);
    len = sizeof(config->wifi_password);
    nvs_get_str(handle, "wifi_password", config->wifi_password, &len);
    
    nvs_get_u8(handle, "enable_web_server", (uint8_t*)&config->enable_web_server);
    nvs_get_u16(handle, "web_port", &config->web_port);
    
    len = 16;
    nvs_get_blob(handle, "aes_key", config->aes_key, &len);
    len = 16;
    nvs_get_blob(handle, "iv", config->iv, &len);
    //nvs_get_u8(handle, "enable_encryption", (uint8_t*)&config->enable_encryption);
    err = nvs_get_u8(handle, "enable_encryption", (uint8_t*)&config->enable_encryption);
    if (err != ESP_OK) 
    {
        config->enable_encryption = true;  // Default to enabled
    }
    nvs_get_u8(handle, "enable_crc", (uint8_t*)&config->enable_crc);
    nvs_get_u8(handle, "enable_ldro", (uint8_t*)&config->enable_ldro);
    nvs_get_u16(handle, "preamble_length", &config->preamble_length);
    nvs_get_u16(handle, "symbol_timeout", &config->symbol_timeout);
    
    nvs_close(handle);
    
    g_current_config = config;
    ESP_LOGI(TAG, "Configuration loaded from NVS");
    return true;
}

// Print configuration
void config_print(const lora_config_t *config) {
    ESP_LOGI(TAG, "=== Node Configuration ===");
    ESP_LOGI(TAG, "Node ID: %d, Name: %s", config->node_id, config->node_name);
    ESP_LOGI(TAG, "Frequency: %.1f MHz", config->frequency / 1000000.0);
    ESP_LOGI(TAG, "SF: %d, BW: %lu Hz, CR: 4/%d", 
             config->spreading_factor, config->bandwidth, config->coding_rate);
    ESP_LOGI(TAG, "TX Power: %d dBm", config->tx_power);
    ESP_LOGI(TAG, "Ping Interval: %d s", config->ping_interval);
    ESP_LOGI(TAG, "Beacon Interval: %d s", config->beacon_interval);
    ESP_LOGI(TAG, "Route Timeout: %d s", config->route_timeout);
    ESP_LOGI(TAG, "Max Hops: %d", config->max_hops);
    ESP_LOGI(TAG, "Self-Healing: %s", config->enable_self_healing ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "Encryption: %s", config->enable_encryption ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "WiFi SSID: %s", config->wifi_ssid);
    ESP_LOGI(TAG, "Web Server: %s:%d", 
             config->enable_web_server ? "Enabled" : "Disabled", 
             config->web_port);
    ESP_LOGI(TAG, "CRC: %s, LDRO: %s", 
             config->enable_crc ? "Enabled" : "Disabled",
             config->enable_ldro ? "Enabled" : "Disabled");
}

// Validate configuration - FIXED VERSION
bool config_validate(const lora_config_t *config) {
    if (config->node_id == 0) {  // Removed the > 255 check since uint8_t can't be > 255
        ESP_LOGE(TAG, "Invalid node ID: %d", config->node_id);
        return false;
    }
    
    if (config->spreading_factor < 7 || config->spreading_factor > 12) {
        ESP_LOGE(TAG, "Invalid spreading factor: %d", config->spreading_factor);
        return false;
    }
    
    if (config->tx_power < 2 || config->tx_power > 20) {
        ESP_LOGE(TAG, "Invalid TX power: %d dBm", config->tx_power);
        return false;
    }
    
    if (config->coding_rate < 5 || config->coding_rate > 8) {
        ESP_LOGE(TAG, "Invalid coding rate: %d", config->coding_rate);
        return false;
    }
    
    return true;
}

// ==================== NODE MANAGEMENT ====================

// Initialize node list
void mesh_init_nodes(void) {
    g_node_count = 0;
    memset(g_nodes, 0, sizeof(g_nodes));
    
    // Add self node
    if (g_current_config) {
        mesh_add_or_update_node(g_current_config->node_id, 
                               g_current_config->node_name, 
                               0,  // RSSI for self
                               0); // Hop count for self
    }
    
    ESP_LOGI(TAG, "Node list initialized");
}

// Get number of nodes
uint8_t mesh_get_node_count(void) {
    return g_node_count;
}

// Get node by index
node_info_t *mesh_get_node(uint8_t index) {
    if (index < g_node_count) {
        return &g_nodes[index];
    }
    return NULL;
}

// Find node by ID
node_info_t *mesh_find_node(uint8_t id) {
    for (int i = 0; i < g_node_count; i++) {
        if (g_nodes[i].id == id) {
            return &g_nodes[i];
        }
    }
    return NULL;
}

// Add or update node
void mesh_add_or_update_node(uint8_t id, const char *name, int16_t rssi, uint8_t hop_count) {
    node_info_t *node = mesh_find_node(id);
    int64_t now = get_time_us();
    
    if (node) {
        // Update existing node
        if (name && strlen(name) > 0) {
            strncpy(node->name, name, sizeof(node->name) - 1);
        }
        node->rssi = rssi;
        node->hop_count = hop_count;
        node->last_seen = now;
        node->online = true;
        
        ESP_LOGI(TAG, "Updated node %d: RSSI=%d, Hops=%d", id, rssi, hop_count);
    } else {
        // Add new node
        if (g_node_count < 20) {
            g_nodes[g_node_count].id = id;
            if (name && strlen(name) > 0) {
                strncpy(g_nodes[g_node_count].name, name, sizeof(g_nodes[g_node_count].name) - 1);
            } else {
                snprintf(g_nodes[g_node_count].name, sizeof(g_nodes[g_node_count].name), "Node-%d", id);
            }
            g_nodes[g_node_count].rssi = rssi;
            g_nodes[g_node_count].hop_count = hop_count;
            g_nodes[g_node_count].last_seen = now;
            g_nodes[g_node_count].uptime = 0;
            g_nodes[g_node_count].online = true;
            g_node_count++;
            
            ESP_LOGI(TAG, "Added new node %d: RSSI=%d, Hops=%d", id, rssi, hop_count);
        } else {
            ESP_LOGW(TAG, "Node list full, cannot add node %d", id);
        }
    }
}

// Update node RSSI
void mesh_update_node_rssi(uint8_t id, int16_t rssi) {
    node_info_t *node = mesh_find_node(id);
    if (node) {
        node->rssi = rssi;
        node->last_seen = get_time_us();
    }
}

// Update node last seen time
void mesh_update_node_seen(uint8_t id) {
    node_info_t *node = mesh_find_node(id);
    if (node) {
        node->last_seen = get_time_us();
        node->online = true;
    }
}

// Remove node
void mesh_remove_node(uint8_t id) {
    for (int i = 0; i < g_node_count; i++) {
        if (g_nodes[i].id == id) {
            // Shift remaining nodes
            for (int j = i; j < g_node_count - 1; j++) {
                g_nodes[j] = g_nodes[j + 1];
            }
            g_node_count--;
            ESP_LOGI(TAG, "Removed node %d", id);
            
            // Also remove any routes through this node
            mesh_remove_routes_via_node(id);
            return;
        }
    }
}

// Clear offline nodes
void mesh_clear_offline_nodes(uint32_t timeout_seconds) {
    int64_t now = get_time_us();
    int64_t timeout_us = timeout_seconds * 1000000ULL;
    
    for (int i = 0; i < g_node_count; i++) {
        if (g_nodes[i].id != g_current_config->node_id && 
            now - g_nodes[i].last_seen > timeout_us) {
            ESP_LOGW(TAG, "Node %d marked offline (last seen %llds ago)", 
                    g_nodes[i].id, (now - g_nodes[i].last_seen) / 1000000);
            g_nodes[i].online = false;
        }
    }
}

// Get number of online nodes
uint8_t mesh_get_online_count(void) {
    uint8_t count = 0;
    for (int i = 0; i < g_node_count; i++) {
        if (g_nodes[i].online) {
            count++;
        }
    }
    return count;
}

// ==================== ROUTE MANAGEMENT ====================

// Initialize route list
void mesh_init_routes(void) {
    g_route_count = 0;
    memset(g_routes, 0, sizeof(g_routes));
    ESP_LOGI(TAG, "Route list initialized");
}

// Get number of routes
uint8_t mesh_get_route_count(void) {
    return g_route_count;
}

// Get route by index
route_info_t *mesh_get_route(uint8_t index) {
    if (index < g_route_count) {
        return &g_routes[index];
    }
    return NULL;
}

// Find route by destination
route_info_t *mesh_find_route(uint8_t destination) {
    for (int i = 0; i < g_route_count; i++) {
        if (g_routes[i].destination == destination) {
            return &g_routes[i];
        }
    }
    return NULL;
}

// Add or update route
void mesh_add_or_update_route(uint8_t dest, uint8_t next_hop, uint8_t hop_count, int16_t link_quality) {
    route_info_t *route = mesh_find_route(dest);
    int64_t now = get_time_us();
    
    if (route) {
        // Update existing route
        route->next_hop = next_hop;
        route->hop_count = hop_count;
        route->link_quality = link_quality;
        route->last_update = now;
        route->active = true;
        
        ESP_LOGI(TAG, "Updated route to %d via %d (hops=%d, quality=%d)", 
                dest, next_hop, hop_count, link_quality);
    } else {
        // Add new route
        if (g_route_count < 20) {
            g_routes[g_route_count].destination = dest;
            g_routes[g_route_count].next_hop = next_hop;
            g_routes[g_route_count].hop_count = hop_count;
            g_routes[g_route_count].link_quality = link_quality;
            g_routes[g_route_count].last_update = now;
            g_routes[g_route_count].active = true;
            g_route_count++;
            
            ESP_LOGI(TAG, "Added route to %d via %d (hops=%d, quality=%d)", 
                    dest, next_hop, hop_count, link_quality);
        } else {
            ESP_LOGW(TAG, "Route list full, cannot add route to %d", dest);
        }
    }
}

// Update route link quality
void mesh_update_route_quality(uint8_t dest, int16_t link_quality) {
    route_info_t *route = mesh_find_route(dest);
    if (route) {
        // Weighted average for link quality
        route->link_quality = (route->link_quality * 3 + link_quality) / 4;
        route->last_update = get_time_us();
    }
}

// Remove route
void mesh_remove_route(uint8_t dest) {
    for (int i = 0; i < g_route_count; i++) {
        if (g_routes[i].destination == dest) {
            // Shift remaining routes
            for (int j = i; j < g_route_count - 1; j++) {
                g_routes[j] = g_routes[j + 1];
            }
            g_route_count--;
            ESP_LOGI(TAG, "Removed route to %d", dest);
            return;
        }
    }
}

// Remove routes via specific node - FIXED: Now properly declared
static void mesh_remove_routes_via_node(uint8_t node_id) {
    for (int i = 0; i < g_route_count; i++) {
        if (g_routes[i].next_hop == node_id) {
            ESP_LOGW(TAG, "Removing route to %d via offline node %d", 
                    g_routes[i].destination, node_id);
            mesh_remove_route(g_routes[i].destination);
            i--; // Check same index again after removal
        }
    }
}

// Clear expired routes
void mesh_clear_expired_routes(uint32_t timeout_seconds) {
    int64_t now = get_time_us();
    int64_t timeout_us = timeout_seconds * 1000000ULL;
    
    for (int i = 0; i < g_route_count; i++) {
        if (now - g_routes[i].last_update > timeout_us) {
            ESP_LOGW(TAG, "Route to %d expired (last update %llds ago)", 
                    g_routes[i].destination, (now - g_routes[i].last_update) / 1000000);
            g_routes[i].active = false;
        }
    }
}

// Get number of active routes
uint8_t mesh_get_active_route_count(void) {
    uint8_t count = 0;
    for (int i = 0; i < g_route_count; i++) {
        if (g_routes[i].active) {
            count++;
        }
    }
    return count;
}

// ==================== MESH NETWORK FUNCTIONS ====================

// Check route timeouts
void mesh_check_route_timeouts(void) {
    if (!g_current_config) return;
    
    int64_t now = get_time_us();
    int64_t timeout_us = g_current_config->route_timeout * 1000000ULL;
    
    for (int i = 0; i < g_route_count; i++) {
        if (g_routes[i].active && now - g_routes[i].last_update > timeout_us) {
            ESP_LOGW(TAG, "Route to %d timed out", g_routes[i].destination);
            g_routes[i].active = false;
        }
    }
}



// Check if node is reachable
bool mesh_is_node_reachable(uint8_t node_id) {
    route_info_t *route = mesh_find_route(node_id);
    return (route != NULL && route->active);
}

// Get next hop for destination
uint8_t mesh_get_next_hop(uint8_t destination) {
    route_info_t *route = mesh_find_route(destination);
    if (route && route->active) {
        return route->next_hop;
    }
    return 0; // 0 means no route found
}

// Get link quality to node
int16_t mesh_get_link_quality(uint8_t node_id) {
    route_info_t *route = mesh_find_route(node_id);
    if (route && route->active) {
        return route->link_quality;
    }
    return -127; // Minimum RSSI value
}

// ==================== TIME HELPER FUNCTIONS ====================

// Get current time in microseconds
int64_t get_time_us(void) {
    return esp_timer_get_time();
}

// Get current time in milliseconds
int64_t get_time_ms(void) {
    return esp_timer_get_time() / 1000;
}

// Get current time in seconds
int64_t get_time_sec(void) {
    return esp_timer_get_time() / 1000000;
}

// Check if timeout has expired
bool is_timeout_expired(int64_t start_time, uint32_t timeout_ms) {
    int64_t now = get_time_ms();
    return (now - start_time) >= timeout_ms;
}

// ==================== ADDITIONAL HELPER FUNCTIONS ====================

// Update self node uptime
void mesh_update_self_uptime(void) {
    if (!g_current_config) return;
    
    node_info_t *self = mesh_find_node(g_current_config->node_id);
    if (self) {
        self->uptime = get_time_us();
        self->online = true;
        self->last_seen = get_time_us();
    }
}

// Print node list for debugging
void mesh_print_nodes(void) {
    ESP_LOGI(TAG, "=== Node List (%d nodes) ===", g_node_count);
    for (int i = 0; i < g_node_count; i++) {
        int64_t last_seen_sec = (get_time_us() - g_nodes[i].last_seen) / 1000000;
        ESP_LOGI(TAG, "Node %d: %s, RSSI=%d, Hops=%d, Online=%s, Last seen=%llds ago",
                g_nodes[i].id, g_nodes[i].name, g_nodes[i].rssi, g_nodes[i].hop_count,
                g_nodes[i].online ? "Yes" : "No", last_seen_sec);
    }
}

// Print route list for debugging
void mesh_print_routes(void) {
    ESP_LOGI(TAG, "=== Route List (%d routes) ===", g_route_count);
    for (int i = 0; i < g_route_count; i++) {
        int64_t last_update_sec = (get_time_us() - g_routes[i].last_update) / 1000000;
        ESP_LOGI(TAG, "Route: To %d via %d, Hops=%d, Quality=%d, Active=%s, Last update=%llds ago",
                g_routes[i].destination, g_routes[i].next_hop, g_routes[i].hop_count,
                g_routes[i].link_quality, g_routes[i].active ? "Yes" : "No", last_update_sec);
    }
}

// Get statistics
void mesh_get_statistics(uint8_t *total_nodes, uint8_t *online_nodes, 
                         uint8_t *total_routes, uint8_t *active_routes) {
    if (total_nodes) *total_nodes = g_node_count;
    if (online_nodes) *online_nodes = mesh_get_online_count();
    if (total_routes) *total_routes = g_route_count;
    if (active_routes) *active_routes = mesh_get_active_route_count();
}