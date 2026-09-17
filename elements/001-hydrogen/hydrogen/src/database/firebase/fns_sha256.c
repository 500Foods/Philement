/*
 * Firebase password-hash contract: SHA256(concat(account_id, password))
 * then RFC 4648 Base64. Bytes must match SQLite crypto_sha256 + encode.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/utils/utils_crypto.h>
#include "fns_sha256.h"

#include <openssl/sha.h>

char* firebase_sha256_b64(const char* a, const char* b) {
    if (!a || !b) {
        return NULL;
    }

    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    size_t total = len_a + len_b;
    unsigned char* combined = malloc(total + 1);
    if (!combined) {
        return NULL;
    }
    memcpy(combined, a, len_a);
    memcpy(combined + len_a, b, len_b);
    combined[total] = '\0';

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(combined, total, hash);
    free(combined);

    return utils_base64_encode(hash, SHA256_DIGEST_LENGTH);
}
