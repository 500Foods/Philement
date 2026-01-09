/*
 * Cryptographic Utilities
 *
 * Provides cryptographic functions for secure operations including:
 * - Base64url encoding/decoding (URL-safe variant)
 * - SHA256 hashing
 * - HMAC-SHA256 operations
 * - Password hashing
 *
 * These utilities are used by authentication, JWT generation, and other
 * security-sensitive operations throughout the Hydrogen framework.
 */

#ifndef UTILS_CRYPTO_H
#define UTILS_CRYPTO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Base64url encode data (URL-safe base64 without padding)
 * @param data Input data to encode
 * @param length Length of input data in bytes
 * @return Allocated string containing base64url encoded data, or NULL on error
 * @note Caller must free the returned string
 * @note Base64url uses '-' and '_' instead of '+' and '/' and omits padding
 */
char* utils_base64url_encode(const unsigned char* data, size_t length);

/**
 * @brief Base64url decode data
 * @param input Base64url encoded string
 * @param output_length Pointer to store the decoded data length
 * @return Allocated buffer containing decoded data, or NULL on error
 * @note Caller must free the returned buffer
 */
unsigned char* utils_base64url_decode(const char* input, size_t* output_length);

/**
 * @brief Compute SHA256 hash of data
 * @param data Input data to hash
 * @param length Length of input data in bytes
 * @return Allocated string containing base64url encoded SHA256 hash, or NULL on error
 * @note Caller must free the returned string
 * @note Output is base64url encoded for easy storage/comparison
 */
char* utils_sha256_hash(const unsigned char* data, size_t length);

/**
 * @brief Compute HMAC-SHA256 of data with a secret key
 * @param data Input data
 * @param data_len Length of input data
 * @param key HMAC secret key
 * @param key_len Length of secret key
 * @param output_len Pointer to store output length (will be SHA256_DIGEST_LENGTH)
 * @return Allocated buffer containing HMAC result, or NULL on error
 * @note Caller must free the returned buffer
 * @note Returns raw bytes, not base64 encoded
 */
unsigned char* utils_hmac_sha256(const unsigned char* data, size_t data_len,
                                  const char* key, size_t key_len,
                                  unsigned int* output_len);

/**
 * @brief Compute password hash using account_id + password
 * @param password Password string
 * @param account_id Account ID to salt the password
 * @return Allocated string containing base64url encoded password hash, or NULL on error
 * @note Caller must free the returned string
 * @note Uses SHA256(account_id + password) for hashing
 */
char* utils_password_hash(const char* password, int account_id);

/**
 * @brief Generate random bytes for cryptographic use
 * @param buffer Buffer to fill with random bytes
 * @param length Number of random bytes to generate
 * @return true on success, false on error
 * @note Uses OpenSSL RAND_bytes for cryptographically secure random data
 */
bool utils_random_bytes(unsigned char* buffer, size_t length);

#endif // UTILS_CRYPTO_H
