/*
 * Mock cryptographic utilities for unit testing
 *
 * This file provides mock implementations of cryptographic functions
 * to enable testing of error conditions in JWT and authentication code.
 */

#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

// Include the header but undefine the macros to access real functions
#include "mock_crypto.h"

// Undefine the macros in this file so we can call the real functions
#undef utils_base64url_encode
#undef utils_base64url_decode
#undef utils_sha256_hash
#undef utils_hmac_sha256
#undef utils_password_hash
#undef utils_random_bytes

// Include the real crypto header to get function prototypes
#include <src/utils/utils_crypto.h>

// Function prototypes - these are defined in the header when USE_MOCK_CRYPTO is set
char* mock_utils_base64url_encode(const unsigned char* data, size_t length);
unsigned char* mock_utils_base64url_decode(const char* input, size_t* output_length);
char* mock_utils_sha256_hash(const unsigned char* data, size_t length);
unsigned char* mock_utils_hmac_sha256(const unsigned char* data, size_t data_len,
                                      const char* key, size_t key_len,
                                      unsigned int* output_len);
char* mock_utils_password_hash(const char* password, int account_id);
bool mock_utils_random_bytes(unsigned char* buffer, size_t length);
void mock_crypto_set_base64url_encode_failure(int should_fail);
void mock_crypto_set_base64url_encode_result(const char* result);
void mock_crypto_set_base64url_decode_failure(int should_fail);
void mock_crypto_set_sha256_hash_failure(int should_fail);
void mock_crypto_set_hmac_sha256_failure(int should_fail);
void mock_crypto_set_password_hash_failure(int should_fail);
void mock_crypto_set_random_bytes_failure(int should_fail);
void mock_crypto_reset_all(void);

// Global variables to store mock state - shared across all object files
int mock_base64url_encode_should_fail = 0;
const char* mock_base64url_encode_result = NULL;
int mock_base64url_decode_should_fail = 0;
int mock_sha256_hash_should_fail = 0;
int mock_hmac_sha256_should_fail = 0;
int mock_password_hash_should_fail = 0;
int mock_random_bytes_should_fail = 0;

// Mock implementation of utils_base64url_encode
char* mock_utils_base64url_encode(const unsigned char* data, size_t length) {
    (void)data;    // Suppress unused parameter
    (void)length;  // Suppress unused parameter

    if (mock_base64url_encode_should_fail) {
        return NULL;
    }

    if (mock_base64url_encode_result) {
        return strdup(mock_base64url_encode_result);
    }

    // Default behavior - call the real function
    return utils_base64url_encode(data, length);
}

// Mock implementation of utils_base64url_decode
unsigned char* mock_utils_base64url_decode(const char* input, size_t* output_length) {
    (void)input;         // Suppress unused parameter
    (void)output_length; // Suppress unused parameter

    if (mock_base64url_decode_should_fail) {
        return NULL;
    }

    // Default behavior - call the real function
    return utils_base64url_decode(input, output_length);
}

// Mock implementation of utils_sha256_hash
char* mock_utils_sha256_hash(const unsigned char* data, size_t length) {
    (void)data;   // Suppress unused parameter
    (void)length; // Suppress unused parameter

    if (mock_sha256_hash_should_fail) {
        return NULL;
    }

    // Default behavior - call the real function
    return utils_sha256_hash(data, length);
}

// Mock implementation of utils_hmac_sha256
unsigned char* mock_utils_hmac_sha256(const unsigned char* data, size_t data_len,
                                      const char* key, size_t key_len,
                                      unsigned int* output_len) {
    (void)data;     // Suppress unused parameter
    (void)data_len; // Suppress unused parameter
    (void)key;      // Suppress unused parameter
    (void)key_len;  // Suppress unused parameter

    if (mock_hmac_sha256_should_fail) {
        return NULL;
    }

    // Set output length
    if (output_len) {
        *output_len = SHA256_DIGEST_LENGTH;
    }

    // Default behavior - call the real function
    return utils_hmac_sha256(data, data_len, key, key_len, output_len);
}

// Mock implementation of utils_password_hash
char* mock_utils_password_hash(const char* password, int account_id) {
    (void)password;   // Suppress unused parameter
    (void)account_id; // Suppress unused parameter

    if (mock_password_hash_should_fail) {
        return NULL;
    }

    // Default behavior - call the real function
    return utils_password_hash(password, account_id);
}

// Mock implementation of utils_random_bytes
bool mock_utils_random_bytes(unsigned char* buffer, size_t length) {
    (void)buffer; // Suppress unused parameter
    (void)length; // Suppress unused parameter

    if (mock_random_bytes_should_fail) {
        return false;
    }

    // Default behavior - call the real function
    return utils_random_bytes(buffer, length);
}

// Mock control functions
void mock_crypto_set_base64url_encode_failure(int should_fail) {
    mock_base64url_encode_should_fail = should_fail;
}

void mock_crypto_set_base64url_encode_result(const char* result) {
    mock_base64url_encode_result = result;
}

void mock_crypto_set_base64url_decode_failure(int should_fail) {
    mock_base64url_decode_should_fail = should_fail;
}

void mock_crypto_set_sha256_hash_failure(int should_fail) {
    mock_sha256_hash_should_fail = should_fail;
}

void mock_crypto_set_hmac_sha256_failure(int should_fail) {
    mock_hmac_sha256_should_fail = should_fail;
}

void mock_crypto_set_password_hash_failure(int should_fail) {
    mock_password_hash_should_fail = should_fail;
}

void mock_crypto_set_random_bytes_failure(int should_fail) {
    mock_random_bytes_should_fail = should_fail;
}

void mock_crypto_reset_all(void) {
    mock_base64url_encode_should_fail = 0;
    mock_base64url_encode_result = NULL;
    mock_base64url_decode_should_fail = 0;
    mock_sha256_hash_should_fail = 0;
    mock_hmac_sha256_should_fail = 0;
    mock_password_hash_should_fail = 0;
    mock_random_bytes_should_fail = 0;
}