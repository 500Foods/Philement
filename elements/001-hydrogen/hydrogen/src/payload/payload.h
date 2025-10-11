/*
 * Payload Handler Module
 * 
 * Handles extraction and decryption of embedded payloads from executables.
 * This module is designed to be independent of specific payload types (e.g., Swagger UI)
 * to allow reuse for other components that may need payload functionality.
 */

#ifndef PAYLOAD_H
#define PAYLOAD_H

#include "../globals.h"
#include <src/config/config.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// Structure to hold a single extracted file
typedef struct {
    char *name;         // File name (including path prefix like "swagger/")
    uint8_t *data;      // File content
    size_t size;        // Content size
    bool is_compressed; // Whether content is Brotli compressed
} PayloadFile;

// Structure to hold extracted payload data
typedef struct {
    uint8_t *data;
    size_t size;
    bool is_compressed;  // Whether the data is Brotli compressed
} PayloadData;

// Global payload cache structure
typedef struct {
    bool is_initialized;     // Whether cache is ready
    bool is_available;       // Whether payload was found and extracted
    PayloadFile *files;      // Array of files in the payload
    size_t num_files;       // Number of files available
    size_t capacity;        // Allocated capacity of files array
    uint8_t *tar_data;      // Decompressed tar archive
    size_t tar_size;        // Size of decompressed tar archive
} PayloadCache;

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
 * Check if payload cache is available and initialized
 *
 * This function checks whether the payload cache has been successfully initialized
 * and contains extracted payload files that can be accessed by other subsystems.
 *
 * @return true if payload cache is available and ready, false otherwise
 */
bool is_payload_cache_available(void);

/**
 * Get payload files with a specific prefix
 *
 * This function retrieves all payload files that start with the specified prefix.
 * It's used by subsystems like Swagger and Terminal to get their specific file sets.
 *
 * @param prefix The file prefix to search for (e.g., "swagger/", "terminal/")
 * @param files Pointer to array of PayloadFile structures (will be allocated)
 * @param num_files Pointer to store the number of files found
 * @param capacity Pointer to store the allocated capacity of the array
 * @return true if operation succeeded, false otherwise
 */
bool get_payload_files_by_prefix(const char *prefix, PayloadFile **files,
                                size_t *num_files, size_t *capacity);

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
