/*
 * Payload Handler Module
 * 
 * Handles extraction and decryption of embedded payloads from executables.
 * This module is designed to be independent of specific payload types (e.g., Swagger UI)
 * to allow reuse for other components that may need payload functionality.
 */

#ifndef PAYLOAD_H
#define PAYLOAD_H

#include "../constants.h"
#include "../config/config.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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

/**
 * Check if a payload exists in the executable
 * 
 * This function performs a lightweight check for a payload marker and validates
 * basic payload structure. It doesn't perform decryption or decompression.
 * 
 * @param marker The marker string to search for
 * @param size If payload is found, will contain its size
 * @return true if valid payload found, false otherwise
 */
bool check_payload_exists(const char *marker, size_t *size);

/**
 * Validate a payload decryption key
 * 
 * This function checks if the provided key is valid for payload decryption.
 * For environment variables, it checks if the variable exists and has a value.
 * 
 * @param key The key to validate (can be direct or ${env.VAR} format)
 * @return true if key is valid, false otherwise
 */
bool validate_payload_key(const char *key);

/**
 * Launch the payload subsystem
 * 
 * This function extracts and processes the payload from the executable.
 * All logging output is tagged with the "Payload" category.
 * 
 * @param config Application configuration containing the payload key
 * @param marker Marker string that identifies the payload
 * @return true if payload was successfully launched, false otherwise
 */
bool launch_payload(const AppConfig *config, const char *marker);

/**
 * Clean up OpenSSL resources
 * 
 * This function cleans up resources allocated by OpenSSL during payload processing.
 * It should be called during shutdown to prevent memory leaks.
 */
void cleanup_openssl(void);

bool decrypt_payload(const uint8_t *encrypted_data, size_t encrypted_size,
                          const char *private_key_b64, uint8_t **decrypted_data,
                          size_t *decrypted_size);
void init_openssl(void);
bool process_payload_data(const PayloadData *payload);

#endif /* PAYLOAD_H */
