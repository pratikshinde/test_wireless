#include "rfm95w.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>
#include "esp_timer.h"
static const char *TAG = "RFM95W";

static spi_device_handle_t spi_handle;
static int cs_pin, rst_pin, dio0_pin;

// Register definitions
enum {
    REG_FIFO = 0x00,
    REG_OP_MODE = 0x01,
    REG_FRF_MSB = 0x06,
    REG_FRF_MID = 0x07,
    REG_FRF_LSB = 0x08,
    REG_PA_CONFIG = 0x09,
    REG_PA_RAMP = 0x0A,
    REG_OCP = 0x0B,
    REG_LNA = 0x0C,
    REG_FIFO_ADDR_PTR = 0x0D,
    REG_FIFO_TX_BASE_ADDR = 0x0E,
    REG_FIFO_RX_BASE_ADDR = 0x0F,
    REG_FIFO_RX_CURRENT_ADDR = 0x10,
    REG_IRQ_FLAGS_MASK = 0x11,
    REG_IRQ_FLAGS = 0x12,
    REG_RX_NB_BYTES = 0x13,
    REG_PKT_RSSI_VALUE = 0x1A,
    REG_PKT_SNR_VALUE = 0x19,
    REG_MODEM_CONFIG_1 = 0x1D,
    REG_MODEM_CONFIG_2 = 0x1E,
    REG_PREAMBLE_MSB = 0x20,
    REG_PREAMBLE_LSB = 0x21,
    REG_PAYLOAD_LENGTH = 0x22,
    REG_MODEM_CONFIG_3 = 0x26,
    REG_FREQ_ERROR_MSB = 0x28,
    REG_FREQ_ERROR_MID = 0x29,
    REG_FREQ_ERROR_LSB = 0x2A,
    REG_RSSI_WIDEBAND = 0x2C,
    REG_DETECTION_OPTIMIZE = 0x31,
    REG_INVERTIQ = 0x33,
    REG_DETECTION_THRESHOLD = 0x37,
    REG_SYNC_WORD = 0x39,
    REG_INVERTIQ2 = 0x3B,
    REG_DIO_MAPPING_1 = 0x40,
    REG_DIO_MAPPING_2 = 0x41,
    REG_VERSION = 0x42,
    REG_PA_DAC = 0x4D,
    REG_FORMER_TEMP = 0x5B,
    REG_AGC_REF = 0x61,
    REG_AGC_THRESH_1 = 0x62,
    REG_AGC_THRESH_2 = 0x63,
    REG_AGC_THRESH_3 = 0x64,
    REG_PLL = 0x70
};

// Write register
static void write_reg(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg | 0x80, value};
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = data,
        .flags = SPI_DEVICE_HALFDUPLEX
    };
    spi_device_polling_transmit(spi_handle, &t);
}

// Read register
static uint8_t read_reg(uint8_t reg) {
    uint8_t tx[2] = {reg & 0x7F, 0x00};
    uint8_t rx[2] = {0};
    
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx,
        .rx_buffer = rx,
        .flags = SPI_DEVICE_HALFDUPLEX
    };
    spi_device_polling_transmit(spi_handle, &t);
    
    return rx[1];
}

bool rfm95w_init(int cs, int rst, int dio0) {
    cs_pin = cs;
    rst_pin = rst;
    dio0_pin = dio0;
    
    ESP_LOGI(TAG, "Initializing RFM95W with pins: CS=%d, RST=%d, DIO0=%d", cs, rst, dio0);
    
    // Configure SPI
    spi_bus_config_t buscfg = {
        .mosi_io_num = 23,
        .miso_io_num = 19,
        .sclk_io_num = 18,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4092
    };
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 500000,
        .mode = 0,
        .spics_io_num = cs_pin,
        .queue_size = 7,
        .flags = 0,  // Full duplex
        .pre_cb = NULL
    };
    
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED);  // Disable DMA
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return false;
    }
    
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Reset module
    gpio_set_direction(rst_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(rst_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(100));  // Longer reset delay
    gpio_set_level(rst_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Check version
    uint8_t version = read_reg(REG_VERSION);
    ESP_LOGI(TAG, "RFM95W Version: 0x%02X", version);
    
    // Try reading multiple registers to verify communication
    ESP_LOGI(TAG, "Testing register reads:");
    ESP_LOGI(TAG, "REG_OP_MODE: 0x%02X", read_reg(REG_OP_MODE));
    ESP_LOGI(TAG, "REG_FRF_MSB: 0x%02X", read_reg(REG_FRF_MSB));
    
    if (version != 0x12) {
        ESP_LOGE(TAG, "Invalid RFM95W version. Expected 0x12, got 0x%02X", version);
        ESP_LOGE(TAG, "Check SPI connections: MOSI=23, MISO=19, SCK=18, CS=%d, RST=%d", cs, rst);
        return false;
    }
    
    // Set sleep mode
    write_reg(REG_OP_MODE, 0x00);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Set LoRa mode
    write_reg(REG_OP_MODE, 0x80);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    ESP_LOGI(TAG, "RFM95W initialized successfully");
    return true;
}

// Set frequency
void rfm95w_set_frequency(uint32_t freq) {
    uint64_t frf = ((uint64_t)freq << 19) / 32000000;
    write_reg(REG_FRF_MSB, (frf >> 16) & 0xFF);
    write_reg(REG_FRF_MID, (frf >> 8) & 0xFF);
    write_reg(REG_FRF_LSB, frf & 0xFF);
}

// Set spreading factor
void rfm95w_set_spreading_factor(uint8_t sf) {
    if (sf < 6 || sf > 12) sf = 7;
    
    uint8_t config2 = read_reg(REG_MODEM_CONFIG_2);
    config2 = (config2 & 0x0F) | ((sf << 4) & 0xF0);
    write_reg(REG_MODEM_CONFIG_2, config2);
    
    // Enable Low Data Rate Optimize for SF11 and SF12
    if (sf >= 11) {
        uint8_t config3 = read_reg(REG_MODEM_CONFIG_3);
        write_reg(REG_MODEM_CONFIG_3, config3 | 0x08);
    }
}

// Set bandwidth
void rfm95w_set_bandwidth(uint32_t bw) {
    uint8_t bw_value;
    
    if (bw <= 7800) bw_value = 0;
    else if (bw <= 10400) bw_value = 1;
    else if (bw <= 15600) bw_value = 2;
    else if (bw <= 20800) bw_value = 3;
    else if (bw <= 31250) bw_value = 4;
    else if (bw <= 41700) bw_value = 5;
    else if (bw <= 62500) bw_value = 6;
    else if (bw <= 125000) bw_value = 7;
    else bw_value = 8;  // 250kHz
    
    uint8_t config1 = read_reg(REG_MODEM_CONFIG_1);
    config1 = (config1 & 0x0F) | (bw_value << 4);
    write_reg(REG_MODEM_CONFIG_1, config1);
}

// Set coding rate
void rfm95w_set_coding_rate(uint8_t cr) {
    if (cr < 5 || cr > 8) cr = 5;
    
    uint8_t config1 = read_reg(REG_MODEM_CONFIG_1);
    config1 = (config1 & 0xF1) | ((cr - 5) << 1);
    write_reg(REG_MODEM_CONFIG_1, config1);
}

// Set TX power
void rfm95w_set_tx_power(int8_t power) {
    if (power > 20) power = 20;
    if (power < 2) power = 2;
    
    if (power > 17) {
        // Enable +20dBm mode
        write_reg(REG_PA_DAC, 0x87);
        write_reg(REG_PA_CONFIG, 0x8F | ((power - 5) << 4));
    } else {
        write_reg(REG_PA_DAC, 0x84);
        write_reg(REG_PA_CONFIG, 0x80 | ((power - 2) << 4));
    }
}

// Set sync word
void rfm95w_set_sync_word(uint8_t sync) {
    write_reg(REG_SYNC_WORD, sync);
}

// Set preamble length
void rfm95w_set_preamble_length(uint16_t length) {
    write_reg(REG_PREAMBLE_MSB, (length >> 8) & 0xFF);
    write_reg(REG_PREAMBLE_LSB, length & 0xFF);
}

// Set CRC
void rfm95w_set_crc(bool enable) {
    uint8_t config2 = read_reg(REG_MODEM_CONFIG_2);
    if (enable) {
        config2 |= 0x04;
    } else {
        config2 &= ~0x04;
    }
    write_reg(REG_MODEM_CONFIG_2, config2);
}

// Set LDRO
void rfm95w_set_ldro(bool enable) {
    uint8_t config3 = read_reg(REG_MODEM_CONFIG_3);
    if (enable) {
        config3 |= 0x08;
    } else {
        config3 &= ~0x08;
    }
    write_reg(REG_MODEM_CONFIG_3, config3);
}

// Set mode
void rfm95w_set_mode(uint8_t mode) {
    write_reg(REG_OP_MODE, mode);
}

// Check if packet received
bool rfm95w_check_rx(void) {
    return (read_reg(REG_IRQ_FLAGS) & 0x40) != 0;
}

// Receive packet
bool rfm95w_receive_packet(uint8_t *buffer, uint8_t *len, int16_t *rssi, int8_t *snr) {
    uint8_t irq_flags = read_reg(REG_IRQ_FLAGS);
    
    if (!(irq_flags & 0x40)) {
        return false; // No packet received
    }
    
    // Clear IRQ flags
    write_reg(REG_IRQ_FLAGS, 0xFF);
    
    // Read packet length
    *len = read_reg(REG_RX_NB_BYTES);
    if (*len == 0) 
    {
        return false;
    }
    
    // Read FIFO
    uint8_t current_addr = read_reg(REG_FIFO_RX_CURRENT_ADDR);
    write_reg(REG_FIFO_ADDR_PTR, current_addr);
    
    for (int i = 0; i < *len; i++) {
        buffer[i] = read_reg(REG_FIFO);
    }
    
    // Read RSSI and SNR
    *rssi = -157 + read_reg(REG_PKT_RSSI_VALUE);
    *snr = (int8_t)read_reg(REG_PKT_SNR_VALUE) / 4;
    
    return true;
}

// Send packet
bool rfm95w_send_packet(uint8_t *data, uint8_t len) {
    // Set standby mode
    write_reg(REG_OP_MODE, 0x81);
    
    // Clear IRQ flags
    write_reg(REG_IRQ_FLAGS, 0xFF);
    
    // Set FIFO pointer
    write_reg(REG_FIFO_ADDR_PTR, 0x00);
    
    // Write packet to FIFO
    for (int i = 0; i < len; i++) {
        write_reg(REG_FIFO, data[i]);
    }
    
    // Set payload length
    write_reg(REG_PAYLOAD_LENGTH, len);
    
    // Start transmission
    write_reg(REG_OP_MODE, 0x83);
    
    // Wait for TX done
    uint32_t start = esp_timer_get_time() / 1000;
    while ((read_reg(REG_IRQ_FLAGS) & 0x08) == 0) {
        if ((esp_timer_get_time() / 1000 - start) > 5000) {
            ESP_LOGE(TAG, "TX timeout");
            return false;
        }
        vTaskDelay(1);
    }
    
    // Clear IRQ flags
    write_reg(REG_IRQ_FLAGS, 0xFF);
    
    // Return to RX mode
    write_reg(REG_OP_MODE, 0x85);
    
    return true;
}