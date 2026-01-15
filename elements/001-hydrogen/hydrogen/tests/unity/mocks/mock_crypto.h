/*
 * Mock cryptographic utilities for unit testing
 *
 * This file provides mock implementations of cryptographic functions
 * to enable testing of error conditions in JWT and authentication code.
 */

#ifndef MOCK_CRYPTO_H
#define MOCK_CRYPTO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Mock function declarations - these will override the real ones when USE_MOCK_CRYPTO is defined
#ifdef USE_MOCK_CRYPTO

// Override crypto functions with our mocks
#define utils_base64url_encode mock_utils_base64url_encode
#define utils_base64url_decode mock_utils_base64url_decode
#define utils_sha256_hash mock_utils_sha256_hash
#define utils_hmac_sha256 mock_utils_hmac_sha256
#define utils_password_hash mock_utils_password_hash
#define utils_random_bytes mock_utils_random_bytes

// Mock function prototypes
char* mock_utils_base64url_encode(const unsigned char* data, size_t length);
unsigned char* mock_utils_base64url_decode(const char* input, size_t* output_length);
char* mock_utils_sha256_hash(const unsigned char* data, size_t length);
unsigned char* mock_utils_hmac_sha256(const unsigned char* data, size_t data_len,
                                      const char* key, size_t key_len,
                                      unsigned int* output_len);
char* mock_utils_password_hash(const char* password, int account_id);
bool mock_utils_random_bytes(unsigned char* buffer, size_t length);

// Mock control functions for tests - always available
void mock_crypto_set_base64url_encode_failure(int should_fail);
void mock_crypto_set_base64url_encode_result(const char* result);
void mock_crypto_set_base64url_decode_failure(int should_fail);
void mock_crypto_set_sha256_hash_failure(int should_fail);
void mock_crypto_set_hmac_sha256_failure(int should_fail);
void mock_crypto_set_password_hash_failure(int should_fail);
void mock_crypto_set_random_bytes_failure(int should_fail);
void mock_crypto_reset_all(void);

// Extern declarations for global mock state variables (defined in mock_crypto.c)
extern int mock_base64url_encode_should_fail;
extern const char* mock_base64url_encode_result;
extern int mock_base64url_decode_should_fail;
extern int mock_sha256_hash_should_fail;
extern int mock_hmac_sha256_should_fail;
extern int mock_password_hash_should_fail;
extern int mock_random_bytes_should_fail;

#endif // USE_MOCK_CRYPTO

#endif // MOCK_CRYPTO_H