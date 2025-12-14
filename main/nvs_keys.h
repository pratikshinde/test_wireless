// main/nvs_keys.h
#ifndef NVS_KEYS_H
#define NVS_KEYS_H

// NVS key names (all <= 15 characters for ESP32 compatibility)
#define KEY_NODE_ID          "id"           // 2 chars
#define KEY_NODE_NAME        "name"         // 4 chars
#define KEY_FREQUENCY        "freq"         // 4 chars
#define KEY_SPREAD_FACTOR    "sf"           // 2 chars
#define KEY_BANDWIDTH        "bw"           // 2 chars
#define KEY_CODING_RATE      "cr"           // 2 chars
#define KEY_TX_POWER         "tx_pwr"       // 6 chars
#define KEY_SYNC_WORD        "sync"         // 4 chars
#define KEY_PING_INTERVAL    "ping_int"     // 8 chars
#define KEY_BEACON_INTERVAL  "beacon_int"   // 10 chars
#define KEY_ROUTE_TIMEOUT    "route_to"     // 8 chars
#define KEY_MAX_HOPS         "max_hops"     // 8 chars
#define KEY_ENABLE_ACK       "ack_en"       // 6 chars
#define KEY_ACK_TIMEOUT      "ack_to"       // 6 chars
#define KEY_ENABLE_HEALING   "heal_en"      // 7 chars
#define KEY_HEALING_TIMEOUT  "heal_to"      // 7 chars
#define KEY_WIFI_SSID        "wifi_ssid"    // 9 chars
#define KEY_WIFI_PASSWORD    "wifi_pwd"     // 8 chars
#define KEY_ENABLE_WEB       "web_en"       // 6 chars
#define KEY_WEB_PORT         "web_port"     // 8 chars
#define KEY_ENABLE_ENC       "enc_en"       // 6 chars
#define KEY_AES_KEY          "aes_key"      // 7 chars
#define KEY_IV               "iv"           // 2 chars
#define KEY_ENABLE_CRC       "crc_en"       // 6 chars
#define KEY_ENABLE_LDRO      "ldro_en"      // 7 chars
#define KEY_PREAMBLE_LEN     "pre_len"      // 7 chars
#define KEY_SYMBOL_TIMEOUT   "sym_to"       // 6 chars

#endif // NVS_KEYS_H