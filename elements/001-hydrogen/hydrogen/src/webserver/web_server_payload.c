/*
 * Payload Handler Implementation
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// System headers
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include <brotli/decode.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/evp.h>

// Project headers
#include "web_server_payload.h"
#include "../logging/logging.h"
#include "../config/config.h"

// Static function declarations
static bool decrypt_payload(const uint8_t *encrypted_data, size_t encrypted_size,
                          const char *private_key_b64, uint8_t **decrypted_data,
                          size_t *decrypted_size);
static void init_openssl(void);

// Initialize OpenSSL once at startup
static void init_openssl(void) {
    static bool initialized = false;
    if (!initialized) {
        OpenSSL_add_all_algorithms();
        ERR_load_crypto_strings();
        initialized = true;
    }
}

bool extract_payload(const char *executable_path, const AppConfig *config,
                    const char *marker, PayloadData *payload) {
    // Validate parameters
    if (!executable_path || !config || !marker || !payload) {
        log_this("PayloadHandler", "Invalid parameters for payload extraction", LOG_LEVEL_ERROR, NULL);
        return false;
    }

    // Check all state flags atomically
    extern volatile sig_atomic_t server_stopping;
    extern volatile sig_atomic_t server_running;
    extern volatile sig_atomic_t server_starting;
    extern volatile sig_atomic_t web_server_shutdown;

    // Prevent extraction during shutdown
    if (server_stopping || web_server_shutdown) {
        log_this("PayloadHandler", "Skipping payload extraction - system is shutting down", 
                LOG_LEVEL_INFO, NULL);
        return false;
    }

    // Only allow extraction during startup or normal operation
    if (!server_starting && !server_running) {
        log_this("PayloadHandler", "Skipping payload extraction - system not in proper state", 
                LOG_LEVEL_INFO, NULL);
        return false;
    }

    // Initialize output structure
    memset(payload, 0, sizeof(PayloadData));

    // Open the executable
    int fd = open(executable_path, O_RDONLY);
    if (fd == -1) {
        log_this("PayloadHandler", "Failed to open executable", LOG_LEVEL_ERROR, NULL);
        return false;
    }

    // Get file size
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        log_this("PayloadHandler", "Failed to get executable size", LOG_LEVEL_ERROR, NULL);
        return false;
    }

    // Map the file
    void *file_data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (file_data == MAP_FAILED) {
        log_this("PayloadHandler", "Failed to map executable", LOG_LEVEL_ERROR, NULL);
        return false;
    }

    // Search for marker
    const char *marker_pos = memmem(file_data, st.st_size, marker, strlen(marker));
    if (!marker_pos) {
        munmap(file_data, st.st_size);
        log_this("PayloadHandler", "No payload marker found in executable", LOG_LEVEL_INFO, NULL);
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
        log_this("PayloadHandler", "Invalid payload size or corrupted payload", LOG_LEVEL_ERROR, NULL);
        munmap(file_data, st.st_size);
        return false;
    }

    // The encrypted payload is before the marker
    const uint8_t *encrypted_data = (uint8_t*)marker_pos - payload_size;
    log_this("PayloadHandler", "Found encrypted payload: %zu bytes", LOG_LEVEL_INFO, payload_size);

    // Initialize OpenSSL
    init_openssl();

    // Get the payload key
    const char *payload_key = config->payload_key;
    if (!payload_key) {
        // Check environment variable if configured
        if (config->payload_key && strncmp(config->payload_key, "${env.", 6) == 0) {
            const char *end = strchr(config->payload_key + 6, '}');
            if (end) {
                char env_var[256];
                size_t len = end - (config->payload_key + 6);
                strncpy(env_var, config->payload_key + 6, len);
                env_var[len] = '\0';
                payload_key = getenv(env_var);
            }
        }
    }

    if (!payload_key) {
        log_this("PayloadHandler", "No valid payload key available", LOG_LEVEL_ERROR, NULL);
        munmap(file_data, st.st_size);
        return false;
    }

    // Decrypt the payload
    uint8_t *decrypted_data = NULL;
    size_t decrypted_size = 0;
    bool success = decrypt_payload(encrypted_data, payload_size, payload_key,
                                 &decrypted_data, &decrypted_size);

    // Unmap the executable
    munmap(file_data, st.st_size);

    if (!success) {
        log_this("PayloadHandler", "Failed to decrypt payload", LOG_LEVEL_ERROR, NULL);
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

static bool decrypt_payload(const uint8_t *encrypted_data, size_t encrypted_size,
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
        log_this("PayloadHandler", "Invalid payload structure", LOG_LEVEL_ERROR, NULL);
        return false;
    }

    // Extract IV (16 bytes after encrypted key)
    memcpy(iv, encrypted_data + 4 + key_size, 16);

    // Log payload structure details
    log_this("PayloadHandler", "Payload structure:", LOG_LEVEL_INFO, NULL);
    log_this("PayloadHandler", "- Total size: %zu bytes", LOG_LEVEL_INFO, encrypted_size);
    log_this("PayloadHandler", "- Key size: %u bytes", LOG_LEVEL_INFO, key_size);
    log_this("PayloadHandler", "- IV: 16 bytes", LOG_LEVEL_INFO, NULL);
    log_this("PayloadHandler", "- Encrypted payload: %zu bytes", LOG_LEVEL_INFO, 
             encrypted_size - 4 - key_size - 16);

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

    private_key_len = BIO_read(bio_chain, private_key_data, private_key_len);
    if (private_key_len <= 0) goto cleanup;

    bio = BIO_new_mem_buf(private_key_data, private_key_len);
    if (!bio) goto cleanup;

    // Load and verify private key
    pkey = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
    if (!pkey) {
        log_this("PayloadHandler", "Failed to load private key", LOG_LEVEL_ERROR, NULL);
        goto cleanup;
    }

    // Initialize decryption context
    pkey_ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (!pkey_ctx) goto cleanup;

    if (EVP_PKEY_decrypt_init(pkey_ctx) <= 0) goto cleanup;
    if (EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PADDING) <= 0) goto cleanup;

    // Get required buffer size for AES key
    if (EVP_PKEY_decrypt(pkey_ctx, NULL, &aes_key_len, encrypted_data + 4, key_size) <= 0) goto cleanup;

    aes_key = calloc(1, aes_key_len);
    if (!aes_key) goto cleanup;

    // Decrypt the AES key
    if (EVP_PKEY_decrypt(pkey_ctx, aes_key, &aes_key_len, encrypted_data + 4, key_size) <= 0) {
        log_this("PayloadHandler", "Failed to decrypt AES key", LOG_LEVEL_ERROR, NULL);
        goto cleanup;
    }

    // Verify AES key length
    if (aes_key_len != 32) {
        log_this("PayloadHandler", "Invalid AES key length", LOG_LEVEL_ERROR, NULL);
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
        log_this("PayloadHandler", "Failed to initialize AES decryption", LOG_LEVEL_ERROR, NULL);
        goto cleanup;
    }

    // Allocate output buffer
    *decrypted_data = calloc(1, encrypted_payload_size + EVP_MAX_BLOCK_LENGTH);
    if (!*decrypted_data) goto cleanup;

    // Decrypt payload
    if (!EVP_DecryptUpdate(ctx, *decrypted_data, &len, encrypted_payload, encrypted_payload_size)) {
        log_this("PayloadHandler", "Failed to decrypt payload", LOG_LEVEL_ERROR, NULL);
        free(*decrypted_data);
        *decrypted_data = NULL;
        goto cleanup;
    }

    *decrypted_size = len;

    // Finalize decryption
    if (!EVP_DecryptFinal_ex(ctx, *decrypted_data + len, &final_len)) {
        log_this("PayloadHandler", "Failed to finalize decryption", LOG_LEVEL_ERROR, NULL);
        free(*decrypted_data);
        *decrypted_data = NULL;
        goto cleanup;
    }

    *decrypted_size += final_len;
    log_this("PayloadHandler", "Payload decrypted successfully (%zu bytes)", LOG_LEVEL_INFO, *decrypted_size);
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