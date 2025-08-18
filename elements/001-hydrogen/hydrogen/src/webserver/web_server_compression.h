/*
 * Web Server Compression Utilities
 * 
 * Provides compression functionality for the web server:
 * - Brotli compression detection and handling
 * - Pre-compressed file detection
 * - On-the-fly API response compression
 */

#ifndef WEB_SERVER_COMPRESSION_H
#define WEB_SERVER_COMPRESSION_H

// System headers
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <microhttpd.h>
#include <brotli/encode.h>
#include <brotli/decode.h>

/**
 * Check if the client supports Brotli compression
 * 
 * @param connection The MHD connection to check
 * @return true if the client accepts Brotli encoding, false otherwise
 */
bool client_accepts_brotli(struct MHD_Connection *connection);

/**
 * Check if a Brotli-compressed version of a file exists
 * 
 * @param file_path The path to the original file
 * @param br_file_path Buffer to store the path to the .br file (if it exists)
 * @param buffer_size Size of the br_file_path buffer
 * @return true if a .br file exists, false otherwise
 */
bool brotli_file_exists(const char *file_path, char *br_file_path, size_t buffer_size);

/**
 * Compress data using Brotli
 * 
 * @param input Data to compress
 * @param input_size Size of input data
 * @param output Buffer to store compressed data
 * @param output_size Pointer to store the size of compressed data
 * @return true if compression was successful, false otherwise
 */
bool compress_with_brotli(const uint8_t *input, size_t input_size, 
                         uint8_t **output, size_t *output_size);

/**
 * Add appropriate content encoding header for Brotli compression
 * 
 * @param response The MHD response to add the header to
 */
void add_brotli_header(struct MHD_Response *response);

#endif // WEB_SERVER_COMPRESSION_H
