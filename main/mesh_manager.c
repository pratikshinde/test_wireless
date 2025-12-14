// main/mesh_manager.c - COMPLETE VERSION
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mesh_manager.h"

static const char *TAG = "MESH_MGR";

#define MAX_NODES 50
#define MAX_ROUTES 100

static node_info_t nodes[MAX_NODES];
static route_info_t routes[MAX_ROUTES];
static uint8_t node_count = 0;
static uint8_t route_count = 0;
static uint32_t self_uptime = 0;

void mesh_init(uint8_t node_id) {
    ESP_LOGI(TAG, "Mesh manager initialized with node ID: %d", node_id);
    self_uptime = 0;
}

void mesh_add_or_update_node(uint8_t id, const char *name, int16_t rssi, uint8_t hop_count) {
    // Check if node already exists
    for (int i = 0; i < node_count; i++) {
        if (nodes[i].id == id) {
            nodes[i].rssi = rssi;
            nodes[i].hop_count = hop_count;
            nodes[i].last_seen = xTaskGetTickCount() * portTICK_PERIOD_MS;
            nodes[i].online = true;
            if (name) {
                strncpy(nodes[i].name, name, sizeof(nodes[i].name) - 1);
                nodes[i].name[sizeof(nodes[i].name) - 1] = '\0';
            }
            return;
        }
    }
    
    // Add new node
    if (node_count < MAX_NODES) {
        nodes[node_count].id = id;
        if (name) {
            strncpy(nodes[node_count].name, name, sizeof(nodes[node_count].name) - 1);
            nodes[node_count].name[sizeof(nodes[node_count].name) - 1] = '\0';
        } else {
            snprintf(nodes[node_count].name, sizeof(nodes[node_count].name), "Node-%d", id);
        }
        nodes[node_count].rssi = rssi;
        nodes[node_count].hop_count = hop_count;
        nodes[node_count].last_seen = xTaskGetTickCount() * portTICK_PERIOD_MS;
        nodes[node_count].online = true;
        node_count++;
    }
}

node_info_t *mesh_find_node(uint8_t id) {
    for (int i = 0; i < node_count; i++) {
        if (nodes[i].id == id) {
            return &nodes[i];
        }
    }
    return NULL;
}

void mesh_update_node_rssi(uint8_t id, int16_t rssi) {
    node_info_t *node = mesh_find_node(id);
    if (node) {
        node->rssi = rssi;
    }
}

void mesh_update_node_seen(uint8_t id) {
    node_info_t *node = mesh_find_node(id);
    if (node) {
        node->last_seen = xTaskGetTickCount() * portTICK_PERIOD_MS;
        node->online = true;
    }
}

void mesh_clear_offline_nodes(uint32_t timeout_seconds) {
    uint32_t timeout_ms = timeout_seconds * 1000;
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    for (int i = 0; i < node_count; i++) {
        if ((now - nodes[i].last_seen) > timeout_ms) {
            nodes[i].online = false;
        }
    }
}

void mesh_add_or_update_route(uint8_t dest, uint8_t next_hop, uint8_t hop_count, int16_t link_quality) {
    // Check if route already exists
    for (int i = 0; i < route_count; i++) {
        if (routes[i].dest_id == dest) {
            routes[i].next_hop = next_hop;
            routes[i].hop_count = hop_count;
            routes[i].link_quality = link_quality;
            routes[i].last_updated = xTaskGetTickCount() * portTICK_PERIOD_MS;
            routes[i].active = true;
            return;
        }
    }
    
    // Add new route
    if (route_count < MAX_ROUTES) {
        routes[route_count].dest_id = dest;
        routes[route_count].next_hop = next_hop;
        routes[route_count].hop_count = hop_count;
        routes[route_count].link_quality = link_quality;
        routes[route_count].last_updated = xTaskGetTickCount() * portTICK_PERIOD_MS;
        routes[route_count].active = true;
        route_count++;
    }
}

route_info_t *mesh_get_route(uint8_t index) {
    if (index < route_count) {
        return &routes[index];
    }
    return NULL;
}

void mesh_clear_expired_routes(uint32_t timeout_seconds) {
    uint32_t timeout_ms = timeout_seconds * 1000;
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    for (int i = 0; i < route_count; i++) {
        if ((now - routes[i].last_updated) > timeout_ms) {
            routes[i].active = false;
        }
    }
}

uint8_t mesh_get_node_count(void) {
    return node_count;
}

uint8_t mesh_get_route_count(void) {
    return route_count;
}

uint8_t mesh_get_online_count(void) {
    uint8_t count = 0;
    for (int i = 0; i < node_count; i++) {
        if (nodes[i].online) {
            count++;
        }
    }
    return count;
}

uint8_t mesh_get_next_hop(uint8_t dest_id) {
    for (int i = 0; i < route_count; i++) {
        if (routes[i].dest_id == dest_id && routes[i].active) {
            return routes[i].next_hop;
        }
    }
    return 0;
}

// ADD THIS FUNCTION
void mesh_update_self_uptime(void) {
    self_uptime++;
    
    // Log every 5 minutes
    if (self_uptime % 300 == 0) {
        ESP_LOGD(TAG, "Node uptime: %lu seconds", self_uptime);
    }
}

// Optional: Add this if needed
uint32_t mesh_get_self_uptime(void) {
    return self_uptime;
}