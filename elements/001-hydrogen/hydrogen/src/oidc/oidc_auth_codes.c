/*
 * OIDC authorization code store (in-memory)
 */

#include <src/hydrogen.h>

#include "oidc_auth_codes.h"
#include "oidc_pkce.h"

#include <src/utils/utils_crypto.h>
#include <src/logging/logging.h>

#include <string.h>
#include <time.h>

OIDCAuthCodeStore* oidc_auth_code_store_create(int default_ttl_seconds);
void oidc_auth_code_store_destroy(OIDCAuthCodeStore *store);
char* oidc_auth_code_hash(const char *plaintext_code);
char* oidc_auth_code_issue(OIDCAuthCodeStore *store,
                           const char *client_id,
                           int account_id,
                           const char *redirect_uri,
                           const char *scope,
                           const char *nonce_or_null,
                           const char *code_challenge,
                           const char *code_challenge_method);
bool oidc_auth_code_consume(OIDCAuthCodeStore *store,
                            const char *plaintext_code,
                            const char *client_id,
                            const char *redirect_uri,
                            const char *code_verifier,
                            OIDCAuthCodeRecord *out_record);

void oidc_auth_code_record_free(OIDCAuthCodeRecord *rec);
bool oidc_auth_code_store_add(OIDCAuthCodeStore *store, OIDCAuthCodeRecord *rec);
OIDCAuthCodeRecord* oidc_auth_code_store_find_hash(OIDCAuthCodeStore *store,
                                                   const char *code_hash);

void oidc_auth_code_record_free(OIDCAuthCodeRecord *rec) {
    free(rec);
}

OIDCAuthCodeStore* oidc_auth_code_store_create(int default_ttl_seconds) {
    OIDCAuthCodeStore *store = (OIDCAuthCodeStore*)calloc(1, sizeof(OIDCAuthCodeStore));
    if (!store) {
        return NULL;
    }
    store->default_ttl_seconds = default_ttl_seconds > 0
        ? default_ttl_seconds
        : OIDC_AUTH_CODE_DEFAULT_TTL_SEC;
    return store;
}

void oidc_auth_code_store_destroy(OIDCAuthCodeStore *store) {
    if (!store) {
        return;
    }
    if (store->records) {
        for (size_t i = 0; i < store->count; i++) {
            oidc_auth_code_record_free(store->records[i]);
        }
        free(store->records);
    }
    free(store);
}

char* oidc_auth_code_hash(const char *plaintext_code) {
    if (!plaintext_code || plaintext_code[0] == '\0') {
        return NULL;
    }
    return utils_sha256_hash((const unsigned char*)plaintext_code, strlen(plaintext_code));
}

bool oidc_auth_code_store_add(OIDCAuthCodeStore *store, OIDCAuthCodeRecord *rec) {
    if (!store || !rec) {
        return false;
    }
    if (store->count >= store->capacity) {
        size_t new_cap = store->capacity == 0U ? 8U : store->capacity * 2U;
        OIDCAuthCodeRecord **grown = (OIDCAuthCodeRecord**)realloc(
            store->records, new_cap * sizeof(OIDCAuthCodeRecord*));
        if (!grown) {
            return false;
        }
        store->records = grown;
        store->capacity = new_cap;
    }
    store->records[store->count] = rec;
    store->count++;
    return true;
}

OIDCAuthCodeRecord* oidc_auth_code_store_find_hash(OIDCAuthCodeStore *store,
                                                   const char *code_hash) {
    if (!store || !code_hash || !store->records) {
        return NULL;
    }
    for (size_t i = 0; i < store->count; i++) {
        OIDCAuthCodeRecord *r = store->records[i];
        if (r && strcmp(r->code_hash, code_hash) == 0) {
            return r;
        }
    }
    return NULL;
}

char* oidc_auth_code_issue(OIDCAuthCodeStore *store,
                           const char *client_id,
                           int account_id,
                           const char *redirect_uri,
                           const char *scope,
                           const char *nonce_or_null,
                           const char *code_challenge,
                           const char *code_challenge_method) {
    if (!store || !client_id || account_id <= 0 || !redirect_uri || !scope ||
        !code_challenge || !oidc_pkce_method_is_s256(code_challenge_method)) {
        return NULL;
    }

    unsigned char raw[32];
    if (!utils_random_bytes(raw, sizeof(raw))) {
        return NULL;
    }
    char *plaintext = utils_base64url_encode(raw, sizeof(raw));
    {
        volatile unsigned char *p = raw;
        for (size_t i = 0; i < sizeof(raw); i++) {
            p[i] = 0;
        }
    }
    if (!plaintext) {
        return NULL;
    }

    char *hash = oidc_auth_code_hash(plaintext);
    if (!hash) {
        free(plaintext);
        return NULL;
    }

    OIDCAuthCodeRecord *rec = (OIDCAuthCodeRecord*)calloc(1, sizeof(OIDCAuthCodeRecord));
    if (!rec) {
        free(hash);
        free(plaintext);
        return NULL;
    }

    snprintf(rec->code_hash, sizeof(rec->code_hash), "%s", hash);
    free(hash);
    snprintf(rec->client_id, sizeof(rec->client_id), "%s", client_id);
    rec->account_id = account_id;
    snprintf(rec->redirect_uri, sizeof(rec->redirect_uri), "%s", redirect_uri);
    snprintf(rec->scope, sizeof(rec->scope), "%s", scope);
    if (nonce_or_null) {
        snprintf(rec->nonce, sizeof(rec->nonce), "%s", nonce_or_null);
    }
    snprintf(rec->code_challenge, sizeof(rec->code_challenge), "%s", code_challenge);
    snprintf(rec->code_challenge_method, sizeof(rec->code_challenge_method), "%s",
             code_challenge_method);
    rec->expires_at = time(NULL) + (time_t)store->default_ttl_seconds;
    rec->consumed_at = 0;

    if (!oidc_auth_code_store_add(store, rec)) {
        oidc_auth_code_record_free(rec);
        free(plaintext);
        return NULL;
    }

    log_this(SR_OIDC, "Issued authorization code", LOG_LEVEL_DEBUG, 0);
    return plaintext;
}

bool oidc_auth_code_consume(OIDCAuthCodeStore *store,
                            const char *plaintext_code,
                            const char *client_id,
                            const char *redirect_uri,
                            const char *code_verifier,
                            OIDCAuthCodeRecord *out_record) {
    if (!store || !plaintext_code || !client_id || !redirect_uri || !code_verifier) {
        return false;
    }

    char *hash = oidc_auth_code_hash(plaintext_code);
    if (!hash) {
        return false;
    }

    OIDCAuthCodeRecord *rec = oidc_auth_code_store_find_hash(store, hash);
    free(hash);
    if (!rec) {
        return false;
    }

    time_t now = time(NULL);
    if (rec->consumed_at != 0) {
        return false;
    }
    if (rec->expires_at < now) {
        return false;
    }
    if (strcmp(rec->client_id, client_id) != 0) {
        return false;
    }
    if (strcmp(rec->redirect_uri, redirect_uri) != 0) {
        return false;
    }
    if (!oidc_pkce_method_is_s256(rec->code_challenge_method)) {
        return false;
    }
    if (!oidc_pkce_verify_s256(code_verifier, rec->code_challenge)) {
        return false;
    }

    rec->consumed_at = now;

    if (out_record) {
        *out_record = *rec;
    }
    return true;
}
