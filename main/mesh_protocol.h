#ifndef MESH_PROTOCOL_H
#define MESH_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "lora_packet.h"

// Remove forward declaration - include the actual header
#include "config_manager.h"

// In mesh_protocol.h, adjust structures:
typedef struct {
    uint8_t node_id;
    char node_name[32];
    uint16_t sequence;
    int16_t rssi;
    uint8_t hop_count;
    uint8_t padding[7];  // Add padding to make 48 bytes (16 * 3)
} beacon_packet_t;

typedef struct {
    uint8_t node_id;
    uint16_t sequence;
    uint32_t timestamp_ms;
    uint8_t padding[9];  // Add padding to make 16 bytes
} ping_packet_t;

typedef struct {
    uint8_t node_id;
    uint16_t ping_sequence;
    uint32_t timestamp_ms;
    uint32_t rtt_ms;
} pong_packet_t;

typedef struct {
    uint8_t src;
    uint8_t dest;
    uint16_t sequence;
    uint8_t hop_count;
} route_request_t;

// Function prototypes
void mesh_protocol_init(lora_config_t *config);
void mesh_maintenance_task(void *pvParameters);
void mesh_handle_packet(lora_packet_t *packet);
void mesh_perform_maintenance(void);
void mesh_self_healing_check(void);
void mesh_initiate_route_discovery(uint8_t destination);
uint16_t mesh_get_sequence_number(void);

#endif // MESH_PROTOCOL_H