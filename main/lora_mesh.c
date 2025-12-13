#include "lora_mesh.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mbedtls/aes.h"
#include <string.h>

static const char *TAG = "LORA_MESH";

// Hardware pins (configured via web UI)
static int cs_pin = 5;
static int rst_pin = 22;
static int dio0_pin = 21;
static int tx_led = 16;
static int rx_led = 17;

static spi_device_handle_t spi;
static mbedtls_aes_context aes_ctx;
static lora_config_t *current_config;

// LoRa register operations (same as before but optimized)
static void lora_write_reg(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg | 0x80, value};
    spi_transaction_t t = {.length = 16, .tx_buffer = data};
    spi_device_transmit(spi, &t);
}

static uint8_t lora_read_reg(uint8_t reg) {
    uint8_t tx[2] = {reg & 0x7F, 0};
    uint8_t rx[2];
    spi_transaction_t t = {.length = 16, .tx_buffer = tx, .rx_buffer = rx};
    spi_device_transmit(spi, &t);
    return rx[1];
}

// Initialize LoRa with configuration
bool lora_mesh_init(lora_config_t *config) {
    current_config = config;
    
    // Configure SPI
    spi_bus_config_t buscfg = {
        .mosi_io_num = 23,
        .miso_io_num = 19,
        .sclk_io_num = 18,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4094
    };
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,
        .mode = 0,
        .spics_io_num = cs_pin,
        .queue_size = 7
    };
    
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));
    
    // Reset LoRa module
    gpio_set_direction(rst_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(rst_pin, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(rst_pin, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    
    // Configure LoRa parameters
    lora_write_reg(0x01, 0x80); // Sleep + LoRa
    vTaskDelay(10 / portTICK_PERIOD_MS);
    
    // Set frequency
    uint64_t frf = ((uint64_t)config->frequency << 19) / 32000000;
    lora_write_reg(0x06, (frf >> 16) & 0xFF);
    lora_write_reg(0x07, (frf >> 8) & 0xFF);
    lora_write_reg(0x08, frf & 0xFF);
    
    // Set TX power
    if (config->tx_power > 20) config->tx_power = 20;
    if (config->tx_power < 2) config->tx_power = 2;
    lora_write_reg(0x09, 0x80 | (config->tx_power - 2));
    
    // Set LoRa modem parameters
    uint8_t bw_value = 7; // 125kHz default
    switch(config->bandwidth) {
        case 7800: bw_value = 0; break;
        case 10400: bw_value = 1; break;
        case 15600: bw_value = 2; break;
        case 20800: bw_value = 3; break;
        case 31250: bw_value = 4; break;
        case 41700: bw_value = 5; break;
        case 62500: bw_value = 6; break;
        case 125000: bw_value = 7; break;
        case 250000: bw_value = 8; break;
    }
    
    lora_write_reg(0x1D, (bw_value << 4) | (config->coding_rate << 1) | (config->enable_crc ? 0x01 : 0x00));
    lora_write_reg(0x1E, (config->spreading_factor << 4) | 0x04);
    
    // Set sync word
    lora_write_reg(0x39, config->sync_word);
    
    // Set preamble length
    lora_write_reg(0x20, (config->preamble_length >> 8) & 0xFF);
    lora_write_reg(0x21, config->preamble_length & 0xFF);
    
    // Initialize AES if encryption enabled
    if (config->enable_encryption) {
        mbedtls_aes_init(&aes_ctx);
        mbedtls_aes_setkey_enc(&aes_ctx, config->aes_key, 128);
    }
    
    // Configure LEDs
    gpio_set_direction(tx_led, GPIO_MODE_OUTPUT);
    gpio_set_direction(rx_led, GPIO_MODE_OUTPUT);
    
    // Enter standby mode
    lora_write_reg(0x01, 0x81);
    
    ESP_LOGI(TAG, "LoRa initialized: %.1fMHz SF%d BW:%lu CR:4/%d %ddBm",
             config->frequency / 1000000.0,
             config->spreading_factor,
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
    uint32_t last_beacon = 0;
    uint32_t last_ping = 0;
    uint32_t last_healing = 0;
    
    // Start in RX continuous mode
    lora_write_reg(0x01, 0x85);
    
    while (1) {
        uint32_t now = esp_timer_get_time() / 1000;
        
        // Check for received packets
        if ((lora_read_reg(0x12) & 0x40) != 0) { // RxDone
            gpio_set_level(rx_led, 1);
            
            uint8_t len = lora_read_reg(0x13);
            uint8_t data[256];
            
            // Read FIFO
            for (int i = 0; i < len; i++) {
                data[i] = lora_read_reg(0x00);
            }
            
            // Process packet
            if (len >= 3) {
                uint8_t src = data[0];
                uint8_t dest = data[1];
                uint8_t type = data[2];
                
                // Get RSSI
                int16_t rssi = -157 + lora_read_reg(0x1A);
                
                // Update node info
                if (dest == config->node_id || dest == 0xFF) {
                    mesh_update_routing_table(src, rssi, 1);
                    
                    ESP_LOGI(TAG, "RX from %d: type=0x%02X RSSI=%d", 
                            src, type, rssi);
                }
            }
            
            lora_write_reg(0x12, 0xFF); // Clear IRQs
            gpio_set_level(rx_led, 0);
        }
        
        // Send periodic beacon
        if (now - last_beacon >= config->beacon_interval * 1000) {
            last_beacon = now;
            mesh_send_beacon();
        }
        
        // Master sends pings
        if (config->node_id == 1 && now - last_ping >= config->ping_interval * 1000) {
            last_ping = now;
            mesh_send_ping();
        }
        
        // Self-healing check
        if (config->enable_self_healing && now - last_healing >= config->healing_timeout * 1000) {
            last_healing = now;
            mesh_self_healing();
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
    
    uint8_t packet[256];
    uint8_t packet_len = 0;
    
    // Build packet header
    packet[0] = current_config->node_id; // Source
    packet[1] = dest;                    // Destination
    packet[2] = type;                    // Message type
    packet_len = 3;
    
    // Add data if any
    if (len > 0) {
        memcpy(packet + 3, data, len);
        packet_len += len;
    }
    
    // Encrypt if enabled
    if (current_config->enable_encryption) {
        uint8_t iv_copy[16];
        memcpy(iv_copy, current_config->iv, 16);
        
        size_t padded_len = ((packet_len + 15) / 16) * 16;
        uint8_t encrypted[256];
        
        mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, 
                             padded_len, iv_copy, packet, encrypted);
        
        memcpy(packet, encrypted, padded_len);
        packet_len = padded_len;
    }
    
    // Send packet
    lora_write_reg(0x0D, 0x00); // Reset FIFO pointer
    lora_write_reg(0x22, packet_len); // Payload length
    
    for (int i = 0; i < packet_len; i++) {
        while ((lora_read_reg(0x1E) & 0x80) == 0); // Wait FIFO space
        lora_write_reg(0x00, packet[i]);
    }
    
    // Start transmission
    gpio_set_level(tx_led, 1);
    lora_write_reg(0x01, 0x83); // TX mode
    
    // Wait for TX done
    while ((lora_read_reg(0x12) & 0x08) == 0);
    
    lora_write_reg(0x12, 0xFF); // Clear IRQs
    lora_write_reg(0x01, 0x81); // Standby mode
    gpio_set_level(tx_led, 0);
    
    ESP_LOGI(TAG, "TX to %d: type=0x%02X len=%d", dest, type, packet_len);
    return true;
}

// Broadcast packet
bool lora_broadcast_packet(uint8_t *data, uint8_t len, msg_type_t type) {
    return lora_send_packet(0xFF, data, len, type);
}

// Send beacon packet
void mesh_send_beacon(void) {
    uint8_t beacon_data[4] = {current_config->node_id, 0, 0, 0};
    lora_broadcast_packet(beacon_data, 4, MSG_BEACON);
}

// Send ping packet
void mesh_send_ping(void) {
    uint8_t ping_data[8] = {current_config->node_id, 0, 0, 0, 0, 0, 0, 0};
    uint32_t timestamp = esp_timer_get_time() / 1000;
    memcpy(ping_data + 4, &timestamp, 4);
    lora_broadcast_packet(ping_data, 8, MSG_PING);
}

// Self-healing algorithm
void mesh_self_healing(void) {
    ESP_LOGI(TAG, "Running self-healing check");
    // Implementation would check routing table and find alternative routes
}

// Get number of online nodes
uint8_t mesh_get_online_nodes(void) {
    // Return count from config_manager
    return 1; // Placeholder
}

// Get number of active routes
uint8_t mesh_get_active_routes(void) {
    // Return count from config_manager
    return 0; // Placeholder
}