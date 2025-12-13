#ifndef LORA_MESH_H
#define LORA_MESH_H

#include <stdint.h>
#include <stdbool.h>
#include "config_manager.h"
#include "lora_packet.h"

// Function prototypes
bool lora_mesh_init(lora_config_t *config);
void lora_mesh_task(void *pvParameters);
void wifi_init_softap(lora_config_t *config);

bool lora_send_packet(uint8_t dest, uint8_t *data, uint8_t len, msg_type_t type);
bool lora_broadcast_packet(uint8_t *data, uint8_t len, msg_type_t type);

void mesh_update_routing_table(uint8_t node_id, int16_t rssi, uint8_t hop_count);
void mesh_self_healing(void);
void mesh_send_beacon(void);  // Add this line
void mesh_send_ping(void);

uint8_t mesh_get_online_nodes(void);
uint8_t mesh_get_active_routes(void);

#endif // LORA_MESH_H