#include "lora_mesh.h"
#include "rfm95w.h"
#include "encryption.h"
#include "mesh_protocol.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <string.h>
#include "esp_timer.h"

static const char *TAG = "LORA_MESH";

// Hardware pins (configured via web UI)
static int cs_pin = 5;
static int rst_pin = 22;
static int dio0_pin = 21;
static int tx_led = 16;
static int rx_led = 17;

//static TaskHandle_t mesh_task_handle = NULL;
static QueueHandle_t lora_queue = NULL;
static lora_config_t *current_config;



// Initialize LoRa with configuration
bool lora_mesh_init(lora_config_t *config) {
    current_config = config;
    
    // Initialize RFM95W driver
    if (!rfm95w_init(cs_pin, rst_pin, dio0_pin)) {
        ESP_LOGE(TAG, "Failed to initialize RFM95W");
        return false;
    }
    
    // Configure LoRa parameters
    rfm95w_set_frequency(config->frequency);
    rfm95w_set_spreading_factor(config->spread_factor);
    rfm95w_set_bandwidth(config->bandwidth);
    rfm95w_set_coding_rate(config->coding_rate);
    rfm95w_set_tx_power(config->tx_power);
    rfm95w_set_sync_word(config->sync_word);
    rfm95w_set_preamble_length(config->preamble_length);
    rfm95w_set_crc(config->enable_crc);
    rfm95w_set_ldro(config->enable_ldro);
    
    // Create packet queue
    lora_queue = xQueueCreate(10, sizeof(lora_packet_t));
    if (!lora_queue) {
        ESP_LOGE(TAG, "Failed to create LoRa queue");
        return false;
    }
    
    // Configure LEDs
    gpio_set_direction(tx_led, GPIO_MODE_OUTPUT);
    gpio_set_direction(rx_led, GPIO_MODE_OUTPUT);
    gpio_set_level(tx_led, 0);
    gpio_set_level(rx_led, 0);
    
    ESP_LOGI(TAG, "LoRa initialized: %.1fMHz SF%d BW:%lu CR:4/%d %ddBm",
             config->frequency / 1000000.0,
             config->spread_factor,
             config->bandwidth,
             config->coding_rate,
             config->tx_power);
    
    return true;
}

// Initialize WiFi AP
void wifi_init_softap(lora_config_t *config) {
    // Create default WiFi AP
    esp_netif_create_default_wifi_ap();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "",
            .password = "",
            .ssid_len = strlen(config->wifi_ssid),
            .channel = 1,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = 10,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    
    strcpy((char*)wifi_config.ap.ssid, config->wifi_ssid);
    strcpy((char*)wifi_config.ap.password, config->wifi_password);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi AP: %s", config->wifi_ssid);
}

// Main mesh task
void lora_mesh_task(void *pvParameters) {
    lora_config_t *config = (lora_config_t *)pvParameters;
    int64_t last_beacon_ms = 0;
    int64_t last_ping_ms = 0;
    
    // Start in RX continuous mode
    rfm95w_set_mode(RFM95W_MODE_RXCONTINUOUS);
    
    while (1) {
        int64_t now_ms = esp_timer_get_time() / 1000;  // Convert to milliseconds
        
        // Check for received packets
        if (rfm95w_check_rx()) {
            gpio_set_level(rx_led, 1);
            
            lora_packet_t packet;
            if (rfm95w_receive_packet(&packet.data[0], &packet.len, &packet.rssi, &packet.snr)) 
            {
                uint8_t decrypted[256];
                size_t decrypted_len = 0;
                
                // Decrypt if enabled
                if (current_config->enable_encryption && packet.len >= 16) {
                    if (decrypt_with_padding_removal(packet.data, packet.len, 
                                                    decrypted, &decrypted_len)) {
                        memcpy(packet.data, decrypted, decrypted_len);
                        packet.len = decrypted_len;
                        ESP_LOGD(TAG, "Packet decrypted: %d -> %d bytes", 
                                packet.len, decrypted_len);
                    } else {
                        ESP_LOGE(TAG, "Decryption failed");
                        continue;
                    }
                }
                
                if (packet.len >= 3) {
                    packet.src = packet.data[0];
                    packet.dest = packet.data[1];
                    packet.type = packet.data[2];
                    
                    // Process packet
                    mesh_handle_packet(&packet);
                }
            }
            gpio_set_level(rx_led, 0);
        }
        
        // Send periodic beacon
        if (now_ms - last_beacon_ms >= (int64_t)config->beacon_interval * 1000) {
            last_beacon_ms = now_ms;
            mesh_send_beacon();
        }
        
        // Master sends pings
        if (config->node_id == 1 && now_ms - last_ping_ms >= (int64_t)config->ping_interval * 1000) {
            last_ping_ms = now_ms;
            mesh_send_ping();
        }
        
        // Process queued packets for transmission
        lora_packet_t tx_packet;
        if (xQueueReceive(lora_queue, &tx_packet, 0) == pdTRUE) {
            gpio_set_level(tx_led, 1);
            rfm95w_send_packet(tx_packet.data, tx_packet.len);
            gpio_set_level(tx_led, 0);
            ESP_LOGI(TAG, "TX to %d: type=0x%02X len=%d", 
                    tx_packet.dest, tx_packet.type, tx_packet.len);
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// Send packet with encryption
bool lora_send_packet(uint8_t dest, uint8_t *data, uint8_t len, msg_type_t type) {
    if (len > 240) {
        ESP_LOGE(TAG, "Packet too long: %d bytes", len);
        return false;
    }
    
    lora_packet_t packet;
    uint8_t packet_len = 0;
    
    // Build packet header
    packet.data[0] = current_config->node_id; // Source
    packet.data[1] = dest;                    // Destination
    packet.data[2] = type;                    // Message type
    packet_len = 3;
    
    // Add data if any
    if (len > 0) {
        memcpy(packet.data + 3, data, len);
        packet_len += len;
    }
    
    // Encrypt if enabled
    if (current_config->enable_encryption) {
        uint8_t encrypted[256];
        if (encrypt_packet(packet.data, encrypted, packet_len)) {
            memcpy(packet.data, encrypted, packet_len);
        }
    }
    
    packet.len = packet_len;
    packet.dest = dest;
    packet.src = current_config->node_id;
    packet.type = type;
    
    // Queue packet for transmission
    if (xQueueSend(lora_queue, &packet, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to queue packet for transmission");
        return false;
    }
    
    return true;
}

// Broadcast packet
bool lora_broadcast_packet(uint8_t *data, uint8_t len, msg_type_t type) {
    return lora_send_packet(0xFF, data, len, type);
}

void mesh_send_ping(void) {
    ping_packet_t ping = {
        .node_id = current_config->node_id,
        .sequence = mesh_get_sequence_number(),
        .timestamp_ms = esp_timer_get_time() / 1000
    };
    
    ESP_LOGD(TAG, "Sending ping: %d bytes", sizeof(ping_packet_t));
    lora_broadcast_packet((uint8_t*)&ping, sizeof(ping_packet_t), MSG_PING);
}

void mesh_send_beacon(void) {
    beacon_packet_t beacon = {
        .node_id = current_config->node_id,
        .node_name = "",
        .sequence = mesh_get_sequence_number(),
        .rssi = 0,
        .hop_count = 0
    };
    strncpy(beacon.node_name, current_config->node_name, sizeof(beacon.node_name) - 1);
    
    ESP_LOGD(TAG, "Sending beacon: %d bytes", sizeof(beacon_packet_t));
    lora_broadcast_packet((uint8_t*)&beacon, sizeof(beacon_packet_t), MSG_BEACON);
}

// Get number of online nodes
uint8_t mesh_get_online_nodes(void) {
    return mesh_get_online_count();
}

// Get number of active routes
uint8_t mesh_get_active_routes(void) {
    return mesh_get_active_route_count();
}