// main/mesh_types.h
#ifndef MESH_TYPES_H
#define MESH_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Node information structure
typedef struct {
    uint8_t id;
    char name[32];
    int16_t rssi;
    uint8_t hop_count;
    uint32_t last_seen;
    bool online;
} node_info_t;

// Route information structure
typedef struct {
    uint8_t dest_id;        // destination ID
    uint8_t next_hop;       // next hop ID  
    uint8_t hop_count;      // hop count
    int16_t link_quality;   // link quality
    uint32_t last_updated;  // last updated timestamp
    bool active;            // route is active (ADDED for mesh_protocol.c)
} route_info_t;

#endif // MESH_TYPES_H