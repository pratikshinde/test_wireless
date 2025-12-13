#ifndef LORA_PACKET_H
#define LORA_PACKET_H

#include <stdint.h>
#include <stdbool.h>

// LoRa packet structure (shared between modules)
typedef struct {
    uint8_t data[256];
    uint8_t len;
    uint8_t dest;
    uint8_t src;
    uint8_t type;
    int16_t rssi;
    int8_t snr;
} lora_packet_t;

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

#endif // LORA_PACKET_H