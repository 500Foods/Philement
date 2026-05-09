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
#include <openssl/core_names.h>
#include <openssl/param_build.h>
#include <openssl/err.h>

// jansson for JWK parsing
#include <jansson.h>

// Local includes
#include "utils_crypto.h"
#include <src/logging/logging.h>

// Base64url encoding table (URL-safe base64)
static const char base64url_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

// Standard Base64 encoding table (RFC 4648)
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Function prototypes
char* utils_base64_encode(const unsigned char* data, size_t length);
char* utils_base64url_encode(const unsigned char* data, size_t length);
unsigned char* utils_base64url_decode(const char* input, size_t* output_length);
char* utils_sha256_hash(const unsigned char* data, size_t length);
unsigned char* utils_hmac_sha256(const unsigned char* data, size_t data_len,
                                  const char* key, size_t key_len,
                                  unsigned int* output_len);
char* utils_password_hash(const char* password, int account_id);
bool utils_random_bytes(unsigned char* buffer, size_t length);
EVP_PKEY* utils_jwk_rsa_to_pkey(const char* jwk_json);
bool utils_rs256_verify(EVP_PKEY* pkey,
                         const unsigned char* input, size_t input_len,
                         const unsigned char* signature, size_t sig_len);

/**
 * Standard Base64 encode data WITH padding (RFC 4648)
 * Used for password hashing to match database storage format
 */
char* utils_base64_encode(const unsigned char* data, size_t length) {
    if (!data || length == 0) return NULL;

    // Calculate output length WITH padding
    size_t output_length = ((length + 2) / 3) * 4;
    char* encoded = calloc(output_length + 1, sizeof(char));
    if (!encoded) return NULL;

    size_t i, j;
    for (i = 0, j = 0; i < length; i += 3) {
        uint32_t octet_a = data[i];
        uint32_t octet_b = (i + 1 < length) ? data[i + 1] : 0;
        uint32_t octet_c = (i + 2 < length) ? data[i + 2] : 0;

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        encoded[j++] = base64_table[(triple >> 18) & 0x3F];
        encoded[j++] = base64_table[(triple >> 12) & 0x3F];
        encoded[j++] = (i + 1 < length) ? base64_table[(triple >> 6) & 0x3F] : '=';
        encoded[j++] = (i + 2 < length) ? base64_table[triple & 0x3F] : '=';
    }

    // String is already null-terminated from calloc
    return encoded;
}

/**
 * Base64url encode data WITHOUT padding (URL-safe)
 * Used for JWTs and other URL-safe encoding needs
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
        const char* ptr_a = strchr(base64url_table, input[i]);
        if (input[i] != '=' && !ptr_a) {
            free(decoded);
            return NULL; // Invalid character
        }
        uint32_t sextet_a = input[i] == '=' ? 0 : (uint32_t)(ptr_a - base64url_table);

        const char* ptr_b = (i + 1 < input_length && input[i + 1] != '=') ? strchr(base64url_table, input[i + 1]) : NULL;
        if (i + 1 < input_length && input[i + 1] != '=' && !ptr_b) {
            free(decoded);
            return NULL; // Invalid character
        }
        uint32_t sextet_b = (i + 1 < input_length && input[i + 1] != '=') ? (uint32_t)(ptr_b - base64url_table) : 0;

        const char* ptr_c = (i + 2 < input_length && input[i + 2] != '=') ? strchr(base64url_table, input[i + 2]) : NULL;
        if (i + 2 < input_length && input[i + 2] != '=' && !ptr_c) {
            free(decoded);
            return NULL; // Invalid character
        }
        uint32_t sextet_c = (i + 2 < input_length && input[i + 2] != '=') ? (uint32_t)(ptr_c - base64url_table) : 0;

        const char* ptr_d = (i + 3 < input_length && input[i + 3] != '=') ? strchr(base64url_table, input[i + 3]) : NULL;
        if (i + 3 < input_length && input[i + 3] != '=' && !ptr_d) {
            free(decoded);
            return NULL; // Invalid character
        }
        uint32_t sextet_d = (i + 3 < input_length && input[i + 3] != '=') ? (uint32_t)(ptr_d - base64url_table) : 0;

        uint32_t triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;

        if (j < decoded_length) decoded[j] = (triple >> 16) & 0xFF;
        if (j + 1 < decoded_length) decoded[j + 1] = (triple >> 8) & 0xFF;
        if (j + 2 < decoded_length) decoded[j + 2] = triple & 0xFF;
    }

    // Adjust decoded length if there were padding characters
    if (input_length >= 2 && input[input_length - 1] == '=' && input[input_length - 2] == '=') {
        decoded_length -= 2;
    } else if (input_length >= 1 && input[input_length - 1] == '=') {
        decoded_length -= 1;
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
 * Uses standard Base64 encoding WITH padding to match database format
 */
char* utils_password_hash(const char* password, int account_id) {
    if (!password) return NULL;

    char combined[256];
    snprintf(combined, sizeof(combined), "%d%s", account_id, password);

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)combined, strlen(combined), hash);

    // Use standard base64 WITH padding to match database storage
    return utils_base64_encode(hash, sizeof(hash));
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

/**
 * Parse an RSA JWK into an EVP_PKEY public key.
 *
 * Implementation notes:
 *  - Only `kty == "RSA"` is supported. ES256/PS256/etc. land in a
 *    later phase if ever needed; the Phase 5 `AllowedAlgorithms`
 *    config already gates which algorithms callers will request.
 *  - Private-key parameters (`d`, `p`, `q`, `dp`, `dq`, `qi`) are
 *    intentionally ignored. The returned key is public-only.
 *  - Uses the OpenSSL 3.x `EVP_PKEY_fromdata` API. OpenSSL 1.1.x is
 *    not supported (the project's minimum is OpenSSL 3.x; verified in
 *    Phase 12 setup).
 *  - On any failure path, a single structured log line is emitted at
 *    LOG_LEVEL_ALERT under "CRYPTO" so callers can omit error
 *    handling beyond NULL-checking the return value.
 */
EVP_PKEY* utils_jwk_rsa_to_pkey(const char* jwk_json) {
    if (!jwk_json) return NULL;

    json_error_t jerr;
    json_t* root = json_loads(jwk_json, 0, &jerr);
    if (!root) {
        log_this("CRYPTO", "JWK parse failed: invalid JSON", LOG_LEVEL_ALERT, 0);
        return NULL;
    }
    if (!json_is_object(root)) {
        log_this("CRYPTO", "JWK parse failed: not a JSON object", LOG_LEVEL_ALERT, 0);
        json_decref(root);
        return NULL;
    }

    const json_t* j_kty = json_object_get(root, "kty");
    const json_t* j_n = json_object_get(root, "n");
    const json_t* j_e = json_object_get(root, "e");

    if (!json_is_string(j_kty) || !json_is_string(j_n) || !json_is_string(j_e)) {
        log_this("CRYPTO", "JWK parse failed: missing or non-string kty/n/e", LOG_LEVEL_ALERT, 0);
        json_decref(root);
        return NULL;
    }

    if (strcmp(json_string_value(j_kty), "RSA") != 0) {
        log_this("CRYPTO",
                 "JWK parse failed: unsupported kty (only RSA supported)",
                 LOG_LEVEL_ALERT, 0);
        json_decref(root);
        return NULL;
    }

    size_t n_len = 0, e_len = 0;
    unsigned char* n_bytes = utils_base64url_decode(json_string_value(j_n), &n_len);
    unsigned char* e_bytes = utils_base64url_decode(json_string_value(j_e), &e_len);

    if (!n_bytes || !e_bytes || n_len == 0 || e_len == 0) {
        log_this("CRYPTO", "JWK parse failed: base64url decode of n/e failed", LOG_LEVEL_ALERT, 0);
        free(n_bytes);
        free(e_bytes);
        json_decref(root);
        return NULL;
    }

    EVP_PKEY* pkey = NULL;
    EVP_PKEY_CTX* pctx = NULL;
    OSSL_PARAM_BLD* bld = OSSL_PARAM_BLD_new();
    OSSL_PARAM* params = NULL;
    BIGNUM* bn_n = BN_bin2bn(n_bytes, (int)n_len, NULL);
    BIGNUM* bn_e = BN_bin2bn(e_bytes, (int)e_len, NULL);

    if (!bld || !bn_n || !bn_e) {
        log_this("CRYPTO", "JWK->EVP_PKEY: BIGNUM/PARAM_BLD allocation failed", LOG_LEVEL_ALERT, 0);
        goto cleanup;
    }

    if (!OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_N, bn_n) ||
        !OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_E, bn_e)) {
        log_this("CRYPTO", "JWK->EVP_PKEY: failed to push RSA parameters", LOG_LEVEL_ALERT, 0);
        goto cleanup;
    }

    params = OSSL_PARAM_BLD_to_param(bld);
    if (!params) {
        log_this("CRYPTO", "JWK->EVP_PKEY: PARAM_BLD_to_param failed", LOG_LEVEL_ALERT, 0);
        goto cleanup;
    }

    pctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
    if (!pctx) {
        log_this("CRYPTO", "JWK->EVP_PKEY: EVP_PKEY_CTX_new_from_name failed", LOG_LEVEL_ALERT, 0);
        goto cleanup;
    }

    if (EVP_PKEY_fromdata_init(pctx) <= 0 ||
        EVP_PKEY_fromdata(pctx, &pkey, EVP_PKEY_PUBLIC_KEY, params) <= 0) {
        log_this("CRYPTO", "JWK->EVP_PKEY: EVP_PKEY_fromdata failed", LOG_LEVEL_ALERT, 0);
        if (pkey) {
            EVP_PKEY_free(pkey);
            pkey = NULL;
        }
        goto cleanup;
    }

cleanup:
    // All cleanup below runs on both success and failure paths.
    // `pkey` is the only resource that survives the function — it
    // lives independently of the construction-time inputs (OpenSSL
    // copies the BIGNUMs out of the OSSL_PARAM array internally).
    // Every resource freed here is consumed by `EVP_PKEY_fromdata`
    // through OSSL_PARAM, NOT retained by the resulting `pkey`. So
    // the success path is correct: free everything except `pkey`.
    if (params) OSSL_PARAM_free(params);
    if (bld) OSSL_PARAM_BLD_free(bld);
    if (pctx) EVP_PKEY_CTX_free(pctx);
    if (bn_n) BN_free(bn_n);
    if (bn_e) BN_free(bn_e);
    free(n_bytes);
    free(e_bytes);
    json_decref(root);
    return pkey;
}

/**
 * Verify an RS256 signature.
 *
 * Bad signature ⇒ silent false. OpenSSL internal error ⇒ logged false.
 */
bool utils_rs256_verify(EVP_PKEY* pkey,
                         const unsigned char* input, size_t input_len,
                         const unsigned char* signature, size_t sig_len) {
    if (!pkey || !input || input_len == 0 || !signature || sig_len == 0) {
        return false;
    }

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        log_this("CRYPTO", "RS256 verify: EVP_MD_CTX_new failed", LOG_LEVEL_ALERT, 0);
        return false;
    }

    bool ok = false;
    if (EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, pkey) != 1) {
        log_this("CRYPTO", "RS256 verify: EVP_DigestVerifyInit failed", LOG_LEVEL_ALERT, 0);
        EVP_MD_CTX_free(mdctx);
        return false;
    }

    int rc = EVP_DigestVerify(mdctx, signature, sig_len, input, input_len);
    if (rc == 1) {
        ok = true;
    } else if (rc < 0) {
        // Negative return = OpenSSL internal error (not just a bad
        // signature). Bad signature is rc == 0.
        log_this("CRYPTO", "RS256 verify: EVP_DigestVerify internal error", LOG_LEVEL_ALERT, 0);
    }

    EVP_MD_CTX_free(mdctx);
    return ok;
}
