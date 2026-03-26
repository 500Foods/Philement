/*
 * Chat Storage Hex Utilities
 *
 * Provides hex conversion functions for binary data encoding/decoding
 * used in chat storage operations.
 */

#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "storage_hex.h"

char* chat_storage_binary_to_hex(const uint8_t* data, size_t len) {
    if (!data || len == 0) {
        return NULL;
    }

    char* hex = calloc(len * 2 + 1, 1);
    if (!hex) {
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        sprintf(hex + i * 2, "%02x", data[i]);
    }

    return hex;
}

/* Helper function to check if a character is a valid hex digit */
static int is_hex_digit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool chat_storage_hex_to_binary(const char* hex, uint8_t** data, size_t* len) {
    if (!hex || !data || !len) {
        return false;
    }

    size_t hex_len = strlen(hex);
    if (hex_len % 2 != 0) {
        return false;
    }

    size_t binary_len = hex_len / 2;
    uint8_t* binary = malloc(binary_len);
    if (!binary) {
        return false;
    }

    for (size_t i = 0; i < binary_len; i++) {
        /* Validate both hex characters before parsing */
        if (!is_hex_digit(hex[i * 2]) || !is_hex_digit(hex[i * 2 + 1])) {
            free(binary);
            return false;
        }
        unsigned int byte;
        if (sscanf(hex + i * 2, "%02x", &byte) != 1) {
            free(binary);
            return false;
        }
        binary[i] = (uint8_t)byte;
    }

    *data = binary;
    *len = binary_len;
    return true;
}
