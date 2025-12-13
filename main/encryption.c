#include "encryption.h"
#include "mbedtls/aes.h"
#include "mbedtls/gcm.h"
#include "esp_log.h"
#include <string.h>
#include "esp_random.h"
static const char *TAG = "ENCRYPTION";
static mbedtls_aes_context aes_ctx;
static uint8_t encryption_key[16];
static uint8_t iv[16];

// Initialize encryption
void encryption_init(const uint8_t *key, const uint8_t *initial_vector) {
    memcpy(encryption_key, key, 16);
    memcpy(iv, initial_vector, 16);
    
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_enc(&aes_ctx, encryption_key, 128);
    
    ESP_LOGI(TAG, "AES encryption initialized");
}

// Encrypt packet (AES-CBC)
bool encrypt_packet(uint8_t *plaintext, uint8_t *ciphertext, size_t length) {
    // Remove the length check - caller should ensure proper length
    uint8_t iv_copy[16];
    memcpy(iv_copy, iv, 16);
    
    mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, 
                         length, iv_copy, plaintext, ciphertext);
    
    return true;
}
bool encrypt_with_auto_padding(uint8_t *plaintext, size_t plaintext_len, 
                              uint8_t *ciphertext, size_t *ciphertext_len) {
    // Calculate required padding
    size_t padding = 16 - (plaintext_len % 16);
    size_t total_len = plaintext_len + padding;
    
    if (total_len > 256) {
        ESP_LOGE(TAG, "Data too large after padding: %d bytes", total_len);
        return false;
    }
    
    // Create padded buffer
    uint8_t padded[256];
    memcpy(padded, plaintext, plaintext_len);
    
    // Add PKCS#7 padding
    for (size_t i = 0; i < padding; i++) {
        padded[plaintext_len + i] = (uint8_t)padding;
    }
    
    // Encrypt
    if (!encrypt_packet(padded, ciphertext, total_len)) {
        return false;
    }
    
    *ciphertext_len = total_len;
    return true;
}

// Decrypt packet (AES-CBC)
bool decrypt_packet(uint8_t *ciphertext, uint8_t *plaintext, size_t length) {
    if (length % 16 != 0) {
        ESP_LOGE(TAG, "Packet length must be multiple of 16 for AES-CBC");
        return false;
    }
    
    uint8_t iv_copy[16];
    memcpy(iv_copy, iv, 16);
    
    mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT,
                         length, iv_copy, ciphertext, plaintext);
    
    return true;
}
bool decrypt_with_padding_removal(uint8_t *ciphertext, size_t ciphertext_len,
                                 uint8_t *plaintext, size_t *plaintext_len) {
    // Decrypt
    uint8_t decrypted[256];
    if (!decrypt_packet(ciphertext, decrypted, ciphertext_len)) {
        return false;
    }
    
    // Remove PKCS#7 padding
    uint8_t padding = decrypted[ciphertext_len - 1];
    if (padding > 16 || padding == 0) {
        ESP_LOGE(TAG, "Invalid padding value: %d", padding);
        return false;
    }
    
    // Verify padding
    for (size_t i = 0; i < padding; i++) {
        if (decrypted[ciphertext_len - 1 - i] != padding) {
            ESP_LOGE(TAG, "Invalid padding at position %d", i);
            return false;
        }
    }
    
    // Copy data without padding
    size_t data_len = ciphertext_len - padding;
    memcpy(plaintext, decrypted, data_len);
    *plaintext_len = data_len;
    
    return true;
}
// Pad data to 16-byte boundary
size_t pad_data(uint8_t *data, size_t length) {
    size_t padding = 16 - (length % 16);
    
    if (padding < 16) {
        // Add PKCS#7 padding
        for (size_t i = 0; i < padding; i++) {
            data[length + i] = padding;
        }
        return length + padding;
    }
    
    return length;
}

// Remove padding from data
size_t unpad_data(uint8_t *data, size_t length) {
    if (length == 0) return 0;
    
    uint8_t padding = data[length - 1];
    if (padding > 16 || padding == 0) {
        return length; // Invalid padding
    }
    
    // Verify padding
    for (size_t i = 0; i < padding; i++) {
        if (data[length - 1 - i] != padding) {
            return length; // Invalid padding
        }
    }
    
    return length - padding;
}

// Generate random IV
void generate_random_iv(uint8_t *output) {
    for (int i = 0; i < 16; i++) {
        output[i] = esp_random() & 0xFF;
    }
}

// Generate random key
void generate_random_key(uint8_t *output) {
    for (int i = 0; i < 16; i++) {
        output[i] = esp_random() & 0xFF;
    }
}

// Encrypt with padding
bool encrypt_with_padding(uint8_t *plaintext, size_t plaintext_len, 
                         uint8_t *ciphertext, size_t *ciphertext_len) {
    // Create padded buffer
    uint8_t padded[256];
    size_t padded_len = pad_data(plaintext, plaintext_len);
    
    if (padded_len > 256) {
        ESP_LOGE(TAG, "Data too large after padding");
        return false;
    }
    
    // Copy and pad data
    memcpy(padded, plaintext, plaintext_len);
    pad_data(padded + plaintext_len, padded_len - plaintext_len);
    
    // Encrypt
    if (!encrypt_packet(padded, ciphertext, padded_len)) {
        return false;
    }
    
    *ciphertext_len = padded_len;
    return true;
}

// Decrypt with padding removal
bool decrypt_with_padding(uint8_t *ciphertext, size_t ciphertext_len,
                         uint8_t *plaintext, size_t *plaintext_len) {
    // Decrypt
    if (!decrypt_packet(ciphertext, plaintext, ciphertext_len)) {
        return false;
    }
    
    // Remove padding
    *plaintext_len = unpad_data(plaintext, ciphertext_len);
    return true;
}

// Clean up encryption context
void encryption_cleanup(void) {
    mbedtls_aes_free(&aes_ctx);
    memset(encryption_key, 0, sizeof(encryption_key));
    memset(iv, 0, sizeof(iv));
    
    ESP_LOGI(TAG, "Encryption cleaned up");
}