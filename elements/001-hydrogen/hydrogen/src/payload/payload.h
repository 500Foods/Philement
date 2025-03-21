/*
 * Payload Handler Module
 * 
 * Handles extraction and decryption of embedded payloads from executables.
 * This module is designed to be independent of specific payload types (e.g., Swagger UI)
 * to allow reuse for other components that may need payload functionality.
 */

#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "../config/config.h"

// Structure to hold extracted payload data
typedef struct {
    uint8_t *data;
    size_t size;
    bool is_compressed;  // Whether the data is Brotli compressed
} PayloadData;

/**
 * Extract an encrypted payload from the executable
 * 
 * @param executable_path Path to the executable containing the payload
 * @param config Application configuration containing the payload key
 * @param marker Marker string that identifies the payload
 * @param payload Pointer to PayloadData structure to store the result
 * @return true if payload was successfully extracted, false otherwise
 */
bool extract_payload(const char *executable_path, 
                    const AppConfig *config,
                    const char *marker,
                    PayloadData *payload);

/**
 * Free resources associated with a payload
 * 
 * @param payload Pointer to PayloadData structure to free
 */
void free_payload(PayloadData *payload);

#endif /* PAYLOAD_H */