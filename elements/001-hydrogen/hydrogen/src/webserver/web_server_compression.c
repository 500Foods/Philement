/*
 * Web Server Compression Implementation
 * 
 * Implements Brotli compression functionality for the web server:
 * - Checking if clients support Brotli
 * - Detecting pre-compressed files
 * - Compressing data on-the-fly
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// System headers
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

// Project headers
#include "web_server_compression.h"
#include "../logging/logging.h"
#include "../utils/utils_time.h"

// Default Brotli compression level (0-11, where 11 is highest)
// Default Brotli compression parameters
// Window size for Brotli (10-24)
#define BROTLI_WINDOW_SIZE 22

// Dynamic compression levels based on content size
#define BROTLI_SMALL_THRESHOLD 5120        // 5KB
#define BROTLI_MEDIUM_THRESHOLD 512000     // 500KB
#define BROTLI_LEVEL_SMALL 11              // Highest compression for small content
#define BROTLI_LEVEL_MEDIUM 6              // Medium compression for medium content
#define BROTLI_LEVEL_LARGE 4               // Lower compression for large content

bool client_accepts_brotli(struct MHD_Connection *connection) {
    // Get the Accept-Encoding header
    const char *encodings = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Accept-Encoding");
    
    // If no Accept-Encoding header or it's empty, client doesn't support compression
    if (!encodings || encodings[0] == '\0') {
        return false;
    }
    
    // Check if "br" is in the Accept-Encoding header
    // This is a simple check that doesn't consider quality values (q=)
    if (strstr(encodings, "br") != NULL) {
        return true;
    }
    
    return false;
}

bool brotli_file_exists(const char *file_path, char *br_file_path, size_t buffer_size) {
    // Check if file_path already ends with .br
    size_t len = strlen(file_path);
    if (len >= 3 && strcmp(file_path + len - 3, ".br") == 0) {
        // File already has .br extension, just copy the path
        if (br_file_path) {
            snprintf(br_file_path, buffer_size, "%s", file_path);
        }
        // Check if the file exists
        struct stat st;
        return (stat(file_path, &st) == 0 && S_ISREG(st.st_mode));
    }
    
    // Construct the .br file path
    if (br_file_path) {
        snprintf(br_file_path, buffer_size, "%s.br", file_path);
    }
    
    // Check if the .br file exists
    char temp_path[1024];
    if (!br_file_path) {
        snprintf(temp_path, sizeof(temp_path), "%s.br", file_path);
        br_file_path = temp_path;
    }
    
    struct stat st;
    return (stat(br_file_path, &st) == 0 && S_ISREG(st.st_mode));
}

bool compress_with_brotli(const uint8_t *input, size_t input_size, 
                         uint8_t **output, size_t *output_size) {
    if (!input || input_size == 0 || !output || !output_size) {
        log_this("WebCompression", "Invalid parameters for Brotli compression", LOG_LEVEL_ERROR);
        return false;
    }
    
    // Get the maximum compressed size needed
    *output_size = BrotliEncoderMaxCompressedSize(input_size);
    if (*output_size == 0) {
        log_this("WebCompression", "Failed to calculate max compressed size", LOG_LEVEL_ERROR);
        return false;
    }
    
    // Allocate buffer for compressed data
    *output = (uint8_t *)malloc(*output_size);
    if (!*output) {
        log_this("WebCompression", "Failed to allocate memory for compressed data", LOG_LEVEL_ERROR);
        return false;
    }
    
    // Start timing the compression operation
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Determine compression level based on input size
    int compression_level;
    if (input_size <= BROTLI_SMALL_THRESHOLD) {
        compression_level = BROTLI_LEVEL_SMALL;
    } else if (input_size <= BROTLI_MEDIUM_THRESHOLD) {
        compression_level = BROTLI_LEVEL_MEDIUM;
    } else {
        compression_level = BROTLI_LEVEL_LARGE;
    }
    
    // Compress the data
    BROTLI_BOOL result = BrotliEncoderCompress(
        compression_level,           // Quality (0-11)
        BROTLI_WINDOW_SIZE,          // LZ77 window size (10-24)
        BROTLI_DEFAULT_MODE,         // Mode (generic, text, font)
        input_size,                  // Input size
        input,                       // Input data
        output_size,                 // Output buffer size (in/out)
        *output                      // Output buffer
    );
    
    // End timing
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Calculate elapsed time in milliseconds with 3 decimal places
    double elapsed_ms = (end_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_ms += (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
    
    if (result != BROTLI_TRUE) {
        log_this("WebCompression", "Brotli compression failed", LOG_LEVEL_ERROR);
        free(*output);
        *output = NULL;
        *output_size = 0;
        return false;
    }
    
    // Log compression stats with detailed information including compression level
    float ratio = (float)(*output_size) / (float)input_size;
    float compression_percent = (1.0f - ratio) * 100.0f;
    
    char stats[256];
    snprintf(stats, sizeof(stats), 
             "Brotli(level=%d): %zu bytes → %zu bytes, ratio: %.2f%%, compression: %.2f%%, time: %.3f ms", 
             compression_level, input_size, *output_size, ratio * 100.0f, compression_percent, elapsed_ms);
    
    log_this("Brotli", stats, LOG_LEVEL_INFO);
    
    return true;
}

void add_brotli_header(struct MHD_Response *response) {
    if (!response) {
        return;
    }
    
    MHD_add_response_header(response, "Content-Encoding", "br");
    
    // Also add Vary header to indicate that responses may vary based on Accept-Encoding
    MHD_add_response_header(response, "Vary", "Accept-Encoding");
}