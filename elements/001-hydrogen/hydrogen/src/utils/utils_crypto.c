/*
 * Cryptographic Utilities Implementation
 *
 * Provides reusable cryptographic functions used throughout Hydrogen
 */

// Global includes
#include <src/hydrogen.h>
#include <string.h>
#include <stdio.h>

// OpenSSL includes for cryptographic operations
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

// Local includes
#include "utils_crypto.h"
#include <src/logging/logging.h>

// Base64url encoding table (URL-safe base64)
static const char base64url_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

// Function prototypes
char* utils_base64url_encode(const unsigned char* data, size_t length);
unsigned char* utils_base64url_decode(const char* input, size_t* output_length);
char* utils_sha256_hash(const unsigned char* data, size_t length);
unsigned char* utils_hmac_sha256(const unsigned char* data, size_t data_len,
                                  const char* key, size_t key_len,
                                  unsigned int* output_len);
char* utils_password_hash(const char* password, int account_id);
bool utils_random_bytes(unsigned char* buffer, size_t length);

/**
 * Base64url encode data
 */
char* utils_base64url_encode(const unsigned char* data, size_t length) {
    if (!data || length == 0) return NULL;

    // Calculate output length without padding
    // Base64url does not use padding characters
    size_t output_length = (length * 4 + 2) / 3;
    char* encoded = calloc(output_length + 1, sizeof(char));
    if (!encoded) return NULL;

    size_t i, j;
    for (i = 0, j = 0; i < length && j < output_length; i += 3) {
        uint32_t octet_a = data[i];
        uint32_t octet_b = (i + 1 < length) ? data[i + 1] : 0;
        uint32_t octet_c = (i + 2 < length) ? data[i + 2] : 0;

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        encoded[j++] = base64url_table[(triple >> 18) & 0x3F];
        if (j < output_length) encoded[j++] = base64url_table[(triple >> 12) & 0x3F];
        if (j < output_length) encoded[j++] = base64url_table[(triple >> 6) & 0x3F];
        if (j < output_length) encoded[j++] = base64url_table[triple & 0x3F];
    }

    // String is already null-terminated from calloc
    return encoded;
}

/**
 * Base64url decode data
 */
unsigned char* utils_base64url_decode(const char* input, size_t* output_length) {
    if (!input || !output_length) return NULL;

    size_t input_length = strlen(input);
    if (input_length % 4 != 0 && input_length % 4 != 2 && input_length % 4 != 3) {
        return NULL; // Invalid base64url length
    }

    size_t decoded_length = (input_length * 3) / 4;
    unsigned char* decoded = calloc(decoded_length + 1, sizeof(unsigned char));
    if (!decoded) return NULL;

    size_t i, j;
    for (i = 0, j = 0; i < input_length; i += 4, j += 3) {
        // Convert base64url chars to 6-bit values
        uint32_t sextet_a = input[i] == '=' ? 0 : (uint32_t)(strchr(base64url_table, input[i]) - base64url_table);
        uint32_t sextet_b = (i + 1 < input_length && input[i + 1] != '=') ?
                            (uint32_t)(strchr(base64url_table, input[i + 1]) - base64url_table) : 0;
        uint32_t sextet_c = (i + 2 < input_length && input[i + 2] != '=') ?
                            (uint32_t)(strchr(base64url_table, input[i + 2]) - base64url_table) : 0;
        uint32_t sextet_d = (i + 3 < input_length && input[i + 3] != '=') ?
                            (uint32_t)(strchr(base64url_table, input[i + 3]) - base64url_table) : 0;

        uint32_t triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;

        if (j < decoded_length) decoded[j] = (triple >> 16) & 0xFF;
        if (j + 1 < decoded_length) decoded[j + 1] = (triple >> 8) & 0xFF;
        if (j + 2 < decoded_length) decoded[j + 2] = triple & 0xFF;
    }

    *output_length = decoded_length;
    return decoded;
}

/**
 * Compute SHA256 hash and return as base64url string
 */
char* utils_sha256_hash(const unsigned char* data, size_t length) {
    if (!data) return NULL;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data, length, hash);

    return utils_base64url_encode(hash, sizeof(hash));
}

/**
 * Compute HMAC-SHA256
 */
unsigned char* utils_hmac_sha256(const unsigned char* data, size_t data_len,
                                  const char* key, size_t key_len,
                                  unsigned int* output_len) {
    if (!data || !key || !output_len) return NULL;

    unsigned char* result = malloc(SHA256_DIGEST_LENGTH);
    if (!result) return NULL;

    *output_len = SHA256_DIGEST_LENGTH;

    if (HMAC(EVP_sha256(), key, (int)key_len, data, data_len, result, output_len) == NULL) {
        free(result);
        return NULL;
    }

    return result;
}

/**
 * Compute password hash using account_id + password
 */
char* utils_password_hash(const char* password, int account_id) {
    if (!password) return NULL;

    char combined[256];
    snprintf(combined, sizeof(combined), "%d%s", account_id, password);

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)combined, strlen(combined), hash);

    return utils_base64url_encode(hash, sizeof(hash));
}

/**
 * Generate cryptographically secure random bytes
 */
bool utils_random_bytes(unsigned char* buffer, size_t length) {
    if (!buffer || length == 0) return false;

    if (RAND_bytes(buffer, (int)length) != 1) {
        log_this("CRYPTO", "Failed to generate random bytes", LOG_LEVEL_ERROR, 0);
        return false;
    }

    return true;
}
