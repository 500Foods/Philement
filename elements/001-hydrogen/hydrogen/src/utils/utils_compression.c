// Global includes
#include <src/hydrogen.h>

// Brotli compression libraries
#include <brotli/encode.h>
#include <brotli/decode.h>

// Standard C libraries
#include <stdlib.h>
#include <string.h>

// Local includes
#include "utils_compression.h"

// Public interface declarations
bool compress_json_result(const char* json_data, size_t json_size,
                         unsigned char** compressed_data, size_t* compressed_size);
bool decompress_cached_result(const unsigned char* compressed_data, size_t compressed_size,
                             char** decompressed_data, size_t* decompressed_size);

/**
 * @brief Compress a JSON result string using Brotli compression
 */
bool compress_json_result(const char* json_data, size_t json_size,
                         unsigned char** compressed_data, size_t* compressed_size) {
    if (!json_data || json_size == 0 || !compressed_data || !compressed_size) {
        return false;
    }

    // Create Brotli encoder instance
    BrotliEncoderState* encoder = BrotliEncoderCreateInstance(NULL, NULL, NULL);
    if (!encoder) {
        log_this(SR_API, "Failed to create Brotli encoder instance", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Set compression quality (6 is a good balance of speed vs compression)
    BrotliEncoderSetParameter(encoder, BROTLI_PARAM_QUALITY, 6);

    // Estimate compressed size (Brotli typically compresses to 20-30% of original)
    size_t estimated_size = json_size / 3;  // Conservative estimate
    if (estimated_size < 1024) {
        estimated_size = 1024;  // Minimum buffer size
    }

    *compressed_data = malloc(estimated_size);
    if (!*compressed_data) {
        BrotliEncoderDestroyInstance(encoder);
        log_this(SR_API, "Failed to allocate compression buffer", LOG_LEVEL_ERROR, 0);
        return false;
    }

    size_t available_out = estimated_size;
    uint8_t* next_out = *compressed_data;
    BROTLI_BOOL result = BROTLI_FALSE;

    // Compress the data
    result = BrotliEncoderCompressStream(encoder,
                                        BROTLI_OPERATION_FINISH,
                                        &json_size, (const uint8_t**)&json_data,
                                        &available_out, &next_out, NULL);

    if (result == BROTLI_FALSE) {
        free(*compressed_data);
        *compressed_data = NULL;
        BrotliEncoderDestroyInstance(encoder);
        log_this(SR_API, "Brotli compression failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Calculate actual compressed size
    *compressed_size = estimated_size - available_out;

    BrotliEncoderDestroyInstance(encoder);

    log_this(SR_API, "Compressed JSON from %zu to %zu bytes (%.1f%%)",
             LOG_LEVEL_DEBUG, 3, json_size, *compressed_size,
             (double)*compressed_size / (double)json_size * 100.0);

    return true;
}

/**
 * @brief Decompress cached result data using Brotli decompression
 */
bool decompress_cached_result(const unsigned char* compressed_data, size_t compressed_size,
                             char** decompressed_data, size_t* decompressed_size) {
    if (!compressed_data || compressed_size == 0 || !decompressed_data || !decompressed_size) {
        return false;
    }

    // Create Brotli decoder instance
    BrotliDecoderState* decoder = BrotliDecoderCreateInstance(NULL, NULL, NULL);
    if (!decoder) {
        log_this(SR_API, "Failed to create Brotli decoder instance", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Estimate decompressed size (assume 3x compression ratio)
    size_t estimated_size = compressed_size * 3;
    if (estimated_size < 1024) {
        estimated_size = 1024;  // Minimum buffer size
    }

    *decompressed_data = malloc(estimated_size + 1);  // +1 for null terminator
    if (!*decompressed_data) {
        BrotliDecoderDestroyInstance(decoder);
        log_this(SR_API, "Failed to allocate decompression buffer", LOG_LEVEL_ERROR, 0);
        return false;
    }

    size_t available_out = estimated_size;
    uint8_t* next_out = (uint8_t*)*decompressed_data;
    size_t available_in = compressed_size;
    const uint8_t* next_in = compressed_data;

    BrotliDecoderResult result = BrotliDecoderDecompressStream(decoder,
                                                              &available_in, &next_in,
                                                              &available_out, &next_out, NULL);

    if (result != BROTLI_DECODER_RESULT_SUCCESS) {
        free(*decompressed_data);
        *decompressed_data = NULL;
        BrotliDecoderDestroyInstance(decoder);
        log_this(SR_API, "Brotli decompression failed with result: %d", LOG_LEVEL_ERROR, 1, result);
        return false;
    }

    // Calculate actual decompressed size
    *decompressed_size = estimated_size - available_out;

    // Null terminate the string
    (*decompressed_data)[*decompressed_size] = '\0';

    BrotliDecoderDestroyInstance(decoder);

    log_this(SR_API, "Decompressed data from %zu to %zu bytes",
             LOG_LEVEL_DEBUG, 2, compressed_size, *decompressed_size);

    return true;
}