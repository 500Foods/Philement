/*
 * Chat Storage Hash Functions
 *
 * Provides hash generation functions for content-addressable storage.
 */

#ifndef STORAGE_HASH_H
#define STORAGE_HASH_H

#include <src/hydrogen.h>

#include <stddef.h>

char* chat_storage_generate_hash(const char* content, size_t length);

#endif // STORAGE_HASH_H
