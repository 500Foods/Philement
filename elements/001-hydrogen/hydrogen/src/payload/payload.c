/*
 * Payload Handler Implementation
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "payload.h"

// OpenSSL includes
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/evp.h>

/**
 * Check if a payload exists in the executable
 * 
 * This function checks for a payload marker and validates basic payload structure.
 * It's a lightweight check that doesn't perform decryption or decompression.
 * 
 * @param marker The marker string to search for
 * @param size If payload is found, will contain its size
 * @return true if valid payload found, false otherwise
 */
bool check_payload_exists(const char *marker, size_t *size) {
    if (!marker || !size || strlen(marker) == 0) return false;
    
    char *executable_path = get_executable_path();
    if (!executable_path) return false;
    
    int fd = open(executable_path, O_RDONLY);
    free(executable_path);
    
    if (fd == -1) return false;
    
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return false;
    }
    
    void *file_data = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    
    if (file_data == MAP_FAILED) return false;
    
    bool result = false;
    
    // Find the LAST occurrence of the marker (the real payload marker)
    const char *marker_pos = NULL;
    char *search_start = (char*)file_data;
    size_t remaining_size = (size_t)st.st_size;
    
    while (remaining_size >= strlen(marker)) {
        char *found = memmem(search_start, remaining_size, marker, strlen(marker));
        if (!found) {
            break;
        }
        marker_pos = found;  // Keep track of the last found occurrence
        
        // Move search position past this marker
        search_start = found + strlen(marker);
        remaining_size = (size_t)st.st_size - (size_t)(search_start - (char*)file_data);
    }
    
    if (marker_pos && st.st_size > 0) {
        size_t file_size = (size_t)st.st_size;
        size_t marker_offset = (size_t)(marker_pos - (char*)file_data);
        if (marker_offset + strlen(marker) + 8 <= file_size) {
            const uint8_t *size_bytes = (uint8_t*)(marker_pos + strlen(marker));
            *size = 0;
            for (int i = 0; i < 8; i++) {
                *size = (*size << 8) | size_bytes[i];
            }
            
            if (*size > 0 && *size <= 100 * 1024 * 1024 && *size <= marker_offset) {
                result = true;
            }
        }
    }
    
    munmap(file_data, (size_t)st.st_size);
    return result;
}

/**
 * Validate a payload decryption key
 * 
 * This function checks if the provided key is valid for payload decryption.
 * For environment variables, it checks if the variable exists and has a value.
 * 
 * @param key The key to validate (can be direct or ${env.VAR} format)
 * @return true if key is valid, false otherwise
 */
bool validate_payload_key(const char *key) {
    if (!key || strcmp(key, "Missing Key") == 0 || strlen(key) == 0) {
        return false;
    }
    
    // Check if it's an environment variable reference
    if (strncmp(key, "${env.", 6) == 0) {
        const char *end = strchr(key + 6, '}');
        if (!end) return false;
        
        size_t len = (size_t)(end - (key + 6));
        if (len == 0 || len > 255) return false;
        
        char env_var[256];
        strncpy(env_var, key + 6, len);
        env_var[len] = '\0';
        
        const char *env_value = getenv(env_var);
        return (env_value && strlen(env_value) > 0);
    }
    
    // Direct key value - just check it's not empty
    return true;
}

/**
 * Initialize OpenSSL once at startup
 *
 * This function initializes OpenSSL algorithms and error strings.
 * It is safe to call this function multiple times as it will only initialize once.
 */
void init_openssl(void) {
    static bool initialized = false;
    if (!initialized) {
        OpenSSL_add_all_algorithms();
        ERR_load_crypto_strings();
        initialized = true;
    }
}

/**
 * Clean up OpenSSL resources
 *
 * This function cleans up resources allocated by OpenSSL during payload processing.
 * It should be called during shutdown to prevent memory leaks.
 * It is safe to call this function multiple times as it will only clean up
 * resources once.
 */
void cleanup_openssl(void) {
    static bool cleaned_up = false;

    // Only clean up once
    if (cleaned_up) {
        log_this(SR_PAYLOAD, "OpenSSL resources already cleaned up", LOG_LEVEL_STATE, 0);
        return;
    }

    // Clean up OpenSSL resources
    EVP_cleanup();
    ERR_free_strings();

    // Set the flag to indicate resources have been cleaned up
    cleaned_up = true;

    // Log the cleanup
    log_this(SR_PAYLOAD, "OpenSSL resources cleaned up", LOG_LEVEL_STATE, 0);
}

bool extract_payload(const char *executable_path, const AppConfig *config,
                    const char *marker, PayloadData *payload) {
    // Validate parameters
    if (!executable_path || !config || !marker || !payload) {
        log_this(SR_PAYLOAD, "― Invalid parameters for payload extraction", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Check all state flags atomically

    // Prevent extraction during shutdown
    if (server_stopping || web_server_shutdown) {
        log_this(SR_PAYLOAD, "―Skipping payload extraction - system is shutting down", LOG_LEVEL_DEBUG, 0);
        return false;
    }

    // Only allow extraction during startup or normal operation
    if (!server_starting && !server_running) {
        log_this(SR_PAYLOAD, "Skipping payload extraction - system not in proper state", LOG_LEVEL_DEBUG, 0);
        return false;
    }

    // Initialize output structure
    memset(payload, 0, sizeof(PayloadData));

    // Open the executable
    int fd = open(executable_path, O_RDONLY);
    if (fd == -1) {
        log_this(SR_PAYLOAD, "Failed to open executable", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Get file size
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        log_this(SR_PAYLOAD, "Failed to get executable size", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Map the file
    void *file_data = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (file_data == MAP_FAILED) {
        log_this(SR_PAYLOAD, "Failed to map executable", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Find the LAST occurrence of the marker (the real payload marker)
    const char *marker_pos = NULL;
    char *search_start = (char*)file_data;
    size_t remaining_size = (size_t)st.st_size;
    
    while (remaining_size >= strlen(marker)) {
        char *found = memmem(search_start, remaining_size, marker, strlen(marker));
        if (!found) {
            break;
        }
        marker_pos = found;  // Keep track of the last found occurrence
        
        // Move search position past this marker
        search_start = found + strlen(marker);
        remaining_size = (size_t)st.st_size - (size_t)(search_start - (char*)file_data);
    }
    
    if (!marker_pos) {
        munmap(file_data, (size_t)st.st_size);
        log_this(SR_PAYLOAD, "No payload marker found in executable", LOG_LEVEL_DEBUG, 0);
        return false;
    }

    // Get encrypted payload size from the 8 bytes after marker
    const uint8_t *size_bytes = (uint8_t*)(marker_pos + strlen(marker));
    size_t payload_size = 0;
    for (int i = 0; i < 8; i++) {
        payload_size = (payload_size << 8) | size_bytes[i];
    }

    // Validate payload size
    if (payload_size == 0 || payload_size > (size_t)(marker_pos - (char*)file_data)) {
        log_this(SR_PAYLOAD, "Invalid payload size or corrupted payload", LOG_LEVEL_ERROR, 0);
        munmap(file_data, (size_t)st.st_size);
        return false;
    }

    // The encrypted payload is before the marker
    const uint8_t *encrypted_data = (uint8_t*)marker_pos - payload_size;
    log_this(SR_PAYLOAD, "― Found encrypted payload: %'d bytes", LOG_LEVEL_DEBUG, 1, payload_size);

    // Initialize OpenSSL
    init_openssl();

    // Get the payload key directly from config - env vars should already be resolved
    if (!config->server.payload_key) {
        log_this(SR_PAYLOAD, "No valid payload key available", LOG_LEVEL_ERROR, 0);
        munmap(file_data, (size_t)st.st_size);
        return false;
    }

    // Log the key snippet we're using (first 5 chars)
    if (strlen(config->server.payload_key) > 5) {
        log_this(SR_PAYLOAD, "― Using key: %.5s...", LOG_LEVEL_DEBUG, 1, config->server.payload_key);
    }

    // Decrypt the payload using config's key directly
    uint8_t *decrypted_data = NULL;
    size_t decrypted_size = 0;
    bool success = decrypt_payload(encrypted_data, payload_size, config->server.payload_key, &decrypted_data, &decrypted_size);

    // Unmap the executable
    munmap(file_data, (size_t)st.st_size);

    if (!success) {
        log_this(SR_PAYLOAD, "Failed to decrypt payload", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Store the result
    payload->data = decrypted_data;
    payload->size = decrypted_size;
    payload->is_compressed = true; // Assume Brotli compression by default

    return true;
}

void free_payload(PayloadData *payload) {
    if (payload) {
        free(payload->data);
        memset(payload, 0, sizeof(PayloadData));
    }
}

/**
 * Process the payload data
 * 
 * This function processes the extracted payload data, for example by
 * decompressing it if needed.
 * 
 * @param payload The payload data to process
 * @return true if payload was successfully processed, false otherwise
 */
bool process_payload_data(const PayloadData *payload) {
    if (!payload || !payload->data || payload->size == 0) {
        log_this(SR_PAYLOAD, "Invalid payload data", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Log payload information
    log_this(SR_PAYLOAD, "―  " SR_PAYLOAD ":   %'d bytes", LOG_LEVEL_DEBUG, 1, payload->size);

    // Check if payload is compressed
    if (payload->is_compressed) {
        // log_this(SR_PAYLOAD, "Payload is compressed with Brotli", LOG_LEVEL_STATE, 0);
        
        // Use Brotli streaming API for decompression
        BrotliDecoderState* decoder = BrotliDecoderCreateInstance(NULL, NULL, NULL);
        if (!decoder) { 
            log_this(SR_PAYLOAD, "Failed to create Brotli decoder", LOG_LEVEL_ERROR, 0);
            return false;
        }
        
        // Initial buffer size - we'll grow it if needed
        size_t buffer_size = payload->size * 4;  // Start with 4x the compressed size
        uint8_t* decompressed_data = malloc(buffer_size);
        if (!decompressed_data) {
            log_this(SR_PAYLOAD, "Failed to allocate memory for decompressed data", LOG_LEVEL_ERROR, 0);
            BrotliDecoderDestroyInstance(decoder);
            return false;
        }
        
        // Set up input/output parameters
        const uint8_t* next_in = payload->data;
        size_t available_in = payload->size;
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
        
        // log_this(SR_PAYLOAD, "― Decompressed size: %'10d bytes", LOG_LEVEL_DEBUG, 1, total_out);
        
        // Parse the tar file to count files and total size
        if (total_out > 512) { // Minimum size for a valid tar file
            size_t pos = 0;
            size_t file_count = 0;
            size_t total_file_size = 0;
            
            // Process tar headers
            while (pos + 512 <= total_out) {
                const uint8_t* header = decompressed_data + pos;
                
                // Check for end of archive (empty block)
                bool is_empty = true;
                for (int i = 0; i < 512; i++) {
                    if (header[i] != 0) {
                        is_empty = false;
                        break;
                    }
                }
                if (is_empty) {
                    break;
                }
                
                // Extract file size from octal representation in header
                char size_str[13] = {0};
                memcpy(size_str, header + 124, 12);
                size_t file_size = 0;
                sscanf(size_str, "%zo", &file_size);
                
                // Get file name for logging
                char file_name[101] = {0};
                memcpy(file_name, header, 100);
                file_name[100] = '\0';  // Ensure null termination
                
                // Only count regular files (type flag '0' or '\0')
                char type_flag = (char)header[156];
                if (type_flag == '0' || type_flag == '\0') {
                    file_count++;
                    total_file_size += file_size;
                }
                
                // Move to next file entry (header + data, rounded up to 512-byte boundary)
                size_t data_blocks = (file_size + 511) / 512;
                pos += 512 + (data_blocks * 512);
                
                // Safety check
                if (pos > total_out) {
                    break;
                }
            }
            
            log_this(SR_PAYLOAD, "― " SR_PAYLOAD " contains: %'d files, total size: %'d bytes", LOG_LEVEL_DEBUG, 2, file_count, total_file_size);
        }
        
        // Free the decompressed data
        free(decompressed_data);
    }
    
    return true;
}

/**
 * Launch the payload subsystem
 * 
 * This function extracts and processes the payload from the executable.
 * All logging output is tagged with the "Payload" category.
 * 
 * @param config Application configuration containing the payload key
 * @param marker Marker string that identifies the payload
 * @return true if payload was successfully launched, false otherwise
 */
bool launch_payload(const AppConfig *config, const char *marker) {
    // Validate parameters
    if (!config || !marker) {
        log_this(SR_PAYLOAD, "Invalid parameters for " SR_PAYLOAD " launch", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Check all state flags atomically

    // Prevent launch during shutdown
    if (server_stopping || web_server_shutdown) {
        log_this(SR_PAYLOAD, "Skipping " SR_PAYLOAD " launch - system is shutting down", LOG_LEVEL_DEBUG, 0);
        return false;
    }

    // Only allow launch during startup or normal operation
    if (!server_starting && !server_running) {
        log_this(SR_PAYLOAD, "Skipping " SR_PAYLOAD " launch - system not in proper state", LOG_LEVEL_DEBUG, 0);
        return false;
    }

    // Get executable path
    char *executable_path = get_executable_path();
    if (!executable_path) {
        log_this(SR_PAYLOAD, "Failed to get executable path", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Extract the payload
    PayloadData payload = {0};
    bool success = extract_payload(executable_path, config, marker, &payload);
    free(executable_path);

    if (!success) {
        log_this(SR_PAYLOAD, "Failed to extract " SR_PAYLOAD, LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Process the payload
    success = process_payload_data(&payload);
    
    // Free the payload data
    free_payload(&payload);
    
    if (!success) {
        log_this(SR_PAYLOAD, "Failed to process " SR_PAYLOAD, LOG_LEVEL_ERROR, 0);
        return false;
    }
    
    return true;
}

bool decrypt_payload(const uint8_t *encrypted_data, size_t encrypted_size,
                          const char *private_key_b64, uint8_t **decrypted_data,
                          size_t *decrypted_size) {
    if (!encrypted_data || encrypted_size < 21 || !private_key_b64 || 
        !decrypted_data || !decrypted_size) {
        return false;
    }

    bool success = false;
    BIO *bio = NULL;
    BIO *b64 = NULL;
    BIO *mem = NULL;
    BIO *bio_chain = NULL;
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_CTX *pkey_ctx = NULL;
    uint8_t *aes_key = NULL;
    uint8_t *private_key_data = NULL;
    size_t private_key_len = 0;
    EVP_CIPHER_CTX *ctx = NULL;
    size_t aes_key_len = 0;
    const uint8_t *encrypted_payload = NULL;
    size_t encrypted_payload_size = 0;
    uint8_t iv[16] = {0};
    int len = 0;
    int final_len = 0;

    *decrypted_data = NULL;
    *decrypted_size = 0;

    // Extract key size and IV
    uint32_t key_size = ((uint32_t)encrypted_data[0] << 24) |
                        ((uint32_t)encrypted_data[1] << 16) |
                        ((uint32_t)encrypted_data[2] << 8) |
                        ((uint32_t)encrypted_data[3]);

    if (key_size == 0 || key_size > 1024 || key_size + 20 >= encrypted_size) {
        log_this(SR_PAYLOAD, "Invalid payload structure", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Extract IV (16 bytes after encrypted key)
    memcpy(iv, encrypted_data + 4 + key_size, 16);

    // Log payload structure details
    log_this(SR_PAYLOAD, SR_PAYLOAD " structure:", LOG_LEVEL_DEBUG, 0);
    log_this(SR_PAYLOAD, "― Executable size:   %'10d bytes", LOG_LEVEL_DEBUG, 1, server_executable_size);
    log_this(SR_PAYLOAD, "― Payload size:      %'10d bytes", LOG_LEVEL_DEBUG, 1, encrypted_size);
    log_this(SR_PAYLOAD, "― Key size:          %10u bytes", LOG_LEVEL_DEBUG, 1, key_size);
    log_this(SR_PAYLOAD, "― Init Vector (IV):  %10d bytes", LOG_LEVEL_DEBUG, 1, 16);
    log_this(SR_PAYLOAD, "― Encrypted size:    %'10d bytes", LOG_LEVEL_DEBUG, 1, encrypted_size - 4 - key_size - 16);

    // Decode private key from base64
    b64 = BIO_new(BIO_f_base64());
    if (!b64) goto cleanup;
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    mem = BIO_new_mem_buf(private_key_b64, -1);
    if (!mem) goto cleanup;

    bio_chain = BIO_push(b64, mem);

    private_key_len = strlen(private_key_b64) * 3 / 4 + 1;
    private_key_data = calloc(1, private_key_len);
    if (!private_key_data) goto cleanup;

    int read_len = BIO_read(bio_chain, private_key_data, (int)private_key_len);
    if (read_len <= 0) goto cleanup;
    private_key_len = (size_t)read_len;

    bio = BIO_new_mem_buf(private_key_data, (int)private_key_len);
    if (!bio) goto cleanup;

    // Load and verify private key
    pkey = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
    if (!pkey) {
        log_this(SR_PAYLOAD, "Failed to load private key", LOG_LEVEL_ERROR, 0);
        goto cleanup;
    }

    // Initialize decryption context
    pkey_ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (!pkey_ctx) goto cleanup;

    if (EVP_PKEY_decrypt_init(pkey_ctx) <= 0) goto cleanup;
    if (EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PADDING) <= 0) goto cleanup;

    // Get required buffer size for AES key
    if (EVP_PKEY_decrypt(pkey_ctx, NULL, &aes_key_len, encrypted_data + 4, (size_t)key_size) <= 0) goto cleanup;

    aes_key = calloc(1, aes_key_len);
    if (!aes_key) goto cleanup;

    // Decrypt the AES key
    if (EVP_PKEY_decrypt(pkey_ctx, aes_key, &aes_key_len, encrypted_data + 4, (size_t)key_size) <= 0) {
        log_this(SR_PAYLOAD, "Failed to decrypt AES key", LOG_LEVEL_ERROR, 0);
        goto cleanup;
    }

    // Verify AES key length
    if (aes_key_len != 32) {
        log_this(SR_PAYLOAD, "Invalid AES key length", LOG_LEVEL_ERROR, 0);
        goto cleanup;
    }

    // Get encrypted payload data
    encrypted_payload = encrypted_data + 4 + key_size + 16;
    encrypted_payload_size = encrypted_size - 4 - key_size - 16;

    // Initialize AES decryption
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) goto cleanup;

    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, aes_key, iv) ||
        !EVP_CIPHER_CTX_set_padding(ctx, 1)) {
        log_this(SR_PAYLOAD, "Failed to initialize AES decryption", LOG_LEVEL_ERROR, 0);
        goto cleanup;
    }

    // Allocate output buffer
    *decrypted_data = calloc(1, encrypted_payload_size + EVP_MAX_BLOCK_LENGTH);
    if (!*decrypted_data) goto cleanup;

    // Decrypt payload
    if (!EVP_DecryptUpdate(ctx, *decrypted_data, &len, encrypted_payload, (int)encrypted_payload_size)) {
        log_this(SR_PAYLOAD, "Failed to decrypt payload", LOG_LEVEL_ERROR, 0);
        free(*decrypted_data);
        *decrypted_data = NULL;
        goto cleanup;
    }

    *decrypted_size = (size_t)len;

    // Finalize decryption
    if (!EVP_DecryptFinal_ex(ctx, *decrypted_data + len, &final_len)) {
        log_this(SR_PAYLOAD, "Failed to finalize decryption", LOG_LEVEL_ERROR, 0);
        free(*decrypted_data);
        *decrypted_data = NULL;
        goto cleanup;
    }

    *decrypted_size += (size_t)final_len;
    log_this(SR_PAYLOAD, "― Decrypted size:    %'10d bytes", LOG_LEVEL_DEBUG, 1, *decrypted_size);
    success = true;

cleanup:
    if (bio) BIO_free(bio);
    if (b64) BIO_free(b64);
    if (mem) BIO_free(mem);
    if (pkey) EVP_PKEY_free(pkey);
    if (pkey_ctx) EVP_PKEY_CTX_free(pkey_ctx);
    if (ctx) EVP_CIPHER_CTX_free(ctx);
    if (aes_key) {
        memset(aes_key, 0, aes_key_len);
        free(aes_key);
    }
    if (private_key_data) {
        memset(private_key_data, 0, private_key_len);
        free(private_key_data);
    }

    return success;
}
