// main/mesh_manager.h - VERIFY THIS IS CORRECT
#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "mesh_types.h"

// Function declarations
void mesh_init(uint8_t node_id);
void mesh_add_or_update_node(uint8_t id, const char *name, int16_t rssi, uint8_t hop_count);
node_info_t *mesh_find_node(uint8_t id);
void mesh_update_node_rssi(uint8_t id, int16_t rssi);
void mesh_update_node_seen(uint8_t id);
void mesh_clear_offline_nodes(uint32_t timeout_seconds);

void mesh_add_or_update_route(uint8_t dest, uint8_t next_hop, uint8_t hop_count, int16_t link_quality);
route_info_t *mesh_get_route(uint8_t index);
void mesh_clear_expired_routes(uint32_t timeout_seconds);

uint8_t mesh_get_node_count(void);
uint8_t mesh_get_route_count(void);
uint8_t mesh_get_online_count(void);
uint8_t mesh_get_next_hop(uint8_t dest_id);

#endif // MESH_MANAGER_H