#ifndef LORA_MESH_H
#define LORA_MESH_H

#include <stdint.h>
#include <stdbool.h>
#include "config_manager.h"

// Message types
typedef enum {
    MSG_DATA = 0x01,
    MSG_PING = 0x02,
    MSG_PONG = 0x03,
    MSG_BEACON = 0x04,
    MSG_ROUTE_DISCOVERY = 0x05,
    MSG_ROUTE_REPLY = 0x06,
    MSG_CONFIG_SYNC = 0x07
} msg_type_t;

// Function prototypes
bool lora_mesh_init(lora_config_t *config);
void lora_mesh_task(void *pvParameters);
void wifi_init_softap(lora_config_t *config);

bool lora_send_packet(uint8_t dest, uint8_t *data, uint8_t len, msg_type_t type);
bool lora_broadcast_packet(uint8_t *data, uint8_t len, msg_type_t type);

void mesh_update_routing_table(uint8_t node_id, int16_t rssi, uint8_t hop_count);
void mesh_self_healing(void);
void mesh_send_beacon(void);
void mesh_send_ping(void);

uint8_t mesh_get_online_nodes(void);
uint8_t mesh_get_active_routes(void);

#endif // LORA_MESH_H