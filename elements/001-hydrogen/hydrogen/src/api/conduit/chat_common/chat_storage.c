/*
 * Chat Storage Module - Content-Addressable Storage Implementation
 *
 * Phase 6: Conversation History with Content-Addressable Storage + Brotli
 */

#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <src/utils/utils_compression.h>
#include <src/utils/utils_crypto.h>
#include <src/logging/logging.h>

#include "chat_storage.h"

static const char* SR_CHAT_STORAGE = "CHAT_STORAGE";

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

bool chat_storage_compress_message(const char* message, size_t message_len,
                                    uint8_t** compressed_data, size_t* compressed_size) {
    if (!message || message_len == 0 || !compressed_data || !compressed_size) {
        log_this(SR_CHAT_STORAGE, "Invalid parameters for compression", LOG_LEVEL_ERROR, 0);
        return false;
    }

    bool result = compress_json_result(message, message_len, compressed_data, compressed_size);
    if (!result) {
        log_this(SR_CHAT_STORAGE, "Brotli compression failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(SR_CHAT_STORAGE, "Compressed message from %zu to %zu bytes (%.1f%%)",
             LOG_LEVEL_DEBUG, 3, message_len, *compressed_size,
             (double)*compressed_size / (double)message_len * 100.0);

    return true;
}

bool chat_storage_decompress_message(const uint8_t* compressed_data, size_t compressed_size,
                                      char** decompressed_data, size_t* decompressed_size) {
    if (!compressed_data || compressed_size == 0 || !decompressed_data || !decompressed_size) {
        log_this(SR_CHAT_STORAGE, "Invalid parameters for decompression", LOG_LEVEL_ERROR, 0);
        return false;
    }

    bool result = decompress_cached_result(compressed_data, compressed_size, decompressed_data, decompressed_size);
    if (!result) {
        log_this(SR_CHAT_STORAGE, "Brotli decompression failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(SR_CHAT_STORAGE, "Decompressed message from %zu to %zu bytes",
             LOG_LEVEL_DEBUG, 2, compressed_size, *decompressed_size);

    return true;
}

char* chat_storage_store_segment(const char* message, size_t message_len) {
    if (!message || message_len == 0) {
        log_this(SR_CHAT_STORAGE, "Invalid message for storage", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    char* hash = chat_storage_generate_hash(message, message_len);
    if (!hash) {
        log_this(SR_CHAT_STORAGE, "Failed to generate hash for message", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    if (chat_storage_segment_exists(hash)) {
        log_this(SR_CHAT_STORAGE, "Segment already exists, skipping storage (deduplication)", LOG_LEVEL_DEBUG, 1, hash);
        return hash;
    }

    uint8_t* compressed = NULL;
    size_t compressed_size = 0;

    bool compress_result = chat_storage_compress_message(message, message_len, &compressed, &compressed_size);
    if (!compress_result) {
        free(hash);
        log_this(SR_CHAT_STORAGE, "Failed to compress message", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    double compression_ratio = 0.0;
    if (message_len > 0) {
        compression_ratio = (double)compressed_size / (double)message_len;
    }

    log_this(SR_CHAT_STORAGE, "Would store segment with hash: %s (original: %zu, compressed: %zu, ratio: %.2f)",
             LOG_LEVEL_DEBUG, 4, hash, message_len, compressed_size, compression_ratio);

    free(compressed);

    return hash;
}

char* chat_storage_retrieve_segment(const char* segment_hash) {
    if (!segment_hash) {
        log_this(SR_CHAT_STORAGE, "Invalid segment hash for retrieval", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    if (!chat_storage_segment_exists(segment_hash)) {
        log_this(SR_CHAT_STORAGE, "Segment not found: %s", LOG_LEVEL_ERROR, 1, segment_hash);
        return NULL;
    }

    log_this(SR_CHAT_STORAGE, "Would retrieve segment: %s", LOG_LEVEL_DEBUG, 1, segment_hash);

    chat_storage_update_access(segment_hash);

    return NULL;
}

bool chat_storage_update_access(const char* segment_hash) {
    if (!segment_hash) {
        return false;
    }

    log_this(SR_CHAT_STORAGE, "Would update access metadata for: %s", LOG_LEVEL_DEBUG, 1, segment_hash);
    return true;
}

bool chat_storage_segment_exists(const char* segment_hash) {
    if (!segment_hash) {
        return false;
    }

    log_this(SR_CHAT_STORAGE, "Checking if segment exists: %s", LOG_LEVEL_DEBUG, 1, segment_hash);

    return false;
}

bool chat_storage_get_stats(const char* segment_hash, 
                            size_t* uncompressed_size, 
                            size_t* compressed_size,
                            double* compression_ratio,
                            int* access_count) {
    if (!segment_hash || !uncompressed_size || !compressed_size || 
        !compression_ratio || !access_count) {
        return false;
    }

    log_this(SR_CHAT_STORAGE, "Would get stats for segment: %s", LOG_LEVEL_DEBUG, 1, segment_hash);

    *uncompressed_size = 0;
    *compressed_size = 0;
    *compression_ratio = 0.0;
    *access_count = 0;

    return true;
}

void chat_storage_free_compressed(uint8_t* data) {
    if (data) {
        free(data);
    }
}

void chat_storage_free_decompressed(char* data) {
    if (data) {
        free(data);
    }
}

void chat_storage_free_hash(char* hash) {
    if (hash) {
        free(hash);
    }
}