#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "WEB_SERVER";
static httpd_handle_t server = NULL;
static lora_config_t *web_config;

// Embedded HTML pages - directly in flash
extern const char dashboard_html_start[] asm("_binary_dashboard_html_start");
extern const char dashboard_html_end[]   asm("_binary_dashboard_html_end");
extern const char lora_config_html_start[] asm("_binary_lora_config_html_start");
extern const char lora_config_html_end[]   asm("_binary_lora_config_html_end");
extern const char devices_html_start[] asm("_binary_devices_html_start");
extern const char devices_html_end[]   asm("_binary_devices_html_end");
extern const char styles_css_start[] asm("_binary_styles_css_start");
extern const char styles_css_end[]   asm("_binary_styles_css_end");
extern const char script_js_start[] asm("_binary_script_js_start");
extern const char script_js_end[]   asm("_binary_script_js_end");

// Embedded favicon (1x1 transparent PNG)
static const unsigned char favicon_png[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4, 0x89, 0x00, 0x00, 0x00,
    0x0A, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,
    0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x49,
    0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};

// HTTP request handlers for embedded content
static esp_err_t dashboard_handler(httpd_req_t *req) {
    size_t html_len = dashboard_html_end - dashboard_html_start;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_send(req, (const char*)dashboard_html_start, html_len);
    ESP_LOGD(TAG, "Served dashboard.html (%d bytes)", html_len);
    return ESP_OK;
}

static esp_err_t lora_config_handler(httpd_req_t *req) {
    size_t html_len = lora_config_html_end - lora_config_html_start;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_send(req, (const char*)lora_config_html_start, html_len);
    return ESP_OK;
}

static esp_err_t devices_handler(httpd_req_t *req) {
    size_t html_len = devices_html_end - devices_html_start;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_send(req, (const char*)devices_html_start, html_len);
    return ESP_OK;
}

static esp_err_t styles_handler(httpd_req_t *req) {
    size_t css_len = styles_css_end - styles_css_start;
    httpd_resp_set_type(req, "text/css");
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=86400");
    httpd_resp_send(req, (const char*)styles_css_start, css_len);
    return ESP_OK;
}

static esp_err_t script_handler(httpd_req_t *req) {
    size_t js_len = script_js_end - script_js_start;
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=86400");
    httpd_resp_send(req, (const char*)script_js_start, js_len);
    return ESP_OK;
}

static esp_err_t favicon_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, (const char*)favicon_png, sizeof(favicon_png));
    return ESP_OK;
}

// API Handlers
static esp_err_t api_config_get_handler(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "node_id", web_config->node_id);
    cJSON_AddStringToObject(root, "node_name", web_config->node_name);
    cJSON_AddNumberToObject(root, "frequency", web_config->frequency);
    cJSON_AddNumberToObject(root, "spreading_factor", web_config->spreading_factor);
    cJSON_AddNumberToObject(root, "bandwidth", web_config->bandwidth);
    cJSON_AddNumberToObject(root, "coding_rate", web_config->coding_rate);
    cJSON_AddNumberToObject(root, "tx_power", web_config->tx_power);
    cJSON_AddNumberToObject(root, "sync_word", web_config->sync_word);
    cJSON_AddNumberToObject(root, "ping_interval", web_config->ping_interval);
    cJSON_AddNumberToObject(root, "beacon_interval", web_config->beacon_interval);
    cJSON_AddNumberToObject(root, "route_timeout", web_config->route_timeout);
    cJSON_AddNumberToObject(root, "max_hops", web_config->max_hops);
    cJSON_AddBoolToObject(root, "enable_ack", web_config->enable_ack);
    cJSON_AddNumberToObject(root, "ack_timeout", web_config->ack_timeout);
    cJSON_AddBoolToObject(root, "enable_self_healing", web_config->enable_self_healing);
    cJSON_AddNumberToObject(root, "healing_timeout", web_config->healing_timeout);
    cJSON_AddBoolToObject(root, "enable_encryption", web_config->enable_encryption);
    cJSON_AddBoolToObject(root, "enable_crc", web_config->enable_crc);
    cJSON_AddBoolToObject(root, "enable_ldro", web_config->enable_ldro);
    cJSON_AddNumberToObject(root, "preamble_length", web_config->preamble_length);
    cJSON_AddNumberToObject(root, "symbol_timeout", web_config->symbol_timeout);
    
    const char *json_str = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    cJSON_Delete(root);
    free((void*)json_str);
    return ESP_OK;
}

static esp_err_t api_config_post_handler(httpd_req_t *req) {
    char buf[1024];
    int ret = httpd_req_recv(req, buf, sizeof(buf)-1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';
    
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    // Update configuration from JSON
    cJSON *item;
    if ((item = cJSON_GetObjectItem(root, "node_id"))) web_config->node_id = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "node_name"))) strncpy(web_config->node_name, item->valuestring, sizeof(web_config->node_name)-1);
    if ((item = cJSON_GetObjectItem(root, "frequency"))) web_config->frequency = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "spreading_factor"))) web_config->spreading_factor = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "bandwidth"))) web_config->bandwidth = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "coding_rate"))) web_config->coding_rate = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "tx_power"))) web_config->tx_power = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "sync_word"))) web_config->sync_word = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "ping_interval"))) web_config->ping_interval = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "beacon_interval"))) web_config->beacon_interval = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "route_timeout"))) web_config->route_timeout = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "max_hops"))) web_config->max_hops = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "enable_ack"))) web_config->enable_ack = cJSON_IsTrue(item);
    if ((item = cJSON_GetObjectItem(root, "ack_timeout"))) web_config->ack_timeout = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "enable_self_healing"))) web_config->enable_self_healing = cJSON_IsTrue(item);
    if ((item = cJSON_GetObjectItem(root, "healing_timeout"))) web_config->healing_timeout = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "enable_encryption"))) web_config->enable_encryption = cJSON_IsTrue(item);
    if ((item = cJSON_GetObjectItem(root, "enable_crc"))) web_config->enable_crc = cJSON_IsTrue(item);
    if ((item = cJSON_GetObjectItem(root, "enable_ldro"))) web_config->enable_ldro = cJSON_IsTrue(item);
    if ((item = cJSON_GetObjectItem(root, "preamble_length"))) web_config->preamble_length = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "symbol_timeout"))) web_config->symbol_timeout = item->valueint;
    
    // Save to NVS
    if (config_save(web_config)) {
        cJSON *response = cJSON_CreateObject();
        cJSON_AddBoolToObject(response, "success", true);
        const char *json_str = cJSON_Print(response);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json_str, strlen(json_str));
        cJSON_Delete(response);
        free((void*)json_str);
        ESP_LOGI(TAG, "Configuration updated via web");
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save config");
    }
    
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t api_nodes_handler(httpd_req_t *req) {
    cJSON *root = cJSON_CreateArray();
    
    // Get nodes from config_manager
    for (int i = 0; i < mesh_get_node_count(); i++) {
        node_info_t *node = mesh_get_node(i);
        if (!node) continue;
        
        cJSON *node_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(node_obj, "id", node->id);
        cJSON_AddStringToObject(node_obj, "name", node->name);
        cJSON_AddNumberToObject(node_obj, "rssi", node->rssi);
        cJSON_AddNumberToObject(node_obj, "hop_count", node->hop_count);
        
        // Convert microseconds to seconds for web display
        cJSON_AddNumberToObject(node_obj, "uptime", node->uptime / 1000000);
        
        // Calculate seconds since last seen
        int64_t now_us = esp_timer_get_time();
        int64_t last_seen_sec = (now_us - node->last_seen) / 1000000;
        cJSON_AddNumberToObject(node_obj, "last_seen", last_seen_sec);
        
        cJSON_AddBoolToObject(node_obj, "online", node->online);
        cJSON_AddItemToArray(root, node_obj);
    }
    
    const char *json_str = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    cJSON_Delete(root);
    free((void*)json_str);
    return ESP_OK;
}

// In api_routes_handler
static esp_err_t api_routes_handler(httpd_req_t *req) {
    cJSON *root = cJSON_CreateArray();
    
    // Get routes from config_manager
    for (int i = 0; i < mesh_get_route_count(); i++) {
        route_info_t *route = mesh_get_route(i);
        if (!route) continue;
        
        cJSON *route_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(route_obj, "destination", route->destination);
        cJSON_AddNumberToObject(route_obj, "next_hop", route->next_hop);
        cJSON_AddNumberToObject(route_obj, "hop_count", route->hop_count);
        cJSON_AddNumberToObject(route_obj, "link_quality", route->link_quality);
        
        // Convert microseconds to seconds for web display
        int64_t now_us = esp_timer_get_time();
        int64_t last_update_sec = (now_us - route->last_update) / 1000000;
        cJSON_AddNumberToObject(route_obj, "last_update", last_update_sec);
        
        cJSON_AddBoolToObject(route_obj, "active", route->active);
        cJSON_AddItemToArray(root, route_obj);
    }
    
    const char *json_str = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    cJSON_Delete(root);
    free((void*)json_str);
    return ESP_OK;
}
// Start web server with embedded content
void web_server_start(lora_config_t *config) {
    web_config = config;
    
    if (!config->enable_web_server) {
        ESP_LOGI(TAG, "Web server disabled");
        return;
    }
    
    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    server_config.server_port = config->web_port;
    server_config.max_uri_handlers = 15;
    server_config.lru_purge_enable = true;
    
    if (httpd_start(&server, &server_config) == ESP_OK) {
        // Register embedded HTML handlers
        httpd_uri_t dashboard = {.uri = "/", .method = HTTP_GET, .handler = dashboard_handler};
        httpd_register_uri_handler(server, &dashboard);
        
        httpd_uri_t config_page = {.uri = "/config", .method = HTTP_GET, .handler = lora_config_handler};
        httpd_register_uri_handler(server, &config_page);
        
        httpd_uri_t devices_page = {.uri = "/devices", .method = HTTP_GET, .handler = devices_handler};
        httpd_register_uri_handler(server, &devices_page);
        
        httpd_uri_t styles = {.uri = "/styles.css", .method = HTTP_GET, .handler = styles_handler};
        httpd_register_uri_handler(server, &styles);
        
        httpd_uri_t script = {.uri = "/script.js", .method = HTTP_GET, .handler = script_handler};
        httpd_register_uri_handler(server, &script);
        
        httpd_uri_t favicon = {.uri = "/favicon.ico", .method = HTTP_GET, .handler = favicon_handler};
        httpd_register_uri_handler(server, &favicon);
        
        // Register API handlers
        httpd_uri_t api_config_get = {.uri = "/api/config", .method = HTTP_GET, .handler = api_config_get_handler};
        httpd_register_uri_handler(server, &api_config_get);
        
        httpd_uri_t api_config_post = {.uri = "/api/config", .method = HTTP_POST, .handler = api_config_post_handler};
        httpd_register_uri_handler(server, &api_config_post);
        
        httpd_uri_t api_nodes = {.uri = "/api/nodes", .method = HTTP_GET, .handler = api_nodes_handler};
        httpd_register_uri_handler(server, &api_nodes);
        
        httpd_uri_t api_routes = {.uri = "/api/routes", .method = HTTP_GET, .handler = api_routes_handler};
        httpd_register_uri_handler(server, &api_routes);
        
        ESP_LOGI(TAG, "Web server started on port %d", config->web_port);
        ESP_LOGI(TAG, "Serving embedded HTML/CSS/JS from flash");
    } else {
        ESP_LOGE(TAG, "Failed to start web server");
    }
}

void web_server_stop(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
        ESP_LOGI(TAG, "Web server stopped");
    }
}