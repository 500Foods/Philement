/*
 * Chat Storage Module - Content-Addressable Storage with Brotli Compression
 *
 * Phase 6: Conversation History with Content-Addressable Storage + Brotli
 * Phase 7: Connected to QueryRefs #063 (store segment), #062 (retrieve), #067 (store chat), #068 (get chat)
 * Phase 9: Added LRU disk cache layer for hot segments
 * Provides storage and retrieval pipelines for conversation segments using
 * SHA-256 hashes as content addresses and Brotli compression.
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <src/hydrogen.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <jansson.h>

/* Forward declaration for LRU cache */
typedef struct ChatLRUCache ChatLRUCache;

#define CHAT_STORAGE_MAX_HASH_LEN    64
#define CHAT_STORAGE_MAX_MESSAGE_LEN 65536

typedef struct ConversationSegment {
    char* segment_hash;
    uint8_t* segment_content;
    size_t uncompressed_size;
    size_t compressed_size;
    double compression_ratio;
    time_t created_at;
    time_t last_accessed;
    int access_count;
} ConversationSegment;

typedef enum {
    CHAT_STORAGE_SUCCESS = 0,
    CHAT_STORAGE_ERROR_NULL_PARAMS,
    CHAT_STORAGE_ERROR_HASH_FAILED,
    CHAT_STORAGE_ERROR_COMPRESS_FAILED,
    CHAT_STORAGE_ERROR_DECOMPRESS_FAILED,
    CHAT_STORAGE_ERROR_DB_FAILED,
    CHAT_STORAGE_ERROR_NOT_FOUND,
    CHAT_STORAGE_ERROR_MEMORY
} ChatStorageResult;

char* chat_storage_generate_hash(const char* content, size_t length);
char* chat_storage_generate_hash_from_json(const char* json_string);

bool chat_storage_compress_message(const char* message, size_t message_len,
                                    uint8_t** compressed_data, size_t* compressed_size);
bool chat_storage_decompress_message(const uint8_t* compressed_data, size_t compressed_size,
                                      char** decompressed_data, size_t* decompressed_size);

char* chat_storage_store_segment(const char* database, const char* message, size_t message_len);

char* chat_storage_retrieve_segment(const char* database, const char* segment_hash);

bool chat_storage_update_access(const char* database, const char* segment_hash);

bool chat_storage_segment_exists(const char* database, const char* segment_hash);

/**
 * Batch retrieval of multiple segments by hash
 *
 * @param database Database name
 * @param segment_hashes Array of hash strings to retrieve
 * @param hash_count Number of hashes in the array
 * @return JSON array of found segments (caller must free), or NULL on error
 */
json_t* chat_storage_retrieve_segments_batch(const char* database,
                                              const char** segment_hashes,
                                              size_t hash_count);

/**
 * Pre-fetch segments for a conversation (conservative approach)
 * Pre-fetches the segment following the given hash in the conversation
 *
 * @param database Database name
 * @param segment_hash Current segment hash
 * @return true if pre-fetch was initiated, false otherwise
 */
bool chat_storage_prefetch_segment(const char* database, const char* segment_hash);

bool chat_storage_get_stats(const char* database, const char* segment_hash,
                            size_t* uncompressed_size,
                            size_t* compressed_size,
                            double* compression_ratio,
                            int* access_count);

char* chat_storage_store_chat(const char* database, const char* convos_ref,
                               const char** segment_hashes, size_t hash_count,
                               const char* engine_name, const char* model,
                               int tokens_prompt, int tokens_completion,
                               double cost_total, int user_id, int duration_ms,
                               const char* session_id);

json_t* chat_storage_get_chat(const char* database, int convos_id);

void chat_storage_free_compressed(uint8_t* data);
void chat_storage_free_decompressed(char* data);
void chat_storage_free_hash(char* hash);

/* Phase 12: Media Assets Storage */

/**
 * Store media asset in media_assets table
 *
 * @param database Database name
 * @param media_hash SHA-256 hash of media content
 * @param media_data Binary media data
 * @param media_size Size of media data in bytes
 * @param mime_type MIME type of media (can be NULL)
 * @return true on success, false on error
 */
bool chat_storage_store_media(const char* database, const char* media_hash,
                              const unsigned char* media_data, size_t media_size,
                              const char* mime_type);

/**
 * Retrieve media asset from media_assets table
 *
 * @param database Database name
 * @param media_hash SHA-256 hash of media content
 * @param media_data Output pointer to allocated media data (caller must free)
 * @param media_size Output size of media data
 * @param mime_type Output MIME type (caller must free, can be NULL if not stored)
 * @return true on success, false on error or not found
 */
bool chat_storage_retrieve_media(const char* database, const char* media_hash,
                                 unsigned char** media_data, size_t* media_size,
                                 char** mime_type);

/**
 * Resolve media:hash references in a JSON content array
 *
 * Parses a JSON array string representing multimodal content, finds any
 * image_url objects with URLs starting with "media:", retrieves the media
 * from storage, and replaces the URL with a data URL.
 *
 * @param database Database name for media storage
 * @param content_json JSON array string of content parts
 * @param resolved_json Output pointer to newly allocated JSON array string (caller must free)
 * @param error_message Output error message on failure (caller must free)
 * @return true on success, false on error
 */
bool chat_storage_resolve_media_in_content(const char* database,
                                           const char* content_json,
                                           char** resolved_json,
                                           char** error_message);

/* Phase 9: LRU Cache Management */

/**
 * Initialize LRU cache for a database
 *
 * @param database Database name
 * @param max_size_bytes Maximum cache size (0 for default)
 * @return true on success, false on error
 */
bool chat_storage_cache_init(const char* database, size_t max_size_bytes);

/**
 * Shutdown LRU cache for a database
 *
 * @param database Database name
 */
void chat_storage_cache_shutdown(const char* database);

/**
 * Get LRU cache for a database (creates if needed)
 *
 * @param database Database name
 * @return Cache instance, or NULL if cache not initialized
 */
ChatLRUCache* chat_storage_get_cache(const char* database);

/**
 * Get cache statistics for a database
 *
 * @param database Database name
 * @param hits Output for cache hits
 * @param misses Output for cache misses
 * @param hit_ratio Output for hit ratio
 * @return true on success, false on error
 */
bool chat_storage_cache_get_stats(const char* database,
                                  uint64_t* hits, uint64_t* misses, double* hit_ratio);

#endif // STORAGE_H