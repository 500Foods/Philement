/*
 * OIDC RP account linker — shared internal helpers.
 *
 * Implements the shared infrastructure used by all four per-strategy
 * linker modules (sub, email, provision, default):
 *
 *   - Test seam (process-global OidcRpLinkQueryFn override).
 *   - oidc_rp_link_run_query: routes through seam or execute_auth_query.
 *   - oidc_rp_link_safe_iss / _safe_sub: truncated-identifier helpers.
 *   - oidc_rp_link_build_account_info: allocate + populate account_info_t.
 *   - QueryRef wrappers #080–#084 (build params, run, parse results).
 *
 * See oidc_rp_link_internal.h for the full contract of each function.
 *
 * Logging policy: NEVER log `email` on the success path. Log `iss`
 * (truncated to OIDC_RP_LINK_ISS_LOG_LEN chars) and `sub` (truncated
 * to OIDC_RP_LINK_SUB_LOG_LEN chars) on every QueryRef error. Use
 * `SR_AUTH` with `OIDC_RP` prefix throughout.
 *
 * Sensitive-buffer scrubbing: form-body strings and credential copies
 * that contain user PII or authentication material are zero-wiped
 * before free (volatile-pointer dance to defeat -O2 dead-store
 * elimination), per the discipline established in Phases 7–14.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_database.h>
#include <jansson.h>

// Local includes
#include "oidc_rp_link_internal.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* -------------------------------------------------------------------------
 * Test seam
 * -------------------------------------------------------------------------
 * Process-global; NULL in production. Installed by
 * oidc_rp_link_test_set_query_fn (defined in oidc_rp_link.c where the
 * public seam API lives), read here.
 *
 * The extern declaration below is the bridge: oidc_rp_link.c owns
 * the storage; oidc_rp_link_internal.c and the strategy modules use
 * it via this declaration.
 */
extern OidcRpLinkQueryFn g_oidc_rp_link_query_fn;

QueryResult *oidc_rp_link_run_query(int query_ref,
                                    const char *database,
                                    json_t *params) {
    if (g_oidc_rp_link_query_fn) {
        return g_oidc_rp_link_query_fn(query_ref, database, params);
    }
    return execute_auth_query(query_ref, database, params);
}

/* -------------------------------------------------------------------------
 * Logging helpers
 * -------------------------------------------------------------------------
 */
void oidc_rp_link_safe_iss(const char *iss,
                            char out[OIDC_RP_LINK_ISS_LOG_LEN + 1]) {
    if (!iss) {
        snprintf(out, OIDC_RP_LINK_ISS_LOG_LEN + 1, "(null)");
        return;
    }
    size_t n = strlen(iss);
    if (n > OIDC_RP_LINK_ISS_LOG_LEN) n = OIDC_RP_LINK_ISS_LOG_LEN;
    memcpy(out, iss, n);
    out[n] = '\0';
}

void oidc_rp_link_safe_sub(const char *sub,
                            char out[OIDC_RP_LINK_SUB_LOG_LEN + 1]) {
    if (!sub) {
        snprintf(out, OIDC_RP_LINK_SUB_LOG_LEN + 1, "(null)");
        return;
    }
    size_t n = strlen(sub);
    if (n > OIDC_RP_LINK_SUB_LOG_LEN) n = OIDC_RP_LINK_SUB_LOG_LEN;
    memcpy(out, sub, n);
    out[n] = '\0';
}

/* -------------------------------------------------------------------------
 * Account info builder
 * -------------------------------------------------------------------------
 */
account_info_t *oidc_rp_link_build_account_info(int account_id,
                                                  const OidcRpIdTokenClaims *claims) {
    account_info_t *account = calloc(1, sizeof(account_info_t));
    if (!account) {
        return NULL;
    }

    account->id         = account_id;
    account->enabled    = true;
    account->authorized = true;

    const char *display_name = (claims->preferred_username && *claims->preferred_username)
                               ? claims->preferred_username
                               : claims->sub;
    account->username = display_name ? strdup(display_name) : strdup("");

    if (claims->email && *claims->email) {
        account->email = strdup(claims->email);
    }

    /* Roles are always empty at this stage; Phase 22's role-mapping
     * module will replace this with the authoritative DB join. */
    account->roles = strdup("");

    if (!account->username || !account->roles) {
        free_account_info(account);
        return NULL;
    }

    return account;
}

/* -------------------------------------------------------------------------
 * QueryRef #080 — lookup OIDC identity by (issuer, subject)
 * -------------------------------------------------------------------------
 */
void oidc_rp_link_query_080_lookup(const char *issuer,
                                   const char *subject,
                                   const char *database,
                                   int *out_identity_id,
                                   int *out_account_id,
                                   bool *out_db_error) {
    *out_identity_id = -1;
    *out_account_id  = -1;
    *out_db_error    = false;

    json_t *params        = json_object();
    json_t *string_params = json_object();
    json_object_set_new(string_params, "ISSUER",  json_string(issuer));
    json_object_set_new(string_params, "SUBJECT", json_string(subject));
    json_object_set_new(params, "STRING", string_params);

    QueryResult *result = oidc_rp_link_run_query(80, database, params);
    json_decref(params);

    if (!result) {
        *out_db_error = true;
        return;
    }
    if (!result->success) {
        log_this(SR_AUTH,
                 "OIDC RP linker: QueryRef #080 failed: %s",
                 LOG_LEVEL_ERROR, 1,
                 result->error_message ? result->error_message : "(unknown)");
        free_query_result(result);
        *out_db_error = true;
        return;
    }

    if (!result->data_json) {
        /* Successful but no rows — clean miss. */
        free_query_result(result);
        return;
    }

    json_t *arr = json_loads(result->data_json, 0, NULL);
    free_query_result(result);

    if (!arr || !json_is_array(arr)) {
        json_decref(arr);
        *out_db_error = true;
        return;
    }

    json_t *row = json_array_get(arr, 0);
    if (!row) {
        /* Zero rows — clean miss. */
        json_decref(arr);
        return;
    }

    /* Column names may be uppercase on DB2 — fall back gracefully. */
    json_t *id_json  = json_object_get(row, "identity_id");
    if (!id_json)    id_json  = json_object_get(row, "IDENTITY_ID");
    json_t *acc_json = json_object_get(row, "account_id");
    if (!acc_json)   acc_json = json_object_get(row, "ACCOUNT_ID");

    if (id_json && json_is_integer(id_json)) {
        *out_identity_id = (int)json_integer_value(id_json);
    }
    if (acc_json && json_is_integer(acc_json)) {
        *out_account_id = (int)json_integer_value(acc_json);
    }

    json_decref(arr);
}

/* -------------------------------------------------------------------------
 * QueryRef #081 — link a new identity row to an existing account
 * -------------------------------------------------------------------------
 */
bool oidc_rp_link_query_081_link_identity(int account_id,
                                          const char *issuer,
                                          const char *subject,
                                          const char *email,
                                          bool email_verified,
                                          const char *database,
                                          int *out_identity_id) {
    *out_identity_id = -1;

    json_t *params          = json_object();
    json_t *integer_params  = json_object();
    json_t *string_params   = json_object();

    json_object_set_new(integer_params, "ACCOUNTID",     json_integer(account_id));
    json_object_set_new(integer_params, "EMAILVERIFIED", json_integer(email_verified ? 1 : 0));
    json_object_set_new(params, "INTEGER", integer_params);

    json_object_set_new(string_params, "ISSUER",  json_string(issuer));
    json_object_set_new(string_params, "SUBJECT", json_string(subject));
    if (email && *email) {
        json_object_set_new(string_params, "EMAIL", json_string(email));
    } else {
        json_object_set_new(string_params, "EMAIL", json_null());
    }
    json_object_set_new(params, "STRING", string_params);

    QueryResult *result = oidc_rp_link_run_query(81, database, params);
    json_decref(params);

    if (!result) {
        return false;
    }
    if (!result->success) {
        /* A UNIQUE constraint violation is reported as failure by the DB
         * layer. Return true (the row already exists) so the caller can
         * retry with #080. We cannot reliably distinguish UNIQUE violations
         * from other errors at this level without string-matching the error
         * message, so we accept false positives. */
        log_this(SR_AUTH,
                 "OIDC RP linker: QueryRef #081 link failed (may be UNIQUE conflict, retry via #080): %s",
                 LOG_LEVEL_DEBUG, 1,
                 result->error_message ? result->error_message : "(unknown)");
        free_query_result(result);
        return true; /* caller retries with #080 */
    }

    /* Extract the new identity_id from the RETURNING clause. */
    if (result->data_json) {
        json_t *arr = json_loads(result->data_json, 0, NULL);
        if (arr && json_is_array(arr)) {
            json_t *row = json_array_get(arr, 0);
            if (row) {
                json_t *id_json = json_object_get(row, "identity_id");
                if (!id_json) id_json = json_object_get(row, "IDENTITY_ID");
                if (id_json && json_is_integer(id_json)) {
                    *out_identity_id = (int)json_integer_value(id_json);
                }
            }
        }
        json_decref(arr);
    }

    free_query_result(result);
    return true;
}

/* -------------------------------------------------------------------------
 * QueryRef #082 — lookup account_id(s) by email
 * -------------------------------------------------------------------------
 */
int oidc_rp_link_query_082_lookup_by_email(const char *email,
                                           const char *database,
                                           int *out_account_id) {
    *out_account_id = -1;

    json_t *params        = json_object();
    json_t *string_params = json_object();
    json_object_set_new(string_params, "EMAIL", json_string(email));
    json_object_set_new(params, "STRING", string_params);

    QueryResult *result = oidc_rp_link_run_query(82, database, params);
    json_decref(params);

    if (!result) {
        return -1; /* transport failure */
    }
    if (!result->success) {
        log_this(SR_AUTH,
                 "OIDC RP linker: QueryRef #082 failed: %s",
                 LOG_LEVEL_ERROR, 1,
                 result->error_message ? result->error_message : "(unknown)");
        free_query_result(result);
        return -1;
    }

    if (!result->data_json) {
        free_query_result(result);
        return 0;
    }

    json_t *arr = json_loads(result->data_json, 0, NULL);
    free_query_result(result);

    if (!arr || !json_is_array(arr)) {
        json_decref(arr);
        return -1;
    }

    size_t row_count = json_array_size(arr);

    if (row_count == 1) {
        json_t *row      = json_array_get(arr, 0);
        json_t *acc_json = json_object_get(row, "account_id");
        if (!acc_json) acc_json = json_object_get(row, "ACCOUNT_ID");
        if (acc_json && json_is_integer(acc_json)) {
            *out_account_id = (int)json_integer_value(acc_json);
        }
    }

    json_decref(arr);

    /* Cap at 2: caller only needs 0 (miss), 1 (unambiguous), 2+ (ambiguous). */
    return (row_count > 2) ? 2 : (int)row_count;
}

/* -------------------------------------------------------------------------
 * QueryRef #083 — provision a new accounts row
 * -------------------------------------------------------------------------
 */
int oidc_rp_link_query_083_provision(const OidcRpIdTokenClaims *claims,
                                     const char *database) {
    json_t *params        = json_object();
    json_t *string_params = json_object();

    /* USERNAME: preferred_username → email local-part → sub. */
    const char *username = NULL;
    char fallback_buf[256];

    if (claims->preferred_username && *claims->preferred_username) {
        username = claims->preferred_username;
    } else if (claims->email && *claims->email) {
        const char *at = strchr(claims->email, '@');
        if (at && at != claims->email) {
            size_t local_len = (size_t)(at - claims->email);
            if (local_len >= sizeof(fallback_buf)) {
                local_len = sizeof(fallback_buf) - 1;
            }
            memcpy(fallback_buf, claims->email, local_len);
            fallback_buf[local_len] = '\0';
            username = fallback_buf;
        }
    }
    if (!username || !*username) username = claims->sub;

    json_object_set_new(string_params, "USERNAME",   json_string(username));
    /* given_name / family_name not in Phase 12 claims struct; pass empty
     * strings. Phase 22 may extend OidcRpIdTokenClaims for these. */
    json_object_set_new(string_params, "FIRST_NAME", json_string(""));
    json_object_set_new(string_params, "LAST_NAME",  json_string(""));
    json_object_set_new(string_params, "SUMMARY",    json_string(""));
    json_object_set_new(params, "STRING", string_params);

    QueryResult *result = oidc_rp_link_run_query(83, database, params);
    json_decref(params);

    if (!result) {
        log_this(SR_AUTH,
                 "OIDC RP linker: QueryRef #083 transport failure (NULL result)",
                 LOG_LEVEL_ERROR, 0);
        return -1;
    }
    if (!result->success) {
        log_this(SR_AUTH,
                 "OIDC RP linker: QueryRef #083 failed: %s",
                 LOG_LEVEL_ERROR, 1,
                 result->error_message ? result->error_message : "(unknown)");
        free_query_result(result);
        return -1;
    }

    int account_id = -1;
    if (result->data_json) {
        json_t *arr = json_loads(result->data_json, 0, NULL);
        if (arr && json_is_array(arr)) {
            json_t *row = json_array_get(arr, 0);
            if (row) {
                json_t *acc_json = json_object_get(row, "account_id");
                if (!acc_json) acc_json = json_object_get(row, "ACCOUNT_ID");
                if (acc_json && json_is_integer(acc_json)) {
                    account_id = (int)json_integer_value(acc_json);
                }
            }
        }
        json_decref(arr);
    }

    free_query_result(result);
    return account_id;
}

/* -------------------------------------------------------------------------
 * QueryRef #084 — touch last_seen_at, email, email_verified
 * -------------------------------------------------------------------------
 */
void oidc_rp_link_query_084_touch(int identity_id,
                                  const char *email,
                                  bool email_verified,
                                  const char *database) {
    json_t *params         = json_object();
    json_t *integer_params = json_object();
    json_t *string_params  = json_object();

    json_object_set_new(integer_params, "IDENTITYID",    json_integer(identity_id));
    json_object_set_new(integer_params, "EMAILVERIFIED", json_integer(email_verified ? 1 : 0));
    json_object_set_new(params, "INTEGER", integer_params);

    if (email && *email) {
        json_object_set_new(string_params, "EMAIL", json_string(email));
    } else {
        json_object_set_new(string_params, "EMAIL", json_null());
    }
    json_object_set_new(params, "STRING", string_params);

    QueryResult *result = oidc_rp_link_run_query(84, database, params);
    json_decref(params);

    if (!result || !result->success) {
        log_this(SR_AUTH,
                 "OIDC RP linker: QueryRef #084 touch failed for identity_id=%d (non-fatal)",
                 LOG_LEVEL_ALERT, 1, identity_id);
    }
    if (result) {
        free_query_result(result);
    }
}
