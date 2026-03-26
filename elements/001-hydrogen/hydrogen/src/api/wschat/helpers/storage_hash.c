/*
 * Chat Storage Hash Functions
 *
 * Provides hash generation functions for content-addressable storage.
 */

#include <src/hydrogen.h>

#include <string.h>

#include <src/utils/utils_crypto.h>

#include "storage_hash.h"

char* chat_storage_generate_hash(const char* content, size_t length) {
    if (!content || length == 0) {
        return NULL;
    }

    return utils_sha256_hash((const unsigned char*)content, length);
}

char* chat_storage_generate_hash_from_json(const char* json_string) {
    if (!json_string) {
        return NULL;
    }

    return utils_sha256_hash((const unsigned char*)json_string, strlen(json_string));
}
