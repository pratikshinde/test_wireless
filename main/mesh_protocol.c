#include "mesh_protocol.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "mesh_manager.h"
#include "mesh_types.h"
static const char *TAG = "MESH_PROTOCOL";
static lora_config_t *current_config;
static uint16_t sequence_number = 0;

// Forward declarations from lora_mesh.h to avoid circular dependency
bool lora_send_packet(uint8_t dest, uint8_t *data, uint8_t len, msg_type_t type);
bool lora_broadcast_packet(uint8_t *data, uint8_t len, msg_type_t type);
void mesh_send_beacon(void);

// Forward declarations for packet handlers
static void handle_beacon_packet(lora_packet_t *packet);
static void handle_ping_packet(lora_packet_t *packet);
static void handle_pong_packet(lora_packet_t *packet);
static void handle_data_packet(lora_packet_t *packet);
static void handle_route_discovery(lora_packet_t *packet);
static void handle_route_reply(lora_packet_t *packet);

// Initialize mesh protocol
void mesh_protocol_init(lora_config_t *config) {
    current_config = config;
    sequence_number = esp_random() & 0xFFFF;
    
    ESP_LOGI(TAG, "Mesh protocol initialized, node ID: %d", config->node_id);
}

// Mesh maintenance task
void mesh_maintenance_task(void *pvParameters) {
    lora_config_t *config = (lora_config_t *)pvParameters;
    int64_t last_maintenance_ms = 0;
    int64_t last_self_healing_ms = 0;
    
    while (1) {
        int64_t now_ms = esp_timer_get_time() / 1000;
        
        // Periodic maintenance
        if (now_ms - last_maintenance_ms >= 10000) { // Every 10 seconds
            last_maintenance_ms = now_ms;
            mesh_perform_maintenance();
        }
        
        // Self-healing check
        if (config->enable_self_healing && 
            now_ms - last_self_healing_ms >= (int64_t)config->healing_timeout * 1000) {
            last_self_healing_ms = now_ms;
            mesh_self_healing_check();
        }
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Handle incoming packet
void mesh_handle_packet(lora_packet_t *packet) {
    if (!current_config || !packet) return;
    
    // Update node information
    mesh_update_node_seen(packet->src);
    mesh_update_node_rssi(packet->src, packet->rssi);
    
    // Process based on packet type
    switch (packet->type) {
        case MSG_BEACON:
            handle_beacon_packet(packet);
            break;
            
        case MSG_PING:
            handle_ping_packet(packet);
            break;
            
        case MSG_PONG:
            handle_pong_packet(packet);
            break;
            
        case MSG_DATA:
            handle_data_packet(packet);
            break;
            
        case MSG_ROUTE_DISCOVERY:
            handle_route_discovery(packet);
            break;
            
        case MSG_ROUTE_REPLY:
            handle_route_reply(packet);
            break;

        case MSG_CONFIG_SYNC:
            // handle_config_sync(packet);
            ESP_LOGD(TAG, "Config sync packet received (ignored)");
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown packet type: 0x%02X from %d (len=%d)", 
                    packet->type, packet->src, packet->len);
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, packet->data, packet->len, ESP_LOG_WARN);
            break;
    }
}

// Handle beacon packet
static void handle_beacon_packet(lora_packet_t *packet) {
    if (!current_config || !packet || packet->len < sizeof(beacon_packet_t)) {
        return;
    }
    
    beacon_packet_t *beacon = (beacon_packet_t *)packet->data;
    
    // Update node information
    mesh_add_or_update_node(beacon->node_id, beacon->node_name, 
                           packet->rssi, beacon->hop_count + 1);
    
    // Update route
    mesh_add_or_update_route(beacon->node_id, beacon->node_id, 
                            beacon->hop_count + 1, packet->rssi);
    
    // Forward beacon with increased hop count (if not too many hops)
    if (beacon->hop_count < current_config->max_hops - 1) {
        beacon->hop_count++;
        lora_broadcast_packet((uint8_t*)beacon, sizeof(beacon_packet_t), MSG_BEACON);
    }
}

// Handle ping packet
static void handle_ping_packet(lora_packet_t *packet) {
    if (!current_config || !packet || packet->len < sizeof(ping_packet_t)) {
        return;
    }
    
    ping_packet_t *ping = (ping_packet_t *)packet->data;
    
    // Send pong response
    // FIX: Echo the Sender's timestamp back so Sender can calculate RTT
    pong_packet_t pong = {
        .node_id = current_config->node_id,
        .ping_sequence = ping->sequence,
        .timestamp_ms = ping->timestamp_ms, // Echo original timestamp
        .rtt_ms = 0 // Not used in packet
    };
    
    // Add random jitter to prevent collision (0-1000ms)
    // Only blocking this task is fine as it's the mesh handling task
    uint32_t delay_ms = esp_random() % 1000;
    vTaskDelay(pdMS_TO_TICKS(delay_ms));

    lora_send_packet(ping->node_id, (uint8_t*)&pong, sizeof(pong_packet_t), MSG_PONG);
}

// Handle pong packet
static void handle_pong_packet(lora_packet_t *packet) {
    if (!packet) return;
    
    // Update node latency information
    if (packet->len >= sizeof(pong_packet_t)) {
        pong_packet_t *pong = (pong_packet_t *)packet->data;
        
        // Calculate RTT locally
        uint32_t now = esp_timer_get_time() / 1000;
        uint32_t rtt = now - pong->timestamp_ms;
        
        ESP_LOGI(TAG, "Pong from %d: RTT=%lums (Seq=%d)", 
                 pong->node_id, rtt, pong->ping_sequence);
    }
}

// Handle data packet
static void handle_data_packet(lora_packet_t *packet) {
    if (!current_config || !packet) return;
    
    // Check if packet is for us
    if (packet->dest == current_config->node_id) {
        // Process data
        ESP_LOGI(TAG, "Data packet from %d: %d bytes", 
                packet->src, packet->len);
        
        // TODO: Handle application data
    } 
    // Check if we should forward
    else if (packet->dest != 0xFF) { // Not broadcast
        // Find route to destination
        uint8_t next_hop = mesh_get_next_hop(packet->dest);
        if (next_hop != 0) {
            // Forward packet
            lora_send_packet(next_hop, packet->data, packet->len, MSG_DATA);
            ESP_LOGI(TAG, "Forwarding packet from %d to %d via %d", 
                    packet->src, packet->dest, next_hop);
        } else {
            ESP_LOGW(TAG, "No route to %d for packet from %d", 
                    packet->dest, packet->src);
        }
    }
}

// Handle route discovery
static void handle_route_discovery(lora_packet_t *packet) {
    if (!packet || packet->len < sizeof(route_request_t) + 3) return;
    
    route_request_t *rreq = (route_request_t *)packet->data;
    
    ESP_LOGI(TAG, "RREQ: Origin=%d Target=%d Seq=%d Hops=%d (via %d)", 
             rreq->src, rreq->dest, rreq->sequence, rreq->hop_count, packet->src);
             
    // 1. Update reverse route to Originator
    mesh_add_or_update_route(rreq->src, packet->src, rreq->hop_count + 1, packet->rssi);
    
    // 2. Check if we are the target
    if (rreq->dest == current_config->node_id) {
        ESP_LOGI(TAG, "RREQ reached target! Sending RREP");
        
        route_reply_t rrep = {
            .src = current_config->node_id, // We are the target
            .dest = rreq->src,              // Originator is destination of RREP
            .sequence = rreq->sequence,
            .hop_count = 0
        };
        
        // Send unicast RREP back to the node we received RREQ from
        lora_send_packet(packet->src, (uint8_t*)&rrep, sizeof(route_reply_t), MSG_ROUTE_REPLY);
        
    } else {
        // 3. Forward RREQ if TTL allows and not seen before (simplified loop check)
        if (rreq->hop_count < current_config->max_hops - 1) {
            // Check if we have processed this RREQ recently (Optimization omitted for simplicity, relying on hop count)
            // Ideally we should check a "seen RREQ" cache
            
            rreq->hop_count++;
            lora_broadcast_packet((uint8_t*)rreq, sizeof(route_request_t), MSG_ROUTE_DISCOVERY);
        }
    }
}

// Handle route reply
static void handle_route_reply(lora_packet_t *packet) {
    if (!packet || packet->len < sizeof(route_reply_t) + 3) return;
    
    route_reply_t *rrep = (route_reply_t *)packet->data;
    
    ESP_LOGI(TAG, "RREP: Target=%d Origin=%d Hops=%d (via %d)", 
             rrep->src, rrep->dest, rrep->hop_count, packet->src);
             
    // 1. Update forward route to Target (rrep.src)
    mesh_add_or_update_route(rrep->src, packet->src, rrep->hop_count + 1, packet->rssi);
    
    // 2. Check if we are the connection initiator (Originator)
    if (rrep->dest == current_config->node_id) {
        ESP_LOGI(TAG, "Route discovery complete! Route to %d established via %d", 
                 rrep->src, packet->src);
                 
    } else {
        // 3. Forward RREP towards Originator (rrep.dest)
        if (rrep->hop_count < current_config->max_hops - 1) {
            uint8_t next_hop = mesh_get_next_hop(rrep->dest);
            
            if (next_hop != 0) {
                rrep->hop_count++;
                lora_send_packet(next_hop, (uint8_t*)rrep, sizeof(route_reply_t), MSG_ROUTE_REPLY);
            } else {
                ESP_LOGW(TAG, "RREP Forward failed: No route to Originator %d", rrep->dest);
                // Fallback: Broadcast if enabled? Or just DROP.
                // Dropping is safer to prevent loops.
            }
        }
    }
}

// Perform mesh maintenance
void mesh_perform_maintenance(void) {
    if (!current_config) return;
    
    // Clear offline nodes
    mesh_clear_offline_nodes(current_config->route_timeout);
    
    // Clear expired routes
    mesh_clear_expired_routes(current_config->route_timeout);
    
    // Update statistics
    mesh_update_self_uptime();
}

// Self-healing algorithm
void mesh_self_healing_check(void) {
    if (!current_config || !current_config->enable_self_healing) return;
    
    ESP_LOGI(TAG, "Running self-healing check");
    
    // Check for broken routes
    for (int i = 0; i < mesh_get_route_count(); i++) {
        route_info_t *route = mesh_get_route(i);
        if (route && route->active) {
            // Check if next hop is still online
            node_info_t *next_hop = mesh_find_node(route->next_hop);
            if (!next_hop || !next_hop->online) {
                ESP_LOGW(TAG, "Route to %d via %d is broken, initiating repair", 
                        route->dest_id, route->next_hop);
                
                // Mark route as inactive
                route->active = false;
                
                // Try to find alternative route
                mesh_initiate_route_discovery(route->dest_id);
            }
        }
    }
    
    // Check for isolated nodes
    uint8_t online_count = mesh_get_online_count();
    if (online_count <= 1 && current_config->node_id != 1) {
        ESP_LOGW(TAG, "Node isolated from mesh, attempting to reconnect");
        // Send beacon to re-establish connection
        mesh_send_beacon();
    }
}

// Initiate route discovery
void mesh_initiate_route_discovery(uint8_t destination) {
    if (!current_config) return;
    
    route_request_t rreq = {
        .src = current_config->node_id,
        .dest = destination,
        .sequence = ++sequence_number,
        .hop_count = 0
    };
    
    // Broadcast route request
    lora_broadcast_packet((uint8_t*)&rreq, sizeof(route_request_t), MSG_ROUTE_DISCOVERY);
    
    ESP_LOGI(TAG, "Initiating route discovery to node %d", destination);
}

// Get sequence number
uint16_t mesh_get_sequence_number(void) {
    return ++sequence_number;
}