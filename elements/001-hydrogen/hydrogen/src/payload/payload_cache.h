/*
 * Payload Cache Header
 */

#ifndef PAYLOAD_CACHE_H
#define PAYLOAD_CACHE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// Include payload.h to get type definitions (don't forward declare them)
#include "payload.h"

// Function prototypes for cache operations
bool initialize_payload_cache(void);
bool load_payload_cache(const AppConfig *config, const char *marker);
bool is_payload_cache_available(void);
bool get_payload_files_by_prefix(const char *prefix, PayloadFile **files, size_t *num_files, size_t *capacity);
bool process_payload_tar_cache(const PayloadData *payload_data);
bool process_payload_tar_cache_from_data(const uint8_t *tar_data, size_t tar_size);
void list_tar_contents(const uint8_t *tar_data, size_t tar_size);
bool parse_tar_into_cache(const uint8_t *tar_data, size_t tar_size);
// Forward declarations for internal functions
int compare_files(const void *a, const void *b);
void cleanup_payload_cache(void);

#endif /* PAYLOAD_CACHE_H */
