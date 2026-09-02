/*
 * Chat Context Hashing - Implementation (Phase 8)
 *
 * Provides context hashing functionality for reducing bandwidth usage
 * by allowing clients to send hashes instead of full message content.
 * Server reconstructs context from hashes using QueryRef #062.
 */

#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <src/logging/logging.h>
#include <src/utils/utils_crypto.h>

#include "context_hashing.h"
#include "storage.h"

static const char* SR_CONTEXT_HASHING = "CONTEXT_HASHING";

bool chat_context_validate_hash(const char* hash) {
    if (!hash) {
        return false;
    }

    size_t len = strlen(hash);
    if (len != 43) {  // base64url encoded SHA-256 (without padding) is 43 characters
        return false;
    }

    // Validate base64url characters
    for (size_t i = 0; i < len; i++) {
        char c = hash[i];
        if (!((c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') ||
              c == '-' || c == '_')) {
            return false;
        }
    }

    return true;
}

char** chat_context_parse_request_hashes(json_t* request_json, char** error_message) {
    if (!request_json) {
        if (error_message) {
            *error_message = strdup("Invalid request JSON");
        }
        return NULL;
    }

    json_t* context_hashes_obj = json_object_get(request_json, "context_hashes");
    if (!context_hashes_obj) {
        // No context_hashes field is valid - client may send full messages
        return NULL;
    }

    if (!json_is_array(context_hashes_obj)) {
        if (error_message) {
            *error_message = strdup("context_hashes must be an array");
        }
        return NULL;
    }

    size_t hash_count = json_array_size(context_hashes_obj);
    if (hash_count == 0) {
        return NULL;
    }

    if (hash_count > CHAT_CONTEXT_MAX_HASHES) {
        if (error_message) {
            *error_message = strdup("Too many context hashes (max 100)");
        }
        return NULL;
    }

    char** hashes = calloc(hash_count, sizeof(char*));
    if (!hashes) {
        if (error_message) {
            *error_message = strdup("Memory allocation failed");
        }
        return NULL;
    }

    size_t valid_count = 0;
    for (size_t i = 0; i < hash_count; i++) {
        json_t* hash_obj = json_array_get(context_hashes_obj, i);
        if (!json_is_string(hash_obj)) {
            log_this(SR_CONTEXT_HASHING, "Invalid hash at index %zu (not a string)", LOG_LEVEL_ALERT, 1, i);
            continue;
        }

        const char* hash_str = json_string_value(hash_obj);
        if (!chat_context_validate_hash(hash_str)) {
            log_this(SR_CONTEXT_HASHING, "Invalid hash format at index %zu", LOG_LEVEL_ALERT, 1, i);
            continue;
        }

        hashes[valid_count] = strdup(hash_str);
        if (!hashes[valid_count]) {
            // Cleanup on allocation failure
            for (size_t j = 0; j < valid_count; j++) {
                free(hashes[j]);
            }
            free(hashes);
            if (error_message) {
                *error_message = strdup("Memory allocation failed");
            }
            return NULL;
        }
        valid_count++;
    }

    if (valid_count == 0) {
        free(hashes);
        return NULL;
    }

    // Resize array to actual valid count + NULL terminator for safe counting
    char** resized = realloc(hashes, (valid_count + 1) * sizeof(char*));
    if (resized) {
        hashes = resized;
    }
    hashes[valid_count] = NULL;

    return hashes;
}



void chat_context_free_hash_array(char** hashes, size_t count) {
    if (!hashes) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        if (hashes[i]) {
            free(hashes[i]);
        }
    }
    free(hashes);
}


