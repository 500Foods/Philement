/*
 * Compression Utilities
 *
 * Provides Brotli compression and decompression functions for caching query results.
 * Used by the Conduit Service to compress cached JSON responses for memory efficiency.
 */

#ifndef UTILS_COMPRESSION_H
#define UTILS_COMPRESSION_H

#include <stddef.h>

/**
 * @brief Compress a JSON result string using Brotli compression
 * @param json_data The JSON string to compress
 * @param json_size Size of the JSON string in bytes
 * @param compressed_data Output parameter for compressed data buffer (allocated by function)
 * @param compressed_size Output parameter for size of compressed data
 * @return true on success, false on failure
 * @note Caller must free compressed_data when done
 */
bool compress_json_result(const char* json_data, size_t json_size,
                         unsigned char** compressed_data, size_t* compressed_size);

/**
 * @brief Decompress cached result data using Brotli decompression
 * @param compressed_data The compressed data buffer
 * @param compressed_size Size of the compressed data in bytes
 * @param decompressed_data Output parameter for decompressed JSON string (allocated by function)
 * @param decompressed_size Output parameter for size of decompressed data
 * @return true on success, false on failure
 * @note Caller must free decompressed_data when done
 */
bool decompress_cached_result(const unsigned char* compressed_data, size_t compressed_size,
                             char** decompressed_data, size_t* decompressed_size);

#endif // UTILS_COMPRESSION_H