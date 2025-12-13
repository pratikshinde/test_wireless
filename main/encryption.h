#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Initialize encryption with key and IV
void encryption_init(const uint8_t *key, const uint8_t *initial_vector);

// Encrypt packet (must be multiple of 16 bytes)
bool encrypt_packet(uint8_t *plaintext, uint8_t *ciphertext, size_t length);

// Decrypt packet (must be multiple of 16 bytes)
bool decrypt_packet(uint8_t *ciphertext, uint8_t *plaintext, size_t length);

// Pad data to 16-byte boundary
size_t pad_data(uint8_t *data, size_t length);

// Remove padding from data
size_t unpad_data(uint8_t *data, size_t length);

// Generate random IV
void generate_random_iv(uint8_t *output);

// Generate random key
void generate_random_key(uint8_t *output);

// Encrypt with automatic padding
bool encrypt_with_padding(uint8_t *plaintext, size_t plaintext_len, 
                         uint8_t *ciphertext, size_t *ciphertext_len);

// Decrypt with automatic padding removal
bool decrypt_with_padding(uint8_t *ciphertext, size_t ciphertext_len,
                         uint8_t *plaintext, size_t *plaintext_len);

// Clean up encryption context
void encryption_cleanup(void);
// Add these prototypes:
bool encrypt_with_auto_padding(uint8_t *plaintext, size_t plaintext_len, 
                              uint8_t *ciphertext, size_t *ciphertext_len);
bool decrypt_with_padding_removal(uint8_t *ciphertext, size_t ciphertext_len,
                                 uint8_t *plaintext, size_t *plaintext_len);
#endif // ENCRYPTION_H