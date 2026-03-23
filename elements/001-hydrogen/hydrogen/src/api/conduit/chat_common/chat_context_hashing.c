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

#include "chat_context_hashing.h"
#include "chat_storage.h"

static const char* SR_CONTEXT_HASHING = "CONTEXT_HASHING";

char* chat_context_hash_content(const char* content, size_t content_length) {
    if (!content || content_length == 0) {
        log_this(SR_CONTEXT_HASHING, "Invalid parameters for hash_content", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    return chat_storage_generate_hash(content, content_length);
}

char* chat_context_hash_json(const char* json_string) {
    if (!json_string) {
        log_this(SR_CONTEXT_HASHING, "Invalid parameters for hash_json", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    return chat_storage_generate_hash_from_json(json_string);
}

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

    // Resize array to actual valid count
    char** resized = realloc(hashes, valid_count * sizeof(char*));
    if (resized) {
        hashes = resized;
    }

    return hashes;
}

ContextHashResult* chat_context_resolve_hashes(const char* database,
                                               const char** hashes,
                                               size_t hash_count) {
    if (!database || !hashes || hash_count == 0) {
        log_this(SR_CONTEXT_HASHING, "Invalid parameters for resolve_hashes", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    ContextHashResult* result = calloc(1, sizeof(ContextHashResult));
    if (!result) {
        return NULL;
    }

    result->entries = calloc(hash_count, sizeof(ContextHashEntry));
    if (!result->entries) {
        free(result);
        return NULL;
    }

    result->entry_count = hash_count;

    for (size_t i = 0; i < hash_count; i++) {
        ContextHashEntry* entry = &result->entries[i];
        entry->hash = strdup(hashes[i]);
        entry->content = NULL;
        entry->content_length = 0;
        entry->from_cache = false;
        entry->is_valid = false;

        if (!entry->hash) {
            continue;
        }

        // Try to retrieve from storage
        char* content = chat_storage_retrieve_segment(database, entry->hash);
        if (content) {
            entry->content = content;
            entry->content_length = strlen(content);
            entry->from_cache = true;
            entry->is_valid = true;

            result->hashes_found++;
            result->total_content_size += entry->content_length;
        } else {
            result->hashes_missing++;
            log_this(SR_CONTEXT_HASHING, "Hash not found in storage: %s", LOG_LEVEL_DEBUG, 1, entry->hash);
        }
    }

    // Calculate bandwidth savings
    if (result->hashes_found > 0) {
        // Each hash is 43 chars + JSON overhead (~10 chars) = ~53 bytes
        size_t hash_overhead = result->hashes_found * 53;
        if (result->total_content_size > hash_overhead) {
            result->bandwidth_saved_bytes = (double)(result->total_content_size - hash_overhead);
            result->bandwidth_saved_percent = (result->bandwidth_saved_bytes / 
                                              (double)result->total_content_size) * 100.0;
        }
    }

    log_this(SR_CONTEXT_HASHING, "Resolved %zu/%zu hashes (saved %.1f%% bandwidth)",
             LOG_LEVEL_DEBUG, 3, result->hashes_found, hash_count, result->bandwidth_saved_percent);

    return result;
}

json_t* chat_context_reconstruct_conversation(const char* database,
                                               const char** context_hashes,
                                               size_t context_hash_count,
                                               json_t* fallback_messages,
                                               size_t fallback_count) {
    if (!database || !fallback_messages) {
        log_this(SR_CONTEXT_HASHING, "Invalid parameters for reconstruct_conversation", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    json_t* reconstructed = json_array();
    if (!reconstructed) {
        return NULL;
    }

    // First, try to resolve context hashes if provided
    ContextHashResult* hash_result = NULL;
    if (context_hashes && context_hash_count > 0) {
        hash_result = chat_context_resolve_hashes(database, context_hashes, context_hash_count);
    }

    // If we have context hashes, use them to reconstruct
    if (hash_result && hash_result->hashes_found > 0) {
        size_t resolved_index = 0;
        for (size_t i = 0; i < hash_result->entry_count; i++) {
            ContextHashEntry* entry = &hash_result->entries[i];
            
            if (entry->is_valid && entry->content) {
                // Parse the content as JSON message
                json_error_t error;
                json_t* msg = json_loads(entry->content, 0, &error);
                if (msg && json_is_object(msg)) {
                    json_array_append_new(reconstructed, msg);
                    resolved_index++;
                } else {
                    if (msg) json_decref(msg);
                    log_this(SR_CONTEXT_HASHING, "Failed to parse content for hash: %s", 
                             LOG_LEVEL_ALERT, 1, entry->hash);
                }
            }
        }
        
        log_this(SR_CONTEXT_HASHING, "Reconstructed %zu messages from context hashes",
                 LOG_LEVEL_DEBUG, 1, resolved_index);
    }

    // Fill in any gaps from fallback messages
    // Strategy: Use hashes for available content, fallback for missing
    // Note: fallback_messages is guaranteed non-NULL due to early return check at line 220
    if (hash_result && hash_result->hashes_missing > 0) {
        size_t reconstructed_count = json_array_size(reconstructed);
        
        for (size_t i = 0; i < fallback_count && reconstructed_count < CHAT_CONTEXT_MAX_HASHES; i++) {
            json_t* fallback_msg = json_array_get(fallback_messages, i);
            if (!json_is_object(fallback_msg)) continue;

            // Check if this position needs a fallback
            // For now, append missing messages at the end
            // A more sophisticated approach would match by content hash
            json_t* content_obj = json_object_get(fallback_msg, "content");
            if (content_obj) {
                json_t* new_msg = json_deep_copy(fallback_msg);
                json_array_append_new(reconstructed, new_msg);
            }
        }
    }

    // If no context hashes or all failed, use fallback messages
    if (json_array_size(reconstructed) == 0 && fallback_messages) {
        log_this(SR_CONTEXT_HASHING, "No context hashes resolved, using fallback messages",
                 LOG_LEVEL_DEBUG, 0);
        
        for (size_t i = 0; i < fallback_count; i++) {
            json_t* msg = json_array_get(fallback_messages, i);
            if (json_is_object(msg)) {
                json_t* new_msg = json_deep_copy(msg);
                json_array_append_new(reconstructed, new_msg);
            }
        }
    }

    chat_context_free_result(hash_result);

    if (json_array_size(reconstructed) == 0) {
        json_decref(reconstructed);
        return NULL;
    }

    return reconstructed;
}

double chat_context_calculate_bandwidth_savings(const ContextHashResult* result,
                                                 size_t original_request_size) {
    if (!result || result->hashes_found == 0 || original_request_size == 0) {
        return 0.0;
    }

    // Hash representation: 43 chars + JSON overhead (~10 chars) = ~53 bytes per hash
    size_t hash_representation = result->hashes_found * 53;
    
    // Content size that was resolved from hashes
    size_t content_replaced = result->total_content_size;
    
    if (content_replaced <= hash_representation) {
        return 0.0;
    }

    size_t saved = content_replaced - hash_representation;
    double percentage = ((double)saved / (double)original_request_size) * 100.0;

    return (percentage > 0.0) ? percentage : 0.0;
}

size_t chat_context_estimate_size_savings(json_t* messages, size_t hash_count) {
    if (!messages || !json_is_array(messages) || hash_count == 0) {
        return 0;
    }

    size_t total_message_size = 0;
    size_t array_size = json_array_size(messages);
    size_t hashes_to_use = (hash_count < array_size) ? hash_count : array_size;

    for (size_t i = 0; i < array_size; i++) {
        json_t* msg = json_array_get(messages, i);
        if (json_is_object(msg)) {
            char* msg_str = json_dumps(msg, JSON_COMPACT);
            if (msg_str) {
                total_message_size += strlen(msg_str);
                free(msg_str);
            }
        }
    }

    // Estimated size if we used hashes instead
    // Each hash: ~53 bytes (43 + JSON overhead)
    // Each message: variable size
    size_t hash_size = hashes_to_use * 53;
    
    if (total_message_size > hash_size) {
        return total_message_size - hash_size;
    }

    return 0;
}

void chat_context_free_hash(char* hash) {
    if (hash) {
        free(hash);
    }
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

void chat_context_free_result(ContextHashResult* result) {
    if (!result) {
        return;
    }

    if (result->entries) {
        for (size_t i = 0; i < result->entry_count; i++) {
            if (result->entries[i].hash) {
                free(result->entries[i].hash);
            }
            if (result->entries[i].content) {
                free(result->entries[i].content);
            }
        }
        free(result->entries);
    }

    free(result);
}
