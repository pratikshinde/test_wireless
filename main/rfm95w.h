#ifndef RFM95W_H
#define RFM95W_H

#include <stdint.h>
#include <stdbool.h>

// Mode definitions
#define RFM95W_MODE_SLEEP 0x00
#define RFM95W_MODE_STANDBY 0x81
#define RFM95W_MODE_TX 0x83
#define RFM95W_MODE_RXCONTINUOUS 0x85
#define RFM95W_MODE_RXSINGLE 0x86

// Function prototypes
bool rfm95w_init(int cs, int rst, int dio0);
void rfm95w_set_frequency(uint32_t freq);
void rfm95w_set_spreading_factor(uint8_t sf);
void rfm95w_set_bandwidth(uint32_t bw);
void rfm95w_set_coding_rate(uint8_t cr);
void rfm95w_set_tx_power(int8_t power);
void rfm95w_set_sync_word(uint8_t sync);
void rfm95w_set_preamble_length(uint16_t length);
void rfm95w_set_crc(bool enable);
void rfm95w_set_ldro(bool enable);
void rfm95w_set_mode(uint8_t mode);
bool rfm95w_check_rx(void);
bool rfm95w_receive_packet(uint8_t *buffer, uint8_t *len, int16_t *rssi, int8_t *snr);
int16_t rfm95w_get_current_rssi(void);
bool rfm95w_is_channel_free(int16_t threshold);
bool rfm95w_send_packet(uint8_t *data, uint8_t len);

// Debug
void rfm95w_dump_registers(void);

#endif // RFM95W_H