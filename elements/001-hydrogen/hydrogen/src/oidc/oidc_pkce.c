/*
 * OIDC IdP PKCE (S256) helpers
 */

#include <src/hydrogen.h>

#include "oidc_pkce.h"

#include <src/utils/utils_crypto.h>
#include <string.h>
#include <openssl/crypto.h>

char* oidc_pkce_make_challenge_s256(const char *verifier);
bool oidc_pkce_verify_s256(const char *verifier, const char *expected_challenge);
bool oidc_pkce_method_is_s256(const char *method);

char* oidc_pkce_make_challenge_s256(const char *verifier) {
    if (!verifier || verifier[0] == '\0') {
        return NULL;
    }
    return utils_sha256_hash((const unsigned char*)verifier, strlen(verifier));
}

bool oidc_pkce_method_is_s256(const char *method) {
    return method != NULL && strcmp(method, "S256") == 0;
}

bool oidc_pkce_verify_s256(const char *verifier, const char *expected_challenge) {
    if (!verifier || !expected_challenge || expected_challenge[0] == '\0') {
        return false;
    }

    char *computed = oidc_pkce_make_challenge_s256(verifier);
    if (!computed) {
        return false;
    }

    size_t a_len = strlen(expected_challenge);
    size_t b_len = strlen(computed);
    bool match = false;
    if (a_len == b_len && a_len > 0U) {
        match = (CRYPTO_memcmp(expected_challenge, computed, a_len) == 0);
    }
    free(computed);
    return match;
}
