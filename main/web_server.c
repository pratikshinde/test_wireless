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

// GET /config - Configuration endpoint
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

// GET /api/nodes - Mesh nodes
static esp_err_t get_nodes_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /api/nodes");
    
    // Create sample nodes for testing
    cJSON *nodes_array = cJSON_CreateArray();
    
    // Add self
    cJSON *self_node = cJSON_CreateObject();
    cJSON_AddNumberToObject(self_node, "id", 1);
    cJSON_AddStringToObject(self_node, "name", "Master");
    cJSON_AddNumberToObject(self_node, "rssi", 0);
    cJSON_AddNumberToObject(self_node, "hops", 0);
    cJSON_AddStringToObject(self_node, "last_seen", "now");
    cJSON_AddBoolToObject(self_node, "online", true);
    cJSON_AddItemToArray(nodes_array, self_node);
    
    // Add sample nodes
    cJSON *node2 = cJSON_CreateObject();
    cJSON_AddNumberToObject(node2, "id", 2);
    cJSON_AddStringToObject(node2, "name", "Node-2");
    cJSON_AddNumberToObject(node2, "rssi", -65);
    cJSON_AddNumberToObject(node2, "hops", 1);
    cJSON_AddStringToObject(node2, "last_seen", "30s ago");
    cJSON_AddBoolToObject(node2, "online", true);
    cJSON_AddItemToArray(nodes_array, node2);
    
    cJSON *node3 = cJSON_CreateObject();
    cJSON_AddNumberToObject(node3, "id", 3);
    cJSON_AddStringToObject(node3, "name", "Node-3");
    cJSON_AddNumberToObject(node3, "rssi", -72);
    cJSON_AddNumberToObject(node3, "hops", 2);
    cJSON_AddStringToObject(node3, "last_seen", "1m ago");
    cJSON_AddBoolToObject(node3, "online", true);
    cJSON_AddItemToArray(nodes_array, node3);
    
    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "nodes", nodes_array);
    cJSON_AddNumberToObject(data, "total", 3);
    cJSON_AddNumberToObject(data, "online", 3);
    
    return send_json_response(req, true, "Nodes loaded", data);
}

// POST /api/ping
static esp_err_t post_ping_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "POST /api/ping");
    
    // In real implementation, send actual ping via LoRa
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "action", "ping_sent");
    cJSON_AddNumberToObject(data, "timestamp", xTaskGetTickCount() * portTICK_PERIOD_MS);
    
    return send_json_response(req, true, "Ping sent to network", data);
}

// POST /api/discover
static esp_err_t post_discover_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "POST /api/discover");
    
    // In real implementation, trigger network discovery
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
    .handler   = get_config_handler,
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
    httpd_register_uri_handler(server, &lora_config_uri);
    httpd_register_uri_handler(server, &devices_uri);
    httpd_register_uri_handler(server, &styles_uri);
    httpd_register_uri_handler(server, &script_uri);
    httpd_register_uri_handler(server, &favicon_uri);
    httpd_register_uri_handler(server, &get_config_uri);
    httpd_register_uri_handler(server, &get_nodes_uri);
    httpd_register_uri_handler(server, &post_ping_uri);
    httpd_register_uri_handler(server, &post_discover_uri);
    httpd_register_uri_handler(server, &test_uri);
    
    ESP_LOGI(TAG, "Web server started successfully on port %d", config.server_port);
    ESP_LOGI(TAG, "Dashboard: http://192.168.4.1");
    ESP_LOGI(TAG, "Config: http://192.168.4.1/lora-config.html");
    
    return server;
}

// Stop web server
void web_server_stop(httpd_handle_t server) {
    if (server) {
        httpd_stop(server);
        ESP_LOGI(TAG, "Web server stopped");
    }
}