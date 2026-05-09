/*
 * Cryptographic Utilities
 *
 * Provides cryptographic functions for secure operations including:
 * - Base64url encoding/decoding (URL-safe variant)
 * - SHA256 hashing
 * - HMAC-SHA256 operations
 * - Password hashing
 * - JWK (RSA) -> EVP_PKEY parsing
 * - RS256 signature verification (EVP_DigestVerify)
 *
 * These utilities are used by authentication, JWT generation, and other
 * security-sensitive operations throughout the Hydrogen framework.
 */

#ifndef UTILS_CRYPTO_H
#define UTILS_CRYPTO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Forward-declare to avoid forcing every includer to pull in <openssl/evp.h>.
typedef struct evp_pkey_st EVP_PKEY;

/**
 * @brief Standard Base64 encode data WITH padding (RFC 4648)
 * @param data Input data to encode
 * @param length Length of input data in bytes
 * @return Allocated string containing base64 encoded data, or NULL on error
 * @note Caller must free the returned string
 */
char* utils_base64_encode(const unsigned char* data, size_t length);

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

/**
 * @brief Parse an RSA JWK (JSON Web Key) into an EVP_PKEY public key.
 *
 * Reads a JWK JSON object (the verbatim JSON object string for one
 * key, as retained by `OidcRpJwk.json_text`), extracts the base64url-
 * encoded modulus (`n`) and exponent (`e`), and constructs an
 * `EVP_PKEY*` suitable for `EVP_DigestVerify`.
 *
 * Only RSA keys (`kty == "RSA"`) are supported in this revision; any
 * other key type returns NULL. Private-key parameters in the JWK (if
 * present) are ignored — the resulting `EVP_PKEY` is public-only.
 *
 * @param jwk_json NUL-terminated JSON object string for a single JWK.
 * @return Newly-allocated `EVP_PKEY*` (caller frees via `EVP_PKEY_free`),
 *         or NULL on parse error / unsupported `kty` / OpenSSL failure.
 *         Errors are logged at LOG_LEVEL_ALERT under SR_AUTH.
 */
EVP_PKEY* utils_jwk_rsa_to_pkey(const char* jwk_json);

/**
 * @brief Verify an RS256 signature over arbitrary signing-input bytes.
 *
 * Wraps `EVP_DigestVerifyInit(EVP_sha256())` + `EVP_DigestVerify`. The
 * caller is expected to have already constructed the JWS signing input
 * (header_b64 + "." + payload_b64) and base64url-decoded the
 * signature segment into raw bytes.
 *
 * @param pkey       Public key (typically from `utils_jwk_rsa_to_pkey`).
 *                   Must be non-NULL.
 * @param input      Signing-input bytes. Must be non-NULL.
 * @param input_len  Length of signing-input bytes. Must be > 0.
 * @param signature  Raw signature bytes (NOT base64url). Must be non-NULL.
 * @param sig_len    Length of signature bytes. Must be > 0.
 * @return true if the signature verifies; false on any failure (bad
 *         signature, OpenSSL internal error, NULL/empty argument).
 *         Bad-signature failures are silent (callers map them to
 *         user-facing error envelopes); OpenSSL internal errors are
 *         logged at LOG_LEVEL_ALERT under SR_AUTH.
 */
bool utils_rs256_verify(EVP_PKEY* pkey,
                         const unsigned char* input, size_t input_len,
                         const unsigned char* signature, size_t sig_len);

#endif // UTILS_CRYPTO_H
