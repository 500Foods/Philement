/*
 * Chat Storage Compression Functions
 *
 * Provides Brotli compression/decompression wrappers for chat messages.
 */

#ifndef STORAGE_COMPRESS_H
#define STORAGE_COMPRESS_H

#include <src/hydrogen.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Compress a message using Brotli
 * @param message Input message data
 * @param message_len Length of input message
 * @param compressed_data Output pointer to compressed data (caller must free)
 * @param compressed_size Output size of compressed data
 * @return true on success, false on error
 */
bool chat_storage_compress_message(const char* message, size_t message_len,
                                    uint8_t** compressed_data, size_t* compressed_size);

/**
 * @brief Decompress a message using Brotli
 * @param compressed_data Input compressed data
 * @param compressed_size Size of compressed data
 * @param decompressed_data Output pointer to decompressed message (caller must free)
 * @param decompressed_size Output size of decompressed data
 * @return true on success, false on error
 */
bool chat_storage_decompress_message(const uint8_t* compressed_data, size_t compressed_size,
                                      char** decompressed_data, size_t* decompressed_size);

#endif // STORAGE_COMPRESS_H
