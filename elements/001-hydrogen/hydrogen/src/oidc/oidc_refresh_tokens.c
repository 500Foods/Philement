/*
 * OIDC refresh token store (in-memory)
 */

#include <src/hydrogen.h>

#include "oidc_refresh_tokens.h"

#include <src/utils/utils_crypto.h>
#include <src/logging/logging.h>

#include <string.h>
#include <time.h>

void oidc_refresh_record_free(OIDCRefreshRecord *rec);
bool oidc_refresh_store_add(OIDCRefreshStore *store, OIDCRefreshRecord *rec);
OIDCRefreshRecord* oidc_refresh_store_find_hash(OIDCRefreshStore *store,
                                                const char *token_hash);

void oidc_refresh_record_free(OIDCRefreshRecord *rec) {
    free(rec);
}

OIDCRefreshStore* oidc_refresh_store_create(int default_ttl_seconds) {
    OIDCRefreshStore *store = (OIDCRefreshStore*)calloc(1, sizeof(OIDCRefreshStore));
    if (!store) {
        return NULL;
    }
    store->default_ttl_seconds = default_ttl_seconds > 0
        ? default_ttl_seconds
        : OIDC_REFRESH_DEFAULT_TTL_SEC;
    return store;
}

void oidc_refresh_store_destroy(OIDCRefreshStore *store) {
    if (!store) {
        return;
    }
    if (store->records) {
        size_t i;
        for (i = 0; i < store->count; i++) {
            oidc_refresh_record_free(store->records[i]);
        }
        free(store->records);
    }
    free(store);
}

char* oidc_refresh_token_hash(const char *plaintext) {
    if (!plaintext || plaintext[0] == '\0') {
        return NULL;
    }
    return utils_sha256_hash((const unsigned char*)plaintext, strlen(plaintext));
}

bool oidc_refresh_store_add(OIDCRefreshStore *store, OIDCRefreshRecord *rec) {
    if (!store || !rec) {
        return false;
    }
    if (store->count >= store->capacity) {
        size_t new_cap = store->capacity == 0U ? 8U : store->capacity * 2U;
        OIDCRefreshRecord **grown = (OIDCRefreshRecord**)realloc(
            store->records, new_cap * sizeof(OIDCRefreshRecord*));
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

OIDCRefreshRecord* oidc_refresh_store_find_hash(OIDCRefreshStore *store,
                                                const char *token_hash) {
    if (!store || !token_hash || !store->records) {
        return NULL;
    }
    size_t i;
    for (i = 0; i < store->count; i++) {
        OIDCRefreshRecord *r = store->records[i];
        if (r && strcmp(r->token_hash, token_hash) == 0) {
            return r;
        }
    }
    return NULL;
}

void oidc_refresh_revoke_family(OIDCRefreshStore *store, const char *family_id) {
    if (!store || !family_id || !store->records) {
        return;
    }
    time_t now = time(NULL);
    size_t i;
    for (i = 0; i < store->count; i++) {
        OIDCRefreshRecord *r = store->records[i];
        if (r && strcmp(r->family_id, family_id) == 0 && r->revoked_at == 0) {
            r->revoked_at = now;
        }
    }
}

char* oidc_refresh_issue(OIDCRefreshStore *store,
                         const char *client_id,
                         int account_id,
                         const char *scope,
                         const char *family_id_or_null) {
    if (!store || !client_id || account_id <= 0 || !scope) {
        return NULL;
    }

    unsigned char raw[32];
    if (!utils_random_bytes(raw, sizeof(raw))) {
        return NULL;
    }
    char *plaintext = utils_base64url_encode(raw, sizeof(raw));
    {
        volatile unsigned char *p = raw;
        size_t i;
        for (i = 0; i < sizeof(raw); i++) {
            p[i] = 0;
        }
    }
    if (!plaintext) {
        return NULL;
    }

    char *hash = oidc_refresh_token_hash(plaintext);
    if (!hash) {
        free(plaintext);
        return NULL;
    }

    char family_buf[OIDC_REFRESH_FAMILY_MAX + 1];
    if (family_id_or_null && family_id_or_null[0] != '\0') {
        snprintf(family_buf, sizeof(family_buf), "%s", family_id_or_null);
    } else {
        unsigned char fam_raw[16];
        if (!utils_random_bytes(fam_raw, sizeof(fam_raw))) {
            free(hash);
            free(plaintext);
            return NULL;
        }
        char *fam = utils_base64url_encode(fam_raw, sizeof(fam_raw));
        if (!fam) {
            free(hash);
            free(plaintext);
            return NULL;
        }
        snprintf(family_buf, sizeof(family_buf), "%s", fam);
        free(fam);
    }

    OIDCRefreshRecord *rec = (OIDCRefreshRecord*)calloc(1, sizeof(OIDCRefreshRecord));
    if (!rec) {
        free(hash);
        free(plaintext);
        return NULL;
    }

    snprintf(rec->token_hash, sizeof(rec->token_hash), "%s", hash);
    free(hash);
    snprintf(rec->client_id, sizeof(rec->client_id), "%s", client_id);
    rec->account_id = account_id;
    snprintf(rec->scope, sizeof(rec->scope), "%s", scope);
    snprintf(rec->family_id, sizeof(rec->family_id), "%s", family_buf);
    rec->expires_at = time(NULL) + (time_t)store->default_ttl_seconds;
    rec->revoked_at = 0;

    if (!oidc_refresh_store_add(store, rec)) {
        oidc_refresh_record_free(rec);
        free(plaintext);
        return NULL;
    }

    log_this(SR_OIDC, "Issued refresh token", LOG_LEVEL_DEBUG, 0);
    return plaintext;
}

bool oidc_refresh_revoke(OIDCRefreshStore *store, const char *plaintext_token) {
    if (!store || !plaintext_token) {
        return false;
    }
    char *hash = oidc_refresh_token_hash(plaintext_token);
    if (!hash) {
        return false;
    }
    OIDCRefreshRecord *rec = oidc_refresh_store_find_hash(store, hash);
    free(hash);
    if (!rec) {
        return false;
    }
    if (rec->revoked_at == 0) {
        rec->revoked_at = time(NULL);
    }
    return true;
}

bool oidc_refresh_lookup(OIDCRefreshStore *store,
                         const char *plaintext_token,
                         OIDCRefreshRecord *out_record) {
    if (!store || !plaintext_token || !out_record) {
        return false;
    }
    char *hash = oidc_refresh_token_hash(plaintext_token);
    if (!hash) {
        return false;
    }
    OIDCRefreshRecord *rec = oidc_refresh_store_find_hash(store, hash);
    free(hash);
    if (!rec) {
        return false;
    }
    *out_record = *rec;
    return true;
}

bool oidc_refresh_rotate(OIDCRefreshStore *store,
                         const char *plaintext_token,
                         const char *client_id,
                         OIDCRefreshRecord *out_record,
                         char **new_plaintext_out) {
    if (new_plaintext_out) {
        *new_plaintext_out = NULL;
    }
    if (!store || !plaintext_token || !client_id) {
        return false;
    }

    char *hash = oidc_refresh_token_hash(plaintext_token);
    if (!hash) {
        return false;
    }
    OIDCRefreshRecord *rec = oidc_refresh_store_find_hash(store, hash);
    free(hash);
    if (!rec) {
        return false;
    }

    if (strcmp(rec->client_id, client_id) != 0) {
        return false;
    }

    time_t now = time(NULL);

    /* Reuse detection: already revoked → burn family */
    if (rec->revoked_at != 0) {
        log_this(SR_OIDC, "Refresh token reuse detected; revoking family", LOG_LEVEL_ALERT, 0);
        oidc_refresh_revoke_family(store, rec->family_id);
        return false;
    }

    if (rec->expires_at < now) {
        rec->revoked_at = now;
        return false;
    }

    if (out_record) {
        *out_record = *rec;
    }

    /* Rotate: revoke old, issue new in same family */
    rec->revoked_at = now;
    char *fresh = oidc_refresh_issue(store, rec->client_id, rec->account_id,
                                     rec->scope, rec->family_id);
    if (!fresh) {
        return false;
    }
    if (new_plaintext_out) {
        *new_plaintext_out = fresh;
    } else {
        free(fresh);
    }
    return true;
}
