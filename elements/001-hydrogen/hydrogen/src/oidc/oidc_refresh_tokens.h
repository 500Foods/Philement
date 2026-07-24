/*
 * OIDC refresh token store (in-memory for Unity; DB QueryRefs #139-#142 later).
 */

#ifndef OIDC_REFRESH_TOKENS_H
#define OIDC_REFRESH_TOKENS_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#define OIDC_REFRESH_DEFAULT_TTL_SEC 86400
#define OIDC_REFRESH_HASH_MAX 128
#define OIDC_REFRESH_CLIENT_MAX 128
#define OIDC_REFRESH_SCOPE_MAX 500
#define OIDC_REFRESH_FAMILY_MAX 64

typedef struct OIDCRefreshRecord {
    char token_hash[OIDC_REFRESH_HASH_MAX + 1];
    char client_id[OIDC_REFRESH_CLIENT_MAX + 1];
    int account_id;
    char scope[OIDC_REFRESH_SCOPE_MAX + 1];
    char family_id[OIDC_REFRESH_FAMILY_MAX + 1];
    time_t expires_at;
    time_t revoked_at; /* 0 if active */
} OIDCRefreshRecord;

typedef struct OIDCRefreshStore {
    OIDCRefreshRecord **records;
    size_t count;
    size_t capacity;
    int default_ttl_seconds;
} OIDCRefreshStore;

OIDCRefreshStore* oidc_refresh_store_create(int default_ttl_seconds);
void oidc_refresh_store_destroy(OIDCRefreshStore *store);

char* oidc_refresh_token_hash(const char *plaintext);

/*
 * Issue opaque refresh token. Returns plaintext (caller frees) or NULL.
 * family_id_or_null: if NULL, generates a new family id.
 */
char* oidc_refresh_issue(OIDCRefreshStore *store,
                         const char *client_id,
                         int account_id,
                         const char *scope,
                         const char *family_id_or_null);

/*
 * Validate and rotate: revoke presented token, issue new one in same family.
 * On reuse of already-revoked token, revokes entire family and fails.
 * out_record receives pre-rotation metadata on success (optional).
 * new_plaintext_out receives new refresh token (caller frees) on success.
 */
bool oidc_refresh_rotate(OIDCRefreshStore *store,
                         const char *plaintext_token,
                         const char *client_id,
                         OIDCRefreshRecord *out_record,
                         char **new_plaintext_out);

/* Revoke one token by plaintext (hash match). */
bool oidc_refresh_revoke(OIDCRefreshStore *store, const char *plaintext_token);

/* Revoke all tokens in a family. */
void oidc_refresh_revoke_family(OIDCRefreshStore *store, const char *family_id);

/*
 * Lookup refresh by plaintext without rotating. Copies metadata into out_record.
 * Returns true if hash found (even if revoked/expired).
 */
bool oidc_refresh_lookup(OIDCRefreshStore *store,
                         const char *plaintext_token,
                         OIDCRefreshRecord *out_record);

#endif /* OIDC_REFRESH_TOKENS_H */
