/*
 * OIDC authorization code store (in-memory for Unity; DB QueryRefs #136-#138
 * for blackbox / multi-process).
 */

#ifndef OIDC_AUTH_CODES_H
#define OIDC_AUTH_CODES_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#define OIDC_AUTH_CODE_DEFAULT_TTL_SEC 300
#define OIDC_AUTH_CODE_HASH_MAX 128
#define OIDC_AUTH_CODE_CLIENT_MAX 128
#define OIDC_AUTH_CODE_REDIRECT_MAX 500
#define OIDC_AUTH_CODE_SCOPE_MAX 500
#define OIDC_AUTH_CODE_NONCE_MAX 500
#define OIDC_AUTH_CODE_CHALLENGE_MAX 128
#define OIDC_AUTH_CODE_METHOD_MAX 20

typedef struct OIDCAuthCodeRecord {
    char code_hash[OIDC_AUTH_CODE_HASH_MAX + 1];
    char client_id[OIDC_AUTH_CODE_CLIENT_MAX + 1];
    int account_id;
    char redirect_uri[OIDC_AUTH_CODE_REDIRECT_MAX + 1];
    char scope[OIDC_AUTH_CODE_SCOPE_MAX + 1];
    char nonce[OIDC_AUTH_CODE_NONCE_MAX + 1];
    char code_challenge[OIDC_AUTH_CODE_CHALLENGE_MAX + 1];
    char code_challenge_method[OIDC_AUTH_CODE_METHOD_MAX + 1];
    time_t expires_at;
    time_t consumed_at; /* 0 if not consumed */
} OIDCAuthCodeRecord;

typedef struct OIDCAuthCodeStore {
    OIDCAuthCodeRecord **records;
    size_t count;
    size_t capacity;
    int default_ttl_seconds;
} OIDCAuthCodeStore;

OIDCAuthCodeStore* oidc_auth_code_store_create(int default_ttl_seconds);
void oidc_auth_code_store_destroy(OIDCAuthCodeStore *store);

/* Hash plaintext code for storage. Caller frees. */
char* oidc_auth_code_hash(const char *plaintext_code);

/*
 * Generate random code, store hash+metadata, return plaintext code (caller frees).
 * Requires S256 challenge/method.
 */
char* oidc_auth_code_issue(OIDCAuthCodeStore *store,
                           const char *client_id,
                           int account_id,
                           const char *redirect_uri,
                           const char *scope,
                           const char *nonce_or_null,
                           const char *code_challenge,
                           const char *code_challenge_method);

/*
 * Validate and consume. On success fills out_record (optional) and returns true.
 * Fails on unknown, expired, already consumed, client/redirect mismatch, bad PKCE.
 */
bool oidc_auth_code_consume(OIDCAuthCodeStore *store,
                            const char *plaintext_code,
                            const char *client_id,
                            const char *redirect_uri,
                            const char *code_verifier,
                            OIDCAuthCodeRecord *out_record);

#endif /* OIDC_AUTH_CODES_H */
