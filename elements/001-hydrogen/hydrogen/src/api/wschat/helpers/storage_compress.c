/*
 * Chat Storage Compression Functions
 *
 * Provides Brotli compression/decompression wrappers for chat messages.
 */

#include <src/hydrogen.h>

#include <src/utils/utils_compression.h>
#include <src/logging/logging.h>

#include "storage_compress.h"

static const char* SR_CHAT_STORAGE_COMPRESS = "CHAT_STORAGE";

bool chat_storage_compress_message(const char* message, size_t message_len,
                                    uint8_t** compressed_data, size_t* compressed_size) {
    if (!message || message_len == 0 || !compressed_data || !compressed_size) {
        log_this(SR_CHAT_STORAGE_COMPRESS, "Invalid parameters for compression", LOG_LEVEL_ERROR, 0);
        return false;
    }

    bool result = compress_json_result(message, message_len, compressed_data, compressed_size);
    if (!result) {
        log_this(SR_CHAT_STORAGE_COMPRESS, "Brotli compression failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(SR_CHAT_STORAGE_COMPRESS, "Compressed message from %zu to %zu bytes (%.1f%%)",
             LOG_LEVEL_DEBUG, 3, message_len, *compressed_size,
             (double)*compressed_size / (double)message_len * 100.0);

    return true;
}

bool chat_storage_decompress_message(const uint8_t* compressed_data, size_t compressed_size,
                                      char** decompressed_data, size_t* decompressed_size) {
    if (!compressed_data || compressed_size == 0 || !decompressed_data || !decompressed_size) {
        log_this(SR_CHAT_STORAGE_COMPRESS, "Invalid parameters for decompression", LOG_LEVEL_ERROR, 0);
        return false;
    }

    bool result = decompress_cached_result(compressed_data, compressed_size, decompressed_data, decompressed_size);
    if (!result) {
        log_this(SR_CHAT_STORAGE_COMPRESS, "Brotli decompression failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(SR_CHAT_STORAGE_COMPRESS, "Decompressed message from %zu to %zu bytes",
             LOG_LEVEL_DEBUG, 2, compressed_size, *decompressed_size);

    return true;
}
