/*
 * Chat Storage Module - Content-Addressable Storage with Brotli Compression
 *
 * Phase 6: Conversation History with Content-Addressable Storage + Brotli
 * Provides storage and retrieval pipelines for conversation segments using
 * SHA-256 hashes as content addresses and Brotli compression.
 */

#ifndef CHAT_STORAGE_H
#define CHAT_STORAGE_H

#include <src/hydrogen.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Maximum sizes for conversation storage
#define CHAT_STORAGE_MAX_HASH_LEN    64
#define CHAT_STORAGE_MAX_MESSAGE_LEN 65536

// Conversation segment structure
typedef struct ConversationSegment {
    char* segment_hash;                  // SHA-256 hash (content address)
    uint8_t* segment_content;           // Brotli-compressed content
    size_t uncompressed_size;           // Original size in bytes
    size_t compressed_size;             // Compressed size in bytes
    double compression_ratio;           // Compression ratio for analytics
    time_t created_at;                  // Timestamp when segment was stored
    time_t last_accessed;               // Timestamp of last retrieval
    int access_count;                   // Number of times accessed
} ConversationSegment;

// Storage result codes
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

// Function prototypes

// Hash generation
char* chat_storage_generate_hash(const char* content, size_t length);
char* chat_storage_generate_hash_from_json(const char* json_string);

// Compression wrappers
bool chat_storage_compress_message(const char* message, size_t message_len,
                                    uint8_t** compressed_data, size_t* compressed_size);
bool chat_storage_decompress_message(const uint8_t* compressed_data, size_t compressed_size,
                                      char** decompressed_data, size_t* decompressed_size);

// Storage pipeline - store a message segment
// Returns the hash on success, NULL on failure
char* chat_storage_store_segment(const char* message, size_t message_len);

// Retrieval pipeline - retrieve and decompress a message by hash
// Returns the decompressed message on success, NULL on failure
char* chat_storage_retrieve_segment(const char* segment_hash);

// Update access metadata on retrieval
bool chat_storage_update_access(const char* segment_hash);

// Check if a segment already exists (for deduplication)
bool chat_storage_segment_exists(const char* segment_hash);

// Get storage statistics for a segment
bool chat_storage_get_stats(const char* segment_hash, 
                            size_t* uncompressed_size, 
                            size_t* compressed_size,
                            double* compression_ratio,
                            int* access_count);

// Cleanup
void chat_storage_free_compressed(uint8_t* data);
void chat_storage_free_decompressed(char* data);
void chat_storage_free_hash(char* hash);

#endif // CHAT_STORAGE_H