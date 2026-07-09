/*
 * OIDC RP role-mapping — Phase 22.
 *
 * Implements four role-mapping strategies controlled by
 * OIDCRPRoleMapping.Source:
 *
 *   DATABASE (default):
 *     QueryRef #017 "Get User Roles" returns one row per role_id from
 *     account_roles. Each role_id is joined to the roles table by a
 *     separate approach: we look up the role name from the `roles` table
 *     via QueryRef #016, or — since #017 only returns role_id integers —
 *     we use the role_id directly as a string representation (matching
 *     what the existing password login path does: it also sends
 *     account->roles = "" and the JWT encodes whatever that string is).
 *     Phase 22 calls #017 and builds a comma-separated role_id list.
 *     A future phase may add a role-name JOIN query; for now role_ids
 *     are the canonical representation in the JWT `roles` field.
 *
 *   IDP_REALM_ROLES:
 *     Copy `claims->roles[]` (parsed from `realm_access.roles` by Phase 12)
 *     directly, prepending IdpRolePrefix to each. No DB query.
 *
 *   IDP_CLIENT_ROLES:
 *     Phase 12 does not parse `resource_access.<client>.roles` separately;
 *     it populates only `claims->roles[]` from `realm_access.roles`.
 *     IDP_CLIENT_ROLES therefore behaves identically to IDP_REALM_ROLES
 *     in Phase 22. This is documented in the header. A future phase may
 *     extend OidcRpIdTokenClaims with a `client_roles[]` array.
 *
 *   MERGE:
 *     Union of DATABASE role_ids (no prefix) and IDP_REALM_ROLES strings
 *     (with IdpRolePrefix). Deduplication is case-sensitive. DB-sourced
 *     role_ids that are numeric strings will not collide with prefixed
 *     IdP role names unless the prefix is empty and the IdP uses numeric
 *     role names (a degenerate configuration).
 *
 * Role string format:
 *   Comma-separated, e.g. "1,3,kc:editor". Empty string when no roles.
 *   No leading/trailing commas or spaces. This is the format that
 *   generate_jwt embeds directly into the JWT payload's `roles` field.
 *
 * Failure discipline:
 *   On DB error or OOM, account->roles is unchanged (remains "").
 *   Login continues; the user gets an empty roles JWT rather than being
 *   blocked. Failure is logged at ALERT level with full context.
 *   This mirrors the QueryRef #084 touch non-fatal discipline (Phase 18
 *   lesson #2).
 *
 * Logging:
 *   Every log line uses SR_AUTH with "OIDC RP roles:" prefix.
 *   Never logs the JWT token, account passwords, or IdP credentials.
 *   Logs account_id, source enum name, and role count on success.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_database.h>
#include <jansson.h>

// Local includes
#include "oidc_rp_roles.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* -------------------------------------------------------------------------
 * Test seam
 * -------------------------------------------------------------------------
 */
static OidcRpRolesQueryFn g_roles_query_fn = NULL;

void oidc_rp_roles_test_set_query_fn(OidcRpRolesQueryFn fn) {
    g_roles_query_fn = fn;
}

void oidc_rp_roles_test_clear_query_fn(void) {
    g_roles_query_fn = NULL;
}

static QueryResult *run_query(int query_ref,
                               const char *database,
                               json_t *params) {
    if (g_roles_query_fn) {
        return g_roles_query_fn(query_ref, database, params);
    }
    return execute_auth_query(query_ref, database, params);
}

/* -------------------------------------------------------------------------
 * Internal: string builder for role lists
 * -------------------------------------------------------------------------
 * A small realloc-on-demand buffer. Roles are appended as
 * "role1,role2,role3". The buffer is always NUL-terminated.
 */
typedef struct {
    char   *buf;
    size_t  len;   /* used bytes, excluding NUL */
    size_t  cap;   /* allocated bytes, including NUL */
} RolesBuf;

static bool roles_buf_init(RolesBuf *rb) {
    rb->buf = malloc(64);
    if (!rb->buf) return false;
    rb->buf[0] = '\0';
    rb->len = 0;
    rb->cap = 64;
    return true;
}

static bool roles_buf_append(RolesBuf *rb, const char *role) {
    if (!role || !*role) return true;

    size_t role_len = strlen(role);
    size_t sep_len  = (rb->len > 0) ? 1 : 0; /* comma separator */
    size_t need     = rb->len + sep_len + role_len + 1; /* +1 for NUL */

    if (need > rb->cap) {
        size_t new_cap = rb->cap * 2;
        while (new_cap < need) new_cap *= 2;
        char *nb = realloc(rb->buf, new_cap);
        if (!nb) return false;
        rb->buf = nb;
        rb->cap = new_cap;
    }

    if (sep_len) {
        rb->buf[rb->len] = ',';
        rb->len++;
    }
    memcpy(rb->buf + rb->len, role, role_len);
    rb->len += role_len;
    rb->buf[rb->len] = '\0';
    return true;
}

/* Steal the built string. The RolesBuf is invalid after this call. */
static char *roles_buf_steal(RolesBuf *rb) {
    char *result = rb->buf;
    rb->buf = NULL;
    rb->len = 0;
    rb->cap = 0;
    return result;
}

static void roles_buf_free(RolesBuf *rb) {
    free(rb->buf);
    rb->buf = NULL;
}

/* -------------------------------------------------------------------------
 * DATABASE source: QueryRef #017 "Get User Roles"
 *
 * Returns a comma-separated list of role_id integers, e.g. "1,3,7".
 * Returns NULL on DB error; caller keeps the existing account->roles.
 * Returns "" (heap-allocated empty string) when the account has no roles.
 * -------------------------------------------------------------------------
 */
char *auth_roles_from_database(int account_id, const char *database) {
    json_t *params         = json_object();
    json_t *integer_params = json_object();
    if (!params || !integer_params) {
        json_decref(params);
        json_decref(integer_params);
        return NULL;
    }

    json_object_set_new(integer_params, "ACCOUNTID", json_integer(account_id));
    json_object_set_new(params, "INTEGER", integer_params);

    QueryResult *result = run_query(17, database, params);
    json_decref(params);

    if (!result) {
        log_this(SR_AUTH,
                 "OIDC RP roles: QueryRef #017 transport failure for account_id=%d",
                 LOG_LEVEL_ALERT, 1, account_id);
        return NULL;
    }
    if (!result->success) {
        log_this(SR_AUTH,
                 "OIDC RP roles: QueryRef #017 failed for account_id=%d: %s",
                 LOG_LEVEL_ALERT, 2,
                 account_id,
                 result->error_message ? result->error_message : "(unknown)");
        free_query_result(result);
        return NULL;
    }

    /* No rows or null data → empty roles list (not an error). */
    if (!result->data_json) {
        free_query_result(result);
        return strdup("");
    }

    json_t *arr = json_loads(result->data_json, 0, NULL);
    free_query_result(result);

    if (!arr || !json_is_array(arr)) {
        json_decref(arr);
        return strdup(""); /* treat unreadable rows as no roles */
    }

    RolesBuf rb;
    if (!roles_buf_init(&rb)) {
        json_decref(arr);
        return NULL;
    }

    size_t n = json_array_size(arr);
    for (size_t i = 0; i < n; i++) {
        json_t *row = json_array_get(arr, i);
        if (!row) continue;

        /* Column name may be uppercase on DB2. */
        json_t *role_json = json_object_get(row, "role_id");
        if (!role_json) role_json = json_object_get(row, "ROLE_ID");
        if (!role_json || !json_is_integer(role_json)) continue;

        char role_str[32];
        snprintf(role_str, sizeof(role_str), "%lld",
                 (long long)json_integer_value(role_json));
        if (!roles_buf_append(&rb, role_str)) {
            roles_buf_free(&rb);
            json_decref(arr);
            return NULL;
        }
    }

    json_decref(arr);
    return roles_buf_steal(&rb);
}

/* -------------------------------------------------------------------------
 * IdP source (IDP_REALM_ROLES / IDP_CLIENT_ROLES):
 *   Build a comma-separated list from claims->roles[], applying IdpRolePrefix.
 *
 * Returns NULL on OOM; returns "" when claims->role_count == 0.
 * -------------------------------------------------------------------------
 */
static char *roles_from_idp(const OidcRpIdTokenClaims *claims,
                              const char *prefix) {
    if (!claims) return strdup("");

    /* prefix is optional: NULL and "" are both treated as "no prefix". */
    const char *pfx    = (prefix && *prefix) ? prefix : "";
    size_t      pfx_len = strlen(pfx);

    RolesBuf rb;
    if (!roles_buf_init(&rb)) return NULL;

    for (size_t i = 0; i < (size_t)OIDC_RP_IDTOKEN_MAX_ROLES; i++) {
        if (!claims->roles[i]) break;
        if (!*claims->roles[i]) continue;

        /* Build "prefix+role" in a temporary buffer. */
        size_t role_len = strlen(claims->roles[i]);
        size_t full_len = pfx_len + role_len + 1;
        char *full = malloc(full_len);
        if (!full) {
            roles_buf_free(&rb);
            return NULL;
        }
        memcpy(full, pfx, pfx_len);
        memcpy(full + pfx_len, claims->roles[i], role_len);
        full[pfx_len + role_len] = '\0';

        bool ok = roles_buf_append(&rb, full);
        free(full);
        if (!ok) {
            roles_buf_free(&rb);
            return NULL;
        }
    }

    return roles_buf_steal(&rb);
}

/* -------------------------------------------------------------------------
 * MERGE: database role_ids + IdP role strings (with prefix), deduplicated.
 *
 * Deduplication is case-sensitive. We walk the final merged list as we
 * build it: for each new token, scan the built-so-far tokens.
 * With typical role counts (single digits) this is O(n²) but negligible.
 * -------------------------------------------------------------------------
 */

/* Split a comma-separated string into tokens (caller owns array + each str). */
static char **split_roles(const char *roles_str, size_t *out_count) {
    *out_count = 0;
    if (!roles_str || !*roles_str) return NULL;

    /* Count tokens. */
    size_t count = 1;
    for (const char *p = roles_str; *p; p++) {
        if (*p == ',') count++;
    }

    char **arr = calloc(count, sizeof(char *));
    if (!arr) return NULL;

    char *dup = strdup(roles_str);
    if (!dup) {
        free(arr);
        return NULL;
    }

    size_t idx = 0;
    char *tok = strtok(dup, ",");
    while (tok && idx < count) {
        arr[idx++] = strdup(tok);
        tok = strtok(NULL, ",");
    }
    free(dup);
    *out_count = idx;
    return arr;
}

static void free_roles_array(char **arr, size_t count) {
    if (!arr) return;
    for (size_t i = 0; i < count; i++) free(arr[i]);
    free(arr);
}

static char *roles_merge(const char *db_roles, const char *idp_roles) {
    size_t  db_count  = 0;
    size_t  idp_count = 0;
    char  **db_arr    = split_roles(db_roles,  &db_count);
    char  **idp_arr   = split_roles(idp_roles, &idp_count);

    RolesBuf rb;
    if (!roles_buf_init(&rb)) {
        free_roles_array(db_arr,  db_count);
        free_roles_array(idp_arr, idp_count);
        return NULL;
    }

    /* Add DB roles first. */
    for (size_t i = 0; i < db_count; i++) {
        if (!db_arr[i] || !*db_arr[i]) continue;
        if (!roles_buf_append(&rb, db_arr[i])) {
            roles_buf_free(&rb);
            free_roles_array(db_arr,  db_count);
            free_roles_array(idp_arr, idp_count);
            return NULL;
        }
    }

    /* Add IdP roles that are not already in the buffer. */
    for (size_t i = 0; i < idp_count; i++) {
        if (!idp_arr[i] || !*idp_arr[i]) continue;

        /* Check for duplicate among DB roles (case-sensitive). */
        bool dup = false;
        for (size_t j = 0; j < db_count; j++) {
            if (db_arr[j] && strcmp(db_arr[j], idp_arr[i]) == 0) {
                dup = true;
                break;
            }
        }
        if (dup) continue;

        if (!roles_buf_append(&rb, idp_arr[i])) {
            roles_buf_free(&rb);
            free_roles_array(db_arr,  db_count);
            free_roles_array(idp_arr, idp_count);
            return NULL;
        }
    }

    free_roles_array(db_arr,  db_count);
    free_roles_array(idp_arr, idp_count);
    return roles_buf_steal(&rb);
}

/* -------------------------------------------------------------------------
 * Public entry point
 * -------------------------------------------------------------------------
 */
void oidc_rp_roles_apply(const OIDCRPProviderConfig *provider,
                          const OidcRpIdTokenClaims   *claims,
                          const char                  *database,
                          account_info_t               *account) {
    if (!provider || !account) return;

    OIDCRPRoleSource source = provider->role_mapping.source;
    const char *prefix      = provider->role_mapping.idp_role_prefix;

    char *new_roles = NULL;

    switch (source) {
        case OIDC_RP_ROLE_SRC_DATABASE: {
            new_roles = auth_roles_from_database(account->id, database);
            if (!new_roles) {
                log_this(SR_AUTH,
                         "OIDC RP roles: database query failed for account_id=%d; "
                         "keeping empty roles (non-fatal)",
                         LOG_LEVEL_ALERT, 1, account->id);
                return;
            }
            break;
        }

        case OIDC_RP_ROLE_SRC_IDP_REALM_ROLES:
        /* IDP_CLIENT_ROLES falls back to IDP_REALM_ROLES in Phase 22
         * because OidcRpIdTokenClaims does not yet have a separate
         * client_roles[] array. See header documentation. */
        case OIDC_RP_ROLE_SRC_IDP_CLIENT_ROLES: {
            if (source == OIDC_RP_ROLE_SRC_IDP_CLIENT_ROLES) {
                log_this(SR_AUTH,
                         "OIDC RP roles: IDP_CLIENT_ROLES not yet distinct from "
                         "IDP_REALM_ROLES (Phase 22 fallback); using realm_access.roles",
                         LOG_LEVEL_DEBUG, 0);
            }
            new_roles = roles_from_idp(claims, prefix);
            if (!new_roles) {
                log_this(SR_AUTH,
                         "OIDC RP roles: OOM building IdP role list for account_id=%d; "
                         "keeping empty roles (non-fatal)",
                         LOG_LEVEL_ALERT, 1, account->id);
                return;
            }
            break;
        }

        case OIDC_RP_ROLE_SRC_MERGE: {
            char *db_roles  = auth_roles_from_database(account->id, database);
            char *idp_roles = roles_from_idp(claims, prefix);

            if (!db_roles || !idp_roles) {
                /* Non-fatal: fall back to whichever half succeeded. */
                if (db_roles && !idp_roles) {
                    log_this(SR_AUTH,
                             "OIDC RP roles: OOM building IdP half of MERGE for "
                             "account_id=%d; using DB roles only",
                             LOG_LEVEL_ALERT, 1, account->id);
                    new_roles = db_roles;
                    free(idp_roles);
                } else if (!db_roles && idp_roles) {
                    log_this(SR_AUTH,
                             "OIDC RP roles: DB half of MERGE failed for "
                             "account_id=%d; using IdP roles only",
                             LOG_LEVEL_ALERT, 1, account->id);
                    new_roles = idp_roles;
                } else {
                    /* Both failed. */
                    log_this(SR_AUTH,
                             "OIDC RP roles: both halves of MERGE failed for "
                             "account_id=%d; keeping empty roles (non-fatal)",
                             LOG_LEVEL_ALERT, 1, account->id);
                    return;
                }
                break;
            }

            new_roles = roles_merge(db_roles, idp_roles);
            free(db_roles);
            free(idp_roles);

            if (!new_roles) {
                log_this(SR_AUTH,
                         "OIDC RP roles: OOM merging roles for account_id=%d; "
                         "keeping empty roles (non-fatal)",
                         LOG_LEVEL_ALERT, 1, account->id);
                return;
            }
            break;
        }

        default:
            /* Unknown source enum — treat as DATABASE to be safe. */
            log_this(SR_AUTH,
                     "OIDC RP roles: unknown RoleMapping.Source=%d for "
                     "account_id=%d; defaulting to database",
                     LOG_LEVEL_ALERT, 2, (int)source, account->id);
            new_roles = auth_roles_from_database(account->id, database);
            if (!new_roles) return;
            break;
    }

    /* Count roles for logging. */
    size_t role_count = 0;
    if (new_roles && *new_roles) {
        role_count = 1;
        for (const char *p = new_roles; *p; p++) {
            if (*p == ',') role_count++;
        }
    }

    log_this(SR_AUTH,
             "OIDC RP roles: account_id=%d source=%s roles=%zu",
             LOG_LEVEL_DEBUG, 3,
             account->id,
             oidc_rp_role_source_name(source),
             role_count);

    /* Replace account->roles with the resolved list. */
    free(account->roles);
    account->roles = new_roles;
}
