/*
 * Chat Context Hashing - Client-Server Optimization (Phase 8)
 *
 * Provides context hashing functionality for reducing bandwidth usage
 * by allowing clients to send hashes instead of full message content.
 * Server reconstructs context from hashes using QueryRef #062.
 */

#ifndef CONTEXT_HASHING_H
#define CONTEXT_HASHING_H

#include <src/hydrogen.h>
#include <stddef.h>
#include <stdbool.h>
#include <jansson.h>

#include "storage.h"

#define CHAT_CONTEXT_MAX_HASH_LEN    64
#define CHAT_CONTEXT_MAX_HASHES      100

/**
 * Context hash entry containing hash and optional reconstructed content
 */
typedef struct ContextHashEntry {
    char* hash;                      // SHA-256 hash of message content
    char* content;                   // Reconstructed content (NULL if not yet resolved)
    size_t content_length;           // Length of content
    bool from_cache;                 // True if retrieved from cache/database
    bool is_valid;                   // True if hash is valid and content resolved
} ContextHashEntry;

/**
 * Context hash resolution result
 */
typedef struct ContextHashResult {
    ContextHashEntry* entries;       // Array of resolved entries
    size_t entry_count;              // Number of entries
    size_t total_content_size;       // Total size of all resolved content
    size_t hashes_found;             // Number of hashes found in cache
    size_t hashes_missing;           // Number of hashes not found
    double bandwidth_saved_bytes;    // Estimated bandwidth saved
    double bandwidth_saved_percent;  // Estimated bandwidth saved percentage
} ContextHashResult;

/**
 * Generate SHA-256 hash for message content
 * 
 * @param content Message content to hash
 * @param content_length Length of content
 * @return Allocated hash string (base64url encoded SHA-256), or NULL on error
 * @note Caller must free the returned string using chat_context_free_hash()
 */
char* chat_context_hash_content(const char* content, size_t content_length);

/**
 * Generate SHA-256 hash for JSON string
 * 
 * @param json_string JSON string to hash
 * @return Allocated hash string (base64url encoded SHA-256), or NULL on error
 * @note Caller must free the returned string using chat_context_free_hash()
 */
char* chat_context_hash_json(const char* json_string);

/**
 * Validate that a context hash string is properly formatted
 * 
 * @param hash Hash string to validate (base64url encoded, 44 chars for SHA-256)
 * @return true if valid format, false otherwise
 */
bool chat_context_validate_hash(const char* hash);

/**
 * Parse context_hashes array from chat request
 * 
 * @param request_json The chat request JSON object
 * @param error_message Output parameter for error message (allocated on failure)
 * @return Allocated array of hash strings, or NULL on error/no hashes
 * @note Caller must free the array and strings using chat_context_free_hash_array()
 */
char** chat_context_parse_request_hashes(json_t* request_json, char** error_message);

/**
 * Resolve context hashes to actual content using storage layer
 * 
 * @param database Database name for storage lookup
 * @param hashes Array of hash strings to resolve
 * @param hash_count Number of hashes in array
 * @return ContextHashResult with resolved entries, or NULL on error
 * @note Caller must free result using chat_context_free_result()
 */
ContextHashResult* chat_context_resolve_hashes(const char* database,
                                               const char** hashes,
                                               size_t hash_count);

/**
 * Reconstruct conversation from context hashes, with fallback for missing
 * 
 * @param database Database name for storage lookup
 * @param context_hashes Array of context hash strings
 * @param context_hash_count Number of context hashes
 * @param fallback_messages Full messages array from request (for fallback)
 * @param fallback_count Number of fallback messages
 * @return Reconstructed messages JSON array, or NULL on error
 * @note Returns JSON array of message objects with role and content
 * @note Caller must free the returned JSON array using json_decref()
 */
json_t* chat_context_reconstruct_conversation(const char* database,
                                               const char** context_hashes,
                                               size_t context_hash_count,
                                               json_t* fallback_messages,
                                               size_t fallback_count);

/**
 * Calculate bandwidth savings from using context hashes
 * 
 * @param result ContextHashResult from resolution
 * @param original_request_size Size of request with full messages
 * @return Percentage of bandwidth saved (0.0 to 100.0)
 */
double chat_context_calculate_bandwidth_savings(const ContextHashResult* result,
                                                 size_t original_request_size);

/**
 * Estimate request size with full messages vs hashes
 * 
 * @param messages JSON array of messages
 * @param hash_count Number of hashes that could replace messages
 * @return Estimated size difference in bytes
 */
size_t chat_context_estimate_size_savings(json_t* messages, size_t hash_count);

/**
 * Free a hash string allocated by this module
 * 
 * @param hash Hash string to free
 */
void chat_context_free_hash(char* hash);

/**
 * Free an array of hashes allocated by this module
 * 
 * @param hashes Array of hash strings
 * @param count Number of hashes in array
 */
void chat_context_free_hash_array(char** hashes, size_t count);

/**
 * Free a ContextHashResult
 * 
 * @param result Result to free
 */
void chat_context_free_result(ContextHashResult* result);

#endif // CONTEXT_HASHING_H
