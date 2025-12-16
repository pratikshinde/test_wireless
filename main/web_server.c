// In web_server.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "config_manager.h"
#include "mesh_manager.h"
#include "mesh_types.h"
#include "lora_mesh.h"


static const char *TAG = "WEB_SERVER";

// HTML files (embedded)
extern const char dashboard_html_start[] asm("_binary_dashboard_html_start");
extern const char dashboard_html_end[] asm("_binary_dashboard_html_end");
extern const char lora_config_html_start[] asm("_binary_lora_config_html_start");
extern const char lora_config_html_end[] asm("_binary_lora_config_html_end");
extern const char devices_html_start[] asm("_binary_devices_html_start");
extern const char devices_html_end[] asm("_binary_devices_html_end");
extern const char styles_css_start[] asm("_binary_styles_css_start");
extern const char styles_css_end[] asm("_binary_styles_css_end");
extern const char script_js_start[] asm("_binary_script_js_start");
extern const char script_js_end[] asm("_binary_script_js_end");

// Forward declarations
static esp_err_t dashboard_html_handler(httpd_req_t *req);
static esp_err_t lora_config_html_handler(httpd_req_t *req);
static esp_err_t devices_html_handler(httpd_req_t *req);
static esp_err_t styles_css_handler(httpd_req_t *req);
static esp_err_t script_js_handler(httpd_req_t *req);
static esp_err_t get_config_handler(httpd_req_t *req);
static esp_err_t get_nodes_handler(httpd_req_t *req);
static esp_err_t post_ping_handler(httpd_req_t *req);
static esp_err_t post_discover_handler(httpd_req_t *req);
static esp_err_t favicon_handler(httpd_req_t *req);
static esp_err_t test_handler(httpd_req_t *req);
static esp_err_t post_config_handler(httpd_req_t *req);
static esp_err_t get_config_api_handler(httpd_req_t *req);
static esp_err_t post_config_direct_handler(httpd_req_t *req);
static esp_err_t config_redirect_handler(httpd_req_t *req);
static esp_err_t get_devices_api_handler(httpd_req_t *req);

static esp_err_t get_config_api_handler(httpd_req_t *req);

// Helper function to send JSON response
static esp_err_t send_json_response(httpd_req_t *req, bool success, const char *message, cJSON *data) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "success", success);
    
    if (message != NULL) {
        cJSON_AddStringToObject(root, "message", message);
    }
    
    if (data != NULL) {
        cJSON_AddItemToObject(root, "data", data);
    }
    
    char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free(json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

// GET /config - Redirect to HTML page
static esp_err_t config_redirect_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Redirecting /config to /lora-config.html");
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/lora-config.html");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// GET /api/config - Actual JSON API
static esp_err_t get_config_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /config");
    
    lora_config_t config;
    esp_err_t err = config_load(&config);
    
    if (err != ESP_OK) {
        return send_json_response(req, false, "Failed to load configuration", NULL);
    }
    
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "node_id", config.node_id);
    cJSON_AddStringToObject(data, "node_name", config.node_name);
    cJSON_AddNumberToObject(data, "frequency", config.frequency);
    cJSON_AddNumberToObject(data, "spread_factor", config.spread_factor);
    cJSON_AddNumberToObject(data, "bandwidth", config.bandwidth);
    cJSON_AddNumberToObject(data, "coding_rate", config.coding_rate);
    cJSON_AddNumberToObject(data, "tx_power", config.tx_power);
    cJSON_AddNumberToObject(data, "sync_word", config.sync_word);
    cJSON_AddNumberToObject(data, "ping_interval", config.ping_interval);
    cJSON_AddNumberToObject(data, "beacon_interval", config.beacon_interval);
    cJSON_AddNumberToObject(data, "route_timeout", config.route_timeout);
    cJSON_AddNumberToObject(data, "max_hops", config.max_hops);
    cJSON_AddBoolToObject(data, "enable_ack", config.enable_ack);
    cJSON_AddNumberToObject(data, "ack_timeout", config.ack_timeout);
    cJSON_AddBoolToObject(data, "enable_self_healing", config.enable_self_healing);
    cJSON_AddNumberToObject(data, "healing_timeout", config.healing_timeout);
    cJSON_AddStringToObject(data, "wifi_ssid", config.wifi_ssid);
    cJSON_AddBoolToObject(data, "enable_web_server", config.enable_web_server);
    cJSON_AddNumberToObject(data, "web_port", config.web_port);
    cJSON_AddBoolToObject(data, "enable_encryption", config.enable_encryption);
    cJSON_AddBoolToObject(data, "enable_crc", config.enable_crc);
    cJSON_AddBoolToObject(data, "enable_ldro", config.enable_ldro);
    cJSON_AddNumberToObject(data, "preamble_length", config.preamble_length);
    cJSON_AddNumberToObject(data, "symbol_timeout", config.symbol_timeout);
    
    return send_json_response(req, true, "Configuration loaded", data);
}

// GET /api/config - Actual JSON API
static esp_err_t get_config_api_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /api/config - Reading configuration");
    
    lora_config_t config;
    esp_err_t err = config_load(&config);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load config from NVS: %s", esp_err_to_name(err));
        return send_json_response(req, false, "Failed to load configuration from storage", NULL);
    }
    
    ESP_LOGI(TAG, "Config loaded: node_id=%d, node_name=%s", config.node_id, config.node_name);
    
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "node_id", config.node_id);
    cJSON_AddStringToObject(data, "node_name", config.node_name);
    cJSON_AddNumberToObject(data, "frequency", config.frequency);
    cJSON_AddNumberToObject(data, "spread_factor", config.spread_factor);
    cJSON_AddNumberToObject(data, "bandwidth", config.bandwidth);
    cJSON_AddNumberToObject(data, "coding_rate", config.coding_rate);
    cJSON_AddNumberToObject(data, "tx_power", config.tx_power);
    cJSON_AddNumberToObject(data, "sync_word", config.sync_word);
    cJSON_AddNumberToObject(data, "ping_interval", config.ping_interval);
    cJSON_AddNumberToObject(data, "beacon_interval", config.beacon_interval);
    cJSON_AddNumberToObject(data, "route_timeout", config.route_timeout);
    cJSON_AddNumberToObject(data, "max_hops", config.max_hops);
    cJSON_AddBoolToObject(data, "enable_ack", config.enable_ack);
    cJSON_AddNumberToObject(data, "ack_timeout", config.ack_timeout);
    cJSON_AddBoolToObject(data, "enable_self_healing", config.enable_self_healing);
    cJSON_AddNumberToObject(data, "healing_timeout", config.healing_timeout);
    cJSON_AddStringToObject(data, "wifi_ssid", config.wifi_ssid);
    cJSON_AddBoolToObject(data, "enable_web_server", config.enable_web_server);
    cJSON_AddNumberToObject(data, "web_port", config.web_port);
    cJSON_AddBoolToObject(data, "enable_encryption", config.enable_encryption);
    cJSON_AddBoolToObject(data, "enable_crc", config.enable_crc);
    cJSON_AddBoolToObject(data, "enable_ldro", config.enable_ldro);
    cJSON_AddNumberToObject(data, "preamble_length", config.preamble_length);
    cJSON_AddNumberToObject(data, "symbol_timeout", config.symbol_timeout);
    
    return send_json_response(req, true, "Configuration loaded successfully", data);
}

// GET /api/nodes - Mesh nodes
static esp_err_t get_nodes_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /api/nodes");
    
    cJSON *nodes_array = cJSON_CreateArray();
    
    // Add self
    lora_config_t config;
    config_load(&config); // Ideally cache this or use g_config if global
    
    cJSON *self_node = cJSON_CreateObject();
    cJSON_AddNumberToObject(self_node, "id", config.node_id);
    cJSON_AddStringToObject(self_node, "name", config.node_name);
    cJSON_AddNumberToObject(self_node, "rssi", 0);
    cJSON_AddNumberToObject(self_node, "hops", 0);
    cJSON_AddStringToObject(self_node, "last_seen", "now");
    cJSON_AddBoolToObject(self_node, "online", true);
    cJSON_AddItemToArray(nodes_array, self_node);
    
    // Add other nodes from mesh manager
    int count = mesh_get_node_count();
    for (int i = 0; i < count; i++) {
        node_info_t *node = mesh_get_node_at_index(i);
        if (node) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddNumberToObject(item, "id", node->id);
            cJSON_AddStringToObject(item, "name", node->name);
            cJSON_AddNumberToObject(item, "rssi", node->rssi);
            cJSON_AddNumberToObject(item, "hops", node->hop_count);
            
            // Format time ago
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            uint32_t diff_sec = (now - node->last_seen) / 1000;
            char time_str[32];
            snprintf(time_str, sizeof(time_str), "%lus ago", diff_sec);
            
            cJSON_AddStringToObject(item, "last_seen", time_str);
            cJSON_AddBoolToObject(item, "online", node->online);
            cJSON_AddItemToArray(nodes_array, item);
        }
    }
    
    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "nodes", nodes_array);
    cJSON_AddNumberToObject(data, "total", cJSON_GetArraySize(nodes_array));
    cJSON_AddNumberToObject(data, "online", mesh_get_online_count() + 1); // +1 for self
    
    return send_json_response(req, true, "Nodes loaded", data);
}

// POST /api/ping
static esp_err_t post_ping_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "POST /api/ping");
    
    // Send broadcast ping
    mesh_send_ping();
    
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "action", "ping_sent");
    cJSON_AddNumberToObject(data, "timestamp", xTaskGetTickCount() * portTICK_PERIOD_MS);
    
    return send_json_response(req, true, "Ping sent to mesh", data);
}

// POST /api/discover
static esp_err_t post_discover_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "POST /api/discover");
    
    // Use ping for discovery for now
    mesh_send_ping();
    
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "action", "discovery_started");
    cJSON_AddNumberToObject(data, "timestamp", xTaskGetTickCount() * portTICK_PERIOD_MS);
    
    return send_json_response(req, true, "Network discovery initiated", data);
}

// GET /api/test - Test endpoint
static esp_err_t test_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /api/test");
    
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "status", "online");
    cJSON_AddNumberToObject(data, "uptime", xTaskGetTickCount() * portTICK_PERIOD_MS / 1000);
    cJSON_AddStringToObject(data, "version", "1.0.0");

    // Add node_id for UI header
    lora_config_t config;
    if (config_load(&config) == ESP_OK) {
        cJSON_AddNumberToObject(data, "node_id", config.node_id);
    } else {
        cJSON_AddNumberToObject(data, "node_id", 1); // Default
    }
    
    return send_json_response(req, true, "Web server is working", data);
}

// GET /favicon.ico - Empty favicon
static esp_err_t favicon_handler(httpd_req_t *req) {
    // Empty 1x1 pixel transparent PNG
    const unsigned char favicon[] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
        0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
        0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4, 0x89, 0x00, 0x00, 0x00,
        0x0A, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,
        0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x49,
        0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
    };
    
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon, sizeof(favicon));
    return ESP_OK;
}

// HTML/CSS/JS handlers
static esp_err_t dashboard_html_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    size_t html_len = dashboard_html_end - dashboard_html_start;
    httpd_resp_send(req, dashboard_html_start, html_len);
    return ESP_OK;
}

static esp_err_t lora_config_html_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    size_t html_len = lora_config_html_end - lora_config_html_start;
    httpd_resp_send(req, lora_config_html_start, html_len);
    return ESP_OK;
}

static esp_err_t devices_html_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    size_t html_len = devices_html_end - devices_html_start;
    httpd_resp_send(req, devices_html_start, html_len);
    return ESP_OK;
}

static esp_err_t styles_css_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/css");
    size_t css_len = styles_css_end - styles_css_start;
    httpd_resp_send(req, styles_css_start, css_len);
    return ESP_OK;
}

static esp_err_t script_js_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/javascript");
    size_t js_len = script_js_end - script_js_start;
    httpd_resp_send(req, script_js_start, js_len);
    return ESP_OK;
}
static esp_err_t post_config_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "POST /api/config - Saving configuration");
    
    // Get content length
    size_t content_len = req->content_len;
    if (content_len > 4096) {
        ESP_LOGE(TAG, "Request too large: %d bytes", content_len);
        return send_json_response(req, false, "Request too large", NULL);
    }
    
    // Allocate buffer
    char *content = malloc(content_len + 1);
    if (content == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory");
        return send_json_response(req, false, "Memory allocation failed", NULL);
    }
    
    // Receive data
    int ret = httpd_req_recv(req, content, content_len);
    if (ret <= 0) {
        free(content);
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        ESP_LOGE(TAG, "Failed to receive request body");
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    ESP_LOGI(TAG, "Received JSON: %s", content);
    
    // Parse JSON
    cJSON *root = cJSON_Parse(content);
    free(content);
    
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "JSON parse error: %s", error_ptr);
        }
        return send_json_response(req, false, "Invalid JSON format", NULL);
    }
    
    // Parse configuration from JSON
    lora_config_t new_config;
    
    // Initialize with default values first
    esp_err_t err = config_load(&new_config);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Could not load existing config, using defaults");
        // Set reasonable defaults
        memset(&new_config, 0, sizeof(lora_config_t));
        strcpy(new_config.node_name, "LoRa-Node");
        new_config.frequency = 868.0;
        new_config.spread_factor = 7;
        new_config.bandwidth = 125;
        new_config.coding_rate = 5;
        new_config.tx_power = 17;
        new_config.ping_interval = 60;
        new_config.beacon_interval = 30;
        new_config.web_port = 80;
    }
    
    // Update with new values from JSON
    cJSON *item;
    
    if ((item = cJSON_GetObjectItem(root, "node_id")) && cJSON_IsNumber(item)) {
        new_config.node_id = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "node_name")) && cJSON_IsString(item)) {
        strncpy(new_config.node_name, item->valuestring, sizeof(new_config.node_name) - 1);
        new_config.node_name[sizeof(new_config.node_name) - 1] = '\0';
    }
    
    if ((item = cJSON_GetObjectItem(root, "frequency")) && cJSON_IsNumber(item)) {
        new_config.frequency = item->valuedouble;
    }
    
    if ((item = cJSON_GetObjectItem(root, "spread_factor")) && cJSON_IsNumber(item)) {
        new_config.spread_factor = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "bandwidth")) && cJSON_IsNumber(item)) {
        new_config.bandwidth = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "coding_rate")) && cJSON_IsNumber(item)) {
        new_config.coding_rate = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "tx_power")) && cJSON_IsNumber(item)) {
        new_config.tx_power = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "sync_word")) && cJSON_IsNumber(item)) {
        new_config.sync_word = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "ping_interval")) && cJSON_IsNumber(item)) {
        new_config.ping_interval = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "beacon_interval")) && cJSON_IsNumber(item)) {
        new_config.beacon_interval = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "route_timeout")) && cJSON_IsNumber(item)) {
        new_config.route_timeout = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "max_hops")) && cJSON_IsNumber(item)) {
        new_config.max_hops = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "enable_ack")) && cJSON_IsBool(item)) {
        new_config.enable_ack = cJSON_IsTrue(item);
    }
    
    if ((item = cJSON_GetObjectItem(root, "ack_timeout")) && cJSON_IsNumber(item)) {
        new_config.ack_timeout = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "enable_self_healing")) && cJSON_IsBool(item)) {
        new_config.enable_self_healing = cJSON_IsTrue(item);
    }
    
    if ((item = cJSON_GetObjectItem(root, "healing_timeout")) && cJSON_IsNumber(item)) {
        new_config.healing_timeout = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "wifi_ssid")) && cJSON_IsString(item)) {
        strncpy(new_config.wifi_ssid, item->valuestring, sizeof(new_config.wifi_ssid) - 1);
        new_config.wifi_ssid[sizeof(new_config.wifi_ssid) - 1] = '\0';
    }
    
    if ((item = cJSON_GetObjectItem(root, "enable_web_server")) && cJSON_IsBool(item)) {
        new_config.enable_web_server = cJSON_IsTrue(item);
    }
    
    if ((item = cJSON_GetObjectItem(root, "web_port")) && cJSON_IsNumber(item)) {
        new_config.web_port = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "enable_encryption")) && cJSON_IsBool(item)) {
        new_config.enable_encryption = cJSON_IsTrue(item);
    }
    
    if ((item = cJSON_GetObjectItem(root, "enable_crc")) && cJSON_IsBool(item)) {
        new_config.enable_crc = cJSON_IsTrue(item);
    }
    
    if ((item = cJSON_GetObjectItem(root, "enable_ldro")) && cJSON_IsBool(item)) {
        new_config.enable_ldro = cJSON_IsTrue(item);
    }
    
    if ((item = cJSON_GetObjectItem(root, "preamble_length")) && cJSON_IsNumber(item)) {
        new_config.preamble_length = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "symbol_timeout")) && cJSON_IsNumber(item)) {
        new_config.symbol_timeout = item->valueint;
    }
    
    cJSON_Delete(root);
    
    // Save to NVS
    ESP_LOGI(TAG, "Saving config to NVS: node_id=%d, name=%s", 
             new_config.node_id, new_config.node_name);
    
    err = config_save(&new_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save config to NVS: %s", esp_err_to_name(err));
        return send_json_response(req, false, "Failed to save configuration to storage", NULL);
    }
    
    ESP_LOGI(TAG, "Configuration saved successfully");
    
    // Return success response
    cJSON *response_data = cJSON_CreateObject();
    cJSON_AddNumberToObject(response_data, "saved_at", xTaskGetTickCount() * portTICK_PERIOD_MS);
    cJSON_AddStringToObject(response_data, "node_name", new_config.node_name);
    
    return send_json_response(req, true, "Configuration saved successfully", response_data);
}
static esp_err_t get_devices_api_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /devices API called");
    
    // Return JSON response instead of HTML
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "success");
    cJSON_AddStringToObject(response, "message", "Devices API endpoint");
    
    char *json_str = cJSON_PrintUnformatted(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free(json_str);
    cJSON_Delete(response);
    return ESP_OK;
}

static esp_err_t post_config_direct_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "POST /config - Saving configuration");
    
   // Get content length
    size_t content_len = req->content_len;
    if (content_len > 4096) {
        ESP_LOGE(TAG, "Request too large: %d bytes", content_len);
        return send_json_response(req, false, "Request too large", NULL);
    }
    
    // Allocate buffer
    char *content = malloc(content_len + 1);
    if (content == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory");
        return send_json_response(req, false, "Memory allocation failed", NULL);
    }
    
    // Receive data
    int ret = httpd_req_recv(req, content, content_len);
    if (ret <= 0) {
        free(content);
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        ESP_LOGE(TAG, "Failed to receive request body");
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    ESP_LOGI(TAG, "Received JSON: %s", content);
    
    // Parse JSON
    cJSON *root = cJSON_Parse(content);
    free(content);
    
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "JSON parse error: %s", error_ptr);
        }
        return send_json_response(req, false, "Invalid JSON format", NULL);
    }
    
    // Parse configuration from JSON
    lora_config_t new_config;
    
    // Initialize with default values first
    esp_err_t err = config_load(&new_config);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Could not load existing config, using defaults");
        // Set reasonable defaults
        memset(&new_config, 0, sizeof(lora_config_t));
        strcpy(new_config.node_name, "LoRa-Node");
        new_config.frequency = 868.0;
        new_config.spread_factor = 7;
        new_config.bandwidth = 125;
        new_config.coding_rate = 5;
        new_config.tx_power = 17;
        new_config.ping_interval = 60;
        new_config.beacon_interval = 30;
        new_config.web_port = 80;
    }
    
    // Update with new values from JSON
    cJSON *item;
    
    if ((item = cJSON_GetObjectItem(root, "node_id")) && cJSON_IsNumber(item)) {
        new_config.node_id = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "node_name")) && cJSON_IsString(item)) {
        strncpy(new_config.node_name, item->valuestring, sizeof(new_config.node_name) - 1);
        new_config.node_name[sizeof(new_config.node_name) - 1] = '\0';
    }
    
    if ((item = cJSON_GetObjectItem(root, "frequency")) && cJSON_IsNumber(item)) {
        new_config.frequency = item->valuedouble;
    }
    
    if ((item = cJSON_GetObjectItem(root, "spread_factor")) && cJSON_IsNumber(item)) {
        new_config.spread_factor = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "bandwidth")) && cJSON_IsNumber(item)) {
        new_config.bandwidth = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "coding_rate")) && cJSON_IsNumber(item)) {
        new_config.coding_rate = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "tx_power")) && cJSON_IsNumber(item)) {
        new_config.tx_power = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "sync_word")) && cJSON_IsNumber(item)) {
        new_config.sync_word = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "ping_interval")) && cJSON_IsNumber(item)) {
        new_config.ping_interval = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "beacon_interval")) && cJSON_IsNumber(item)) {
        new_config.beacon_interval = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "route_timeout")) && cJSON_IsNumber(item)) {
        new_config.route_timeout = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "max_hops")) && cJSON_IsNumber(item)) {
        new_config.max_hops = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "enable_ack")) && cJSON_IsBool(item)) {
        new_config.enable_ack = cJSON_IsTrue(item);
    }
    
    if ((item = cJSON_GetObjectItem(root, "ack_timeout")) && cJSON_IsNumber(item)) {
        new_config.ack_timeout = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "enable_self_healing")) && cJSON_IsBool(item)) {
        new_config.enable_self_healing = cJSON_IsTrue(item);
    }
    
    if ((item = cJSON_GetObjectItem(root, "healing_timeout")) && cJSON_IsNumber(item)) {
        new_config.healing_timeout = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "wifi_ssid")) && cJSON_IsString(item)) {
        strncpy(new_config.wifi_ssid, item->valuestring, sizeof(new_config.wifi_ssid) - 1);
        new_config.wifi_ssid[sizeof(new_config.wifi_ssid) - 1] = '\0';
    }
    
    if ((item = cJSON_GetObjectItem(root, "enable_web_server")) && cJSON_IsBool(item)) {
        new_config.enable_web_server = cJSON_IsTrue(item);
    }
    
    if ((item = cJSON_GetObjectItem(root, "web_port")) && cJSON_IsNumber(item)) {
        new_config.web_port = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "enable_encryption")) && cJSON_IsBool(item)) {
        new_config.enable_encryption = cJSON_IsTrue(item);
    }
    
    if ((item = cJSON_GetObjectItem(root, "enable_crc")) && cJSON_IsBool(item)) {
        new_config.enable_crc = cJSON_IsTrue(item);
    }
    
    if ((item = cJSON_GetObjectItem(root, "enable_ldro")) && cJSON_IsBool(item)) {
        new_config.enable_ldro = cJSON_IsTrue(item);
    }
    
    if ((item = cJSON_GetObjectItem(root, "preamble_length")) && cJSON_IsNumber(item)) {
        new_config.preamble_length = item->valueint;
    }
    
    if ((item = cJSON_GetObjectItem(root, "symbol_timeout")) && cJSON_IsNumber(item)) {
        new_config.symbol_timeout = item->valueint;
    }
    
    cJSON_Delete(root);
    
    // Save to NVS
    ESP_LOGI(TAG, "Saving config to NVS: node_id=%d, name=%s", 
             new_config.node_id, new_config.node_name);
    
    err = config_save(&new_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save config to NVS: %s", esp_err_to_name(err));
        return send_json_response(req, false, "Failed to save configuration to storage", NULL);
    }
    
    ESP_LOGI(TAG, "Configuration saved successfully");
    
    // Return success response
    cJSON *response_data = cJSON_CreateObject();
    cJSON_AddNumberToObject(response_data, "saved_at", xTaskGetTickCount() * portTICK_PERIOD_MS);
    cJSON_AddStringToObject(response_data, "node_name", new_config.node_name);
    
    return send_json_response(req, true, "Configuration saved successfully", response_data);
}
static const httpd_uri_t post_config_direct_uri = {
    .uri       = "/config",
    .method    = HTTP_POST,
    .handler   = post_config_direct_handler,
    .user_ctx  = NULL
};
// Register this GET handler
static const httpd_uri_t get_config_api_uri = {
    .uri       = "/api/config",
    .method    = HTTP_GET,
    .handler   = get_config_api_handler,
    .user_ctx  = NULL
};
static const httpd_uri_t devices_api_uri = {
    .uri       = "/devices",  // API endpoint (no .html)
    .method    = HTTP_GET,
    .handler   = get_devices_api_handler,
    .user_ctx  = NULL
};
// URI definitions
static const httpd_uri_t dashboard_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = dashboard_html_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t dashboard_html_uri = {
    .uri       = "/dashboard.html",
    .method    = HTTP_GET,
    .handler   = dashboard_html_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t lora_config_uri = {
    .uri       = "/lora-config.html",
    .method    = HTTP_GET,
    .handler   = lora_config_html_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t devices_uri = {
    .uri       = "/devices.html",
    .method    = HTTP_GET,
    .handler   = devices_html_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t styles_uri = {
    .uri       = "/styles.css",
    .method    = HTTP_GET,
    .handler   = styles_css_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t script_uri = {
    .uri       = "/script.js",
    .method    = HTTP_GET,
    .handler   = script_js_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t favicon_uri = {
    .uri       = "/favicon.ico",
    .method    = HTTP_GET,
    .handler   = favicon_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t get_config_uri = {
    .uri       = "/config",
    .method    = HTTP_GET,
    .handler   = config_redirect_handler,      // Returns JSON
    .user_ctx  = NULL
};

static const httpd_uri_t get_nodes_uri = {
    .uri       = "/api/nodes",
    .method    = HTTP_GET,
    .handler   = get_nodes_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t post_ping_uri = {
    .uri       = "/api/ping",
    .method    = HTTP_POST,
    .handler   = post_ping_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t post_discover_uri = {
    .uri       = "/api/discover",
    .method    = HTTP_POST,
    .handler   = post_discover_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t test_uri = {
    .uri       = "/api/test",
    .method    = HTTP_GET,
    .handler   = test_handler,
    .user_ctx  = NULL
};
static const httpd_uri_t post_config_uri = {
    .uri       = "/api/config",  // Should match your frontend AJAX calls
    .method    = HTTP_POST,
    .handler   = post_config_handler,  // The missing handler above
    .user_ctx  = NULL
};



// Start web server
httpd_handle_t web_server_start(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    ESP_LOGI(TAG, "=== Starting Web Server ===");
    
    // Load configuration
    lora_config_t current_config;
    esp_err_t err = config_load(&current_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load configuration");
        return NULL;
    }
    
    // Check if web server is enabled
    if (!current_config.enable_web_server) {
        ESP_LOGW(TAG, "Web server is disabled in configuration");
        return NULL;
    }
    
    // Set port
    config.server_port = current_config.web_port;
    config.stack_size = 8192;
    config.max_uri_handlers = 20;
    config.backlog_conn = 5;
    
    ESP_LOGI(TAG, "Starting HTTP server on port %d", config.server_port);
    
    // Start server
    err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(err));
        return NULL;
    }
    
    // Register all URI handlers
    httpd_register_uri_handler(server, &dashboard_uri);
    httpd_register_uri_handler(server, &dashboard_html_uri);
    httpd_register_uri_handler(server, &get_config_uri);
    httpd_register_uri_handler(server, &lora_config_uri);    
    httpd_register_uri_handler(server, &devices_uri);
    httpd_register_uri_handler(server, &styles_uri);
    httpd_register_uri_handler(server, &script_uri);
    httpd_register_uri_handler(server, &favicon_uri);
    
    httpd_register_uri_handler(server, &get_nodes_uri);
    httpd_register_uri_handler(server, &post_ping_uri);
    httpd_register_uri_handler(server, &post_discover_uri);
    httpd_register_uri_handler(server, &test_uri);
    httpd_register_uri_handler(server, &post_config_uri);  
    httpd_register_uri_handler(server, &devices_api_uri);
    httpd_register_uri_handler(server, &get_config_api_uri);
    httpd_register_uri_handler(server, &post_config_direct_uri);

    ESP_LOGI(TAG, "Web server started successfully on port %d", config.server_port);
    ESP_LOGI(TAG, "Dashboard: http://192.168.4.1");
    ESP_LOGI(TAG, "Config: http://192.168.4.1/lora-config.html");
    ESP_LOGI(TAG, "=== Registered URI Handlers ===");
    ESP_LOGI(TAG, "1. /              -> dashboard_html_handler (HTML)");
    ESP_LOGI(TAG, "2. /dashboard.html -> dashboard_html_handler (HTML)");
    ESP_LOGI(TAG, "3. /config        -> get_config_handler (JSON API)");
    ESP_LOGI(TAG, "4. /lora-config.html -> lora_config_html_handler (HTML)");
    ESP_LOGI(TAG, "5. /devices.html  -> devices_html_handler (HTML)");
    ESP_LOGI(TAG, "6. /devices       -> get_devices_api_handler (JSON API)");
    ESP_LOGI(TAG, "7. /styles.css    -> styles_css_handler (CSS)");
    ESP_LOGI(TAG, "8. /script.js     -> script_js_handler (JS)");
    ESP_LOGI(TAG, "9. /favicon.ico   -> favicon_handler");
    ESP_LOGI(TAG, "10. /api/nodes    -> get_nodes_handler (JSON)");
    ESP_LOGI(TAG, "11. /api/ping     -> post_ping_handler (JSON)");
    ESP_LOGI(TAG, "12. /api/discover -> post_discover_handler (JSON)");
    ESP_LOGI(TAG, "13. /api/test     -> test_handler (JSON)");
    ESP_LOGI(TAG, "14. /api/config   -> post_config_handler (JSON POST)");
    return server;
}

// Stop web server
void web_server_stop(httpd_handle_t server) {
    if (server) {
        httpd_stop(server);
        ESP_LOGI(TAG, "Web server stopped");
    }
}