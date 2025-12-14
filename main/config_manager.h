#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "mesh_types.h"
typedef int esp_err_t;


// ==================== LoRa CONFIGURATION ====================
typedef struct {
    // Node Identity
    uint8_t node_id;            // Node ID (1 = Master, others = Slaves)
    char node_name[32];         // Human-readable node name
    
    // LoRa Physical Parameters
    uint32_t frequency;         // Frequency in Hz (e.g., 868000000)
    uint8_t spread_factor;   // Spreading Factor (7-12)
    uint32_t bandwidth;         // Bandwidth in Hz
    uint8_t coding_rate;        // Coding Rate (5=4/5, 6=4/6, 7=4/7, 8=4/8)
    int8_t tx_power;            // TX Power in dBm (2-20)
    uint8_t sync_word;          // Sync Word (0x12 = private, 0x34 = public)
    
    // Network Parameters
    uint16_t ping_interval;     // Ping interval in seconds
    uint16_t beacon_interval;   // Beacon interval in seconds
    uint16_t route_timeout;     // Route timeout in seconds
    uint8_t max_hops;           // Maximum routing hops (1-10)
    bool enable_ack;            // Enable ACK for messages
    uint16_t ack_timeout;       // ACK timeout in milliseconds
    
    // Mesh Parameters
    bool enable_self_healing;   // Enable self-healing
    uint16_t healing_timeout;   // Self-healing check interval in seconds
    
    // WiFi AP Parameters
    char wifi_ssid[32];         // WiFi AP SSID
    char wifi_password[32];     // WiFi AP Password
    
    // Web Server
    bool enable_web_server;     // Enable web server
    uint16_t web_port;          // Web server port (default: 80)
    
    // Encryption
    uint8_t aes_key[16];        // AES encryption key (16 bytes)
    uint8_t iv[16];             // Initialization vector (16 bytes)
    bool enable_encryption;     // Enable AES encryption
    
    // Advanced LoRa Parameters
    bool enable_crc;            // Enable CRC checking
    bool enable_ldro;           // Enable Low Data Rate Optimize
    uint16_t preamble_length;   // Preamble length
    uint16_t symbol_timeout;    // Symbol timeout
} lora_config_t;

// ==================== FUNCTION PROTOTYPES ====================

// Configuration Management
esp_err_t config_init(void);
esp_err_t config_load_defaults(lora_config_t *config);
esp_err_t config_save(const lora_config_t *config);
esp_err_t config_load(lora_config_t *config);

void config_print(const lora_config_t *config);
bool config_validate(const lora_config_t *config);

// Node Management
void mesh_init_nodes(void);
uint8_t mesh_get_node_count(void);
node_info_t *mesh_get_node(uint8_t index);
uint8_t mesh_get_online_count(void);

// Route Management
void mesh_init_routes(void);
uint8_t mesh_get_route_count(void);

route_info_t *mesh_find_route(uint8_t destination);

void mesh_update_route_quality(uint8_t dest, int16_t link_quality);
void mesh_remove_route(uint8_t dest);

uint8_t mesh_get_active_route_count(void);

// Mesh Network Functions
void mesh_check_route_timeouts(void);
void mesh_self_healing_check(void);
bool mesh_is_node_reachable(uint8_t node_id);
uint8_t mesh_get_next_hop(uint8_t destination);
int16_t mesh_get_link_quality(uint8_t node_id);

// Time Helpers
int64_t get_time_us(void);          // Get current time in microseconds
int64_t get_time_ms(void);          // Get current time in milliseconds
int64_t get_time_sec(void);         // Get current time in seconds
bool is_timeout_expired(int64_t start_time, uint32_t timeout_ms);

// Additional Helper Functions
void mesh_update_self_uptime(void);  // Add this line
void mesh_print_nodes(void);
void mesh_print_routes(void);
void mesh_get_statistics(uint8_t *total_nodes, uint8_t *online_nodes, 
                         uint8_t *total_routes, uint8_t *active_routes);
// Also add this to config_manager.h:
void debug_nvs_content(void);
void debug_nvs_content(void);
void config_print(const lora_config_t *config);
#endif // CONFIG_MANAGER_H