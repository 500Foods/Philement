/*
 * Chat Storage Hex Utilities
 *
 * Provides hex conversion functions for binary data encoding/decoding
 * used in chat storage operations.
 */

#ifndef STORAGE_HEX_H
#define STORAGE_HEX_H

#include <src/hydrogen.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Convert binary data to hexadecimal string
 * @param data Input binary data
 * @param len Length of input data in bytes
 * @return Allocated hex string (caller must free), or NULL on error
 */
char* chat_storage_binary_to_hex(const uint8_t* data, size_t len);

/**
 * @brief Convert hexadecimal string to binary data
 * @param hex Input hexadecimal string
 * @param data Output pointer to allocated binary data (caller must free)
 * @param len Output length of binary data
 * @return true on success, false on error
 */
bool chat_storage_hex_to_binary(const char* hex, uint8_t** data, size_t* len);

#endif // STORAGE_HEX_H
