/*
 * Payload Cache Implementation
 */

// Project includes
#include <src/hydrogen.h>
#include <src/payload/payload.h>
#include <src/payload/payload_cache.h>

// Forward declarations for internal functions
// parse_tar_into_cache is now public for unit testing

// Forward declarations for functions from payload.c
bool extract_payload(const char *executable_path, const AppConfig *config,
                    const char *marker, PayloadData *payload);
bool decrypt_payload(const uint8_t *encrypted_data, size_t encrypted_size,
                          const char *private_key_b64, uint8_t **decrypted_data,
                          size_t *decrypted_size);
bool process_payload_data(const PayloadData *payload);
void free_payload(PayloadData *payload);

// Global payload cache
PayloadCache global_payload_cache = {0};

/**
 * Initialize the global payload cache
 */
bool initialize_payload_cache(void) {
    memset(&global_payload_cache, 0, sizeof(PayloadCache));
    global_payload_cache.is_initialized = true;
    log_this(SR_PAYLOAD, SR_PAYLOAD " Cache initialization", LOG_LEVEL_DEBUG, 0);
    return true;
}

/**
 * Load payload into the global cache
 */
bool load_payload_cache(const AppConfig *config, const char *marker) {
    if (!global_payload_cache.is_initialized) {
        log_this(SR_PAYLOAD, "― " SR_PAYLOAD " Cache not initialized", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Get executable path
    char *executable_path = get_executable_path();
    if (!executable_path) {
        log_this(SR_PAYLOAD, "― Failed to get executable path", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Extract and decrypt the payload
    PayloadData payload = {0};
    bool success = extract_payload(executable_path, config, marker, &payload);
    free(executable_path);

    if (!success) {
        log_this(SR_PAYLOAD, "― Failed to extract payload from executable", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Process (decompress) the payload and load into cache
    success = process_payload_tar_cache_from_data(payload.data, payload.size);

    // Free the temporary payload data
    free_payload(&payload);

    if (success) {
        global_payload_cache.is_available = true;
//        log_this(SR_PAYLOAD, "― " SR_PAYLOAD " Cache loaded", LOG_LEVEL_STATE, 0);
    } else {
        log_this(SR_PAYLOAD, "Failed to process " SR_PAYLOAD " into Cache", LOG_LEVEL_ERROR, 0);
    }

    return success;
}

/**
 * Check if payload cache is available
 */
bool is_payload_cache_available(void) {
    return global_payload_cache.is_initialized && global_payload_cache.is_available;
}

/**
 * Get payload files by prefix filter
 */
bool get_payload_files_by_prefix(const char *prefix, PayloadFile **files,
                                size_t *num_files, size_t *capacity) {
    if (!is_payload_cache_available() || !files || !num_files || !capacity) {
        return false;
    }

    // Copy the requested subset
    if (!prefix || strcmp(prefix, "") == 0) {
        // Return all files
        *num_files = global_payload_cache.num_files;
        *capacity = global_payload_cache.capacity;
        *files = calloc(global_payload_cache.num_files, sizeof(PayloadFile));

        if (!*files) {
            return false;
        }

        for (size_t i = 0; i < global_payload_cache.num_files; i++) {
            (*files)[i].name = strdup(global_payload_cache.files[i].name);
            if (global_payload_cache.files[i].data) {
                (*files)[i].data = malloc(global_payload_cache.files[i].size);
                if ((*files)[i].data) {
                    memcpy((*files)[i].data, global_payload_cache.files[i].data, global_payload_cache.files[i].size);
                }
            }
            (*files)[i].size = global_payload_cache.files[i].size;
            (*files)[i].is_compressed = global_payload_cache.files[i].is_compressed;
        }
    } else {
        // Filter by prefix
        size_t prefix_len = strlen(prefix);
        size_t matching_count = 0;

        // Count matching files
        for (size_t i = 0; i < global_payload_cache.num_files; i++) {
            if (global_payload_cache.files[i].name &&
                strncmp(global_payload_cache.files[i].name, prefix, prefix_len) == 0) {
                matching_count++;
            }
        }

        if (matching_count == 0) {
            *files = NULL;
            *num_files = 0;
            *capacity = 0;
            return true; // Success but no matching files
        }

        // Allocate space for matching files
        *files = calloc(matching_count, sizeof(PayloadFile));
        if (!*files) {
            return false;
        }

        *capacity = matching_count;
        *num_files = 0;

        // Copy matching files
        for (size_t i = 0; i < global_payload_cache.num_files; i++) {
            if (global_payload_cache.files[i].name &&
                strncmp(global_payload_cache.files[i].name, prefix, prefix_len) == 0) {
                (*files)[*num_files].name = strdup(global_payload_cache.files[i].name);
                if (global_payload_cache.files[i].data) {
                    (*files)[*num_files].data = malloc(global_payload_cache.files[i].size);
                    if ((*files)[*num_files].data) {
                        memcpy((*files)[*num_files].data, global_payload_cache.files[i].data, global_payload_cache.files[i].size);
                    }
                }
                (*files)[*num_files].size = global_payload_cache.files[i].size;
                (*files)[*num_files].is_compressed = global_payload_cache.files[i].is_compressed;
                (*num_files)++;
            }
        }
    }

    return true;
}

/**
 * Process payload tar cache from PayloadData
 */
bool process_payload_tar_cache(const PayloadData *payload_data) {
    // Not yet implemented - always return false for compilation
    (void)payload_data;
    return false;
}

/**
 * Process payload tar cache from raw data
 */
bool process_payload_tar_cache_from_data(const uint8_t *tar_data, size_t tar_size) {
    // Check input parameters
    if (!tar_data || tar_size == 0) {
        log_this(SR_PAYLOAD, "Invalid payload data for processing", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Check if payload is compressed (assume Brotli compression)
    // For now, always assume compressed since that's the current implementation
    // log_this(SR_PAYLOAD, "― Processing compressed payload: %zu bytes", LOG_LEVEL_STATE, 1, tar_size);

    // Use Brotli streaming API for decompression
    BrotliDecoderState* decoder = BrotliDecoderCreateInstance(NULL, NULL, NULL);
    if (!decoder) {
        log_this(SR_PAYLOAD, "Failed to create Brotli decoder", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Initial buffer size - we'll grow it if needed
    size_t buffer_size = tar_size * 4;  // Start with 4x the compressed size
    uint8_t* decompressed_data = malloc(buffer_size);
    if (!decompressed_data) {
        log_this(SR_PAYLOAD, "Failed to allocate memory for decompressed data", LOG_LEVEL_ERROR, 0);
        BrotliDecoderDestroyInstance(decoder);
        return false;
    }

    // Set up input/output parameters
    const uint8_t* next_in = tar_data;
    size_t available_in = tar_size;
    uint8_t* next_out = decompressed_data;
    size_t available_out = buffer_size;
    size_t total_out = 0;

    // Decompress until done
    BrotliDecoderResult result;
    do {
        result = BrotliDecoderDecompressStream(
            decoder,
            &available_in, &next_in,
            &available_out, &next_out,
            &total_out);

        if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
            // Need more output space
            size_t current_position = (size_t)(next_out - decompressed_data);
            buffer_size *= 2;
            uint8_t* new_buffer = realloc(decompressed_data, buffer_size);
            if (!new_buffer) {
                log_this(SR_PAYLOAD, "Failed to resize decompression buffer", LOG_LEVEL_ERROR, 0);
                free(decompressed_data);
                BrotliDecoderDestroyInstance(decoder);
                return false;
            }

            decompressed_data = new_buffer;
            next_out = decompressed_data + current_position;
            available_out = buffer_size - current_position;
        } else if (result == BROTLI_DECODER_RESULT_ERROR) {
            log_this(SR_PAYLOAD, "Brotli decompression error: %s", LOG_LEVEL_ERROR, 1, BrotliDecoderErrorString(BrotliDecoderGetErrorCode(decoder)));
            free(decompressed_data);
            BrotliDecoderDestroyInstance(decoder);
            return false;
        }
    } while (result != BROTLI_DECODER_RESULT_SUCCESS);

    // Clean up decoder
    BrotliDecoderDestroyInstance(decoder);

    log_this(SR_PAYLOAD, "― Decompressed size:    %'zu bytes", LOG_LEVEL_DEBUG, 1, total_out);

    // Parse the decompressed tar data
    bool success = parse_tar_into_cache(decompressed_data, total_out);
    free(decompressed_data);

    return success;
}

/**
 * List contents of tar file
 */
void list_tar_contents(const uint8_t *tar_data, size_t tar_size) {
    // Not yet implemented - dummy implementation for compilation
    (void)tar_data;
    (void)tar_size;
}

/**
 * Parse tar data and store files in global cache
 */
// Comparator function for sorting files by name
int compare_files(const void *a, const void *b) {
    const PayloadFile *fa = (const PayloadFile *)a;
    const PayloadFile *fb = (const PayloadFile *)b;
    return strcmp(fa->name, fb->name);
}

bool parse_tar_into_cache(const uint8_t *tar_data, size_t tar_size) {
    if (!tar_data || tar_size < 512) {
        log_this(SR_PAYLOAD, "Invalid tar data or size too small", LOG_LEVEL_ERROR, 0);
        return false;
    }

    size_t pos = 0;
    size_t initial_capacity = 16;
    PayloadFile *temp_files = calloc(initial_capacity, sizeof(PayloadFile));

    if (!temp_files) {
        log_this(SR_PAYLOAD, "Failed to allocate temporary file array", LOG_LEVEL_ERROR, 0);
        return false;
    }

    size_t file_count = 0;
    size_t total_processed = 0;

    // Process tar entries
    while (pos + 512 <= tar_size) {
        const uint8_t *header = tar_data + pos;

        // Check for end of archive (empty block)
        bool is_empty = true;
        for (size_t i = 0; i < 512; i++) {
            if (header[i] != 0) {
                is_empty = false;
                break;
            }
        }
        if (is_empty) {
//             log_this(SR_PAYLOAD, "End of tar archive reached", LOG_LEVEL_DEBUG, 0);
            break;
        }

        // Extract file name (null-terminated after 100 bytes)
        char filename[101] = {0};
        memcpy(filename, header, 100);

        // Extract file size from octal representation
        char size_str[13] = {0};
        memcpy(size_str, header + 124, 12);
        size_t file_size = 0;
        sscanf(size_str, "%zo", &file_size);

        // Get file type
        char type_flag = (char)header[156];
        bool is_regular_file = (type_flag == '0' || type_flag == '\0');

        if (is_regular_file && file_size > 0 && filename[0] != '\0') {
            // Grow array if needed
            if (file_count >= initial_capacity) {
                initial_capacity *= 2;
                PayloadFile *new_files = realloc(temp_files, initial_capacity * sizeof(PayloadFile));
                if (!new_files) {
                    log_this(SR_PAYLOAD, "Failed to grow temporary file array", LOG_LEVEL_ERROR, 0);
                    free(temp_files);
                    return false;
                }
                temp_files = new_files;
            }

            // Store file information
            temp_files[file_count].name = strdup(filename);
            temp_files[file_count].size = file_size;

            // Check if this is a Brotli compressed file by extension
            size_t name_len = strlen(filename);
            temp_files[file_count].is_compressed =
                (name_len > 3 && strcmp(filename + name_len - 3, ".br") == 0);

            // Extract file data
            size_t data_offset = pos + 512;
            if (data_offset + file_size > tar_size) {
                log_this(SR_PAYLOAD, "File data extends beyond tar boundary: %s", LOG_LEVEL_ERROR, 1, filename);
                free(temp_files[file_count].name);
                break;
            }

            temp_files[file_count].data = malloc(file_size);
            if (!temp_files[file_count].data) {
                log_this(SR_PAYLOAD, "Failed to allocate memory for file data: %s", LOG_LEVEL_ERROR, 1, filename);
                free(temp_files[file_count].name);
                break;
            }

            memcpy(temp_files[file_count].data, tar_data + data_offset, file_size);

            total_processed += file_size;
            file_count++;
        }

        // Move to next entry (header + data, rounded up to 512-byte boundary)
        size_t data_blocks = (file_size + 511) / 512;
        pos += 512 + (data_blocks * 512);

        // Safety check
        if (pos > tar_size) {
            log_this(SR_PAYLOAD, "Exceeded tar file size during parsing", LOG_LEVEL_ERROR, 0);
            break;
        }
    }

    log_this(SR_PAYLOAD, "Caching %'zu files:", LOG_LEVEL_DEBUG, 1, file_count);

    // Sort the collected files by filename
    if (file_count > 0) {
        qsort(temp_files, file_count, sizeof(PayloadFile), compare_files);
    }

    // Log the cached files in sorted order
    for (size_t i = 0; i < file_count; i++) {
        log_this(SR_PAYLOAD, "― %'8zu bytes: %s", LOG_LEVEL_DEBUG, 2, temp_files[i].size, temp_files[i].name);
    }

    // Store results in global cache
    global_payload_cache.files = temp_files;
    global_payload_cache.num_files = file_count;
    global_payload_cache.capacity = initial_capacity;

    log_this(SR_PAYLOAD, SR_PAYLOAD " Cache populated with %'zu files (%'zu bytes)", LOG_LEVEL_DEBUG, 2, file_count, total_processed);

    return (file_count > 0);
}

/**
 * Clean up the global payload cache
 */
void cleanup_payload_cache(void) {
    if (global_payload_cache.files) {
        for (size_t i = 0; i < global_payload_cache.num_files; i++) {
            free(global_payload_cache.files[i].name);
            free(global_payload_cache.files[i].data);
        }
        free(global_payload_cache.files);
    }

    free(global_payload_cache.tar_data);

    memset(&global_payload_cache, 0, sizeof(PayloadCache));
    log_this(SR_PAYLOAD, "Payload cache cleaned up", LOG_LEVEL_DEBUG, 0);
}
