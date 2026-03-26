/*
 * Chat Storage Hash Functions
 *
 * Provides hash generation functions for content-addressable storage.
 */

#ifndef STORAGE_HASH_H
#define STORAGE_HASH_H

#include <src/hydrogen.h>

#include <stddef.h>

/**
 * @brief Generate SHA-256 hash of content
 * @param content Input content data
 * @param length Length of input data
 * @return Allocated hash string (caller must free), or NULL on error
 */
char* chat_storage_generate_hash(const char* content, size_t length);

/**
 * @brief Generate SHA-256 hash of JSON string
 * @param json_string Input JSON string
 * @return Allocated hash string (caller must free), or NULL on error
 */
char* chat_storage_generate_hash_from_json(const char* json_string);

#endif // STORAGE_HASH_H
