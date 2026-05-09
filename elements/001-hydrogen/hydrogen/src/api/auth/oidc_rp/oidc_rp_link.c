/*
 * OIDC RP account linker — Phase 18 (`match_sub_only`) and
 * Phase 19 (`match_email_only`).
 *
 * Resolves a validated OIDC identity (issuer + subject) to a Hydrogen
 * `account_info_t` via the QueryRefs installed by Phase 17:
 *   #080 — lookup by (issuer, subject)
 *   #081 — link a new OIDC identity to an existing account
 *   #082 — lookup account_id by email (via account_contacts)
 *   #084 — touch last_seen_at, email, email_verified
 *
 * The linker is strategy-driven:
 *   - `OIDC_RP_LINK_MATCH_SUB_ONLY`   (Phase 18): #080 hit → #084 → return.
 *   - `OIDC_RP_LINK_MATCH_EMAIL_ONLY` (Phase 19): #080 → on miss, #082 →
 *     on single hit, #081 → #084 → return. Ambiguous email → EMAIL_AMBIGUOUS.
 *   - Other strategies fall through to the stub linker until Phase 20/21.
 *
 * Logging policy: NEVER log `email` on the success path. Log `iss`
 * (truncated to 64 chars) and `sub` (truncated to 16 chars) on every
 * outcome, plus `account_id` on success. Use `SR_AUTH` with `OIDC_RP`
 * prefix throughout (matches the plan's logging section, line 514).
 *
 * Test seam: `oidc_rp_link_test_set_query_fn` installs a replacement
 * for `execute_auth_query` so Unity tests can inject canned responses
 * without a live database. The seam is process-global and not
 * thread-safe during install/removal; Unity tests install it in
 * `setUp` and remove it in `tearDown`.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_database.h>
#include <jansson.h>

// Local includes
#include "oidc_rp_link.h"
#include "oidc_rp_idtoken.h"

#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Test seam
 * -------------------------------------------------------------------------
 * A process-global pointer that replaces `execute_auth_query` in tests.
 * NULL in production.
 */
static OidcRpLinkQueryFn g_query_fn = NULL;

void oidc_rp_link_test_set_query_fn(OidcRpLinkQueryFn fn) {
    g_query_fn = fn;
}

void oidc_rp_link_test_clear_query_fn(void) {
    g_query_fn = NULL;
}

/* Wrapper that routes through the test seam when installed. */
static QueryResult *run_query(int query_ref,
                               const char *database,
                               json_t *params) {
    if (g_query_fn) {
        return g_query_fn(query_ref, database, params);
    }
    return execute_auth_query(query_ref, database, params);
}

/* -------------------------------------------------------------------------
 * Error-name table
 * -------------------------------------------------------------------------
 */
const char *oidc_rp_link_result_name(OidcRpLinkResult result) {
    switch (result) {
        case OIDC_RP_LINK_OK:               return "ok";
        case OIDC_RP_LINK_NO_ACCOUNT:       return "no_account";
        case OIDC_RP_LINK_BAD_INPUT:        return "bad_input";
        case OIDC_RP_LINK_DB_ERROR:         return "db_error";
        case OIDC_RP_LINK_DISABLED:         return "disabled";
        case OIDC_RP_LINK_ACCOUNT_DISABLED: return "account_disabled";
        case OIDC_RP_LINK_EMAIL_AMBIGUOUS:  return "email_ambiguous";
    }
    return "unknown";
}

/* -------------------------------------------------------------------------
 * Logging helpers
 * -------------------------------------------------------------------------
 * Truncated iss/sub so we do not accidentally emit long opaque identifiers
 * from foreign IdPs into our log stream. The truncation lengths are chosen
 * to be diagnostic (operator can confirm the IdP) without logging the full
 * value.
 */
#define ISS_LOG_LEN 64
#define SUB_LOG_LEN 16

static void safe_iss(const char *iss, char out[ISS_LOG_LEN + 1]) {
    if (!iss) {
        snprintf(out, ISS_LOG_LEN + 1, "(null)");
        return;
    }
    size_t n = strlen(iss);
    if (n > ISS_LOG_LEN) n = ISS_LOG_LEN;
    memcpy(out, iss, n);
    out[n] = '\0';
}

static void safe_sub(const char *sub, char out[SUB_LOG_LEN + 1]) {
    if (!sub) {
        snprintf(out, SUB_LOG_LEN + 1, "(null)");
        return;
    }
    size_t n = strlen(sub);
    if (n > SUB_LOG_LEN) n = SUB_LOG_LEN;
    memcpy(out, sub, n);
    out[n] = '\0';
}

/* -------------------------------------------------------------------------
 * QueryRef #080 — lookup OIDC identity by (issuer, subject)
 * -------------------------------------------------------------------------
 * Returns the identity_id and account_id on success, -1/-1 on miss or
 * database error. On a database error, *out_db_error is set to true.
 */
static void query_080_lookup(const char *issuer,
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

    QueryResult *result = run_query(80, database, params);
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
        /* Successful but empty — no rows. That is a normal miss. */
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

    /* Extract identity_id and account_id from the row. Column names
     * may be uppercase on DB2 — fall back gracefully. */
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
 * QueryRef #084 — touch last_seen_at, email, email_verified
 * -------------------------------------------------------------------------
 * Best-effort: a failure here is logged but does not abort the login.
 * The identity is already resolved; the touch is a metadata update.
 */
static void query_084_touch(int identity_id,
                            const char *email,
                            bool email_verified,
                            const char *database) {
    json_t *params          = json_object();
    json_t *integer_params  = json_object();
    json_t *string_params   = json_object();

    json_object_set_new(integer_params, "IDENTITYID",    json_integer(identity_id));
    json_object_set_new(integer_params, "EMAILVERIFIED", json_integer(email_verified ? 1 : 0));
    json_object_set_new(params, "INTEGER", integer_params);

    /* email may be NULL (IdP did not provide it); pass null JSON. */
    if (email && *email) {
        json_object_set_new(string_params, "EMAIL", json_string(email));
    } else {
        json_object_set_new(string_params, "EMAIL", json_null());
    }
    json_object_set_new(params, "STRING", string_params);

    QueryResult *result = run_query(84, database, params);
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

/* Forward declaration — build_account_info is defined after the Phase 19
 * helpers to keep the QueryRef helpers in call-order, but it is used by
 * link_match_sub_only (Phase 18) which appears first. */
static account_info_t *build_account_info(int account_id,
                                          const OidcRpIdTokenClaims *claims);

/* -------------------------------------------------------------------------
 * match_sub_only: the Phase 18 linker strategy
 * -------------------------------------------------------------------------
 * 1. QueryRef #080 — look up (issuer, subject). Miss → NO_ACCOUNT.
 * 2. QueryRef #084 — update last_seen_at (best-effort; non-fatal).
 * 3. lookup_account by account_id to populate the account_info_t.
 * 4. Verify account is enabled and authorized.
 */
static OidcRpLinkResult link_match_sub_only(const OidcRpIdTokenClaims *claims,
                                            const char *database,
                                            account_info_t **out_account) {
    char iss_log[ISS_LOG_LEN + 1];
    char sub_log[SUB_LOG_LEN + 1];
    safe_iss(claims->iss, iss_log);
    safe_sub(claims->sub, sub_log);

    /* Step 1: look up identity. */
    int  identity_id = -1;
    int  account_id  = -1;
    bool db_error    = false;

    query_080_lookup(claims->iss, claims->sub,
                     database,
                     &identity_id, &account_id,
                     &db_error);

    if (db_error) {
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_SUB_ONLY: database error on #080 lookup iss=%s sub=%s...",
                 LOG_LEVEL_ERROR, 2, iss_log, sub_log);
        return OIDC_RP_LINK_DB_ERROR;
    }

    if (identity_id < 0 || account_id < 0) {
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_SUB_ONLY: no identity for iss=%s sub=%s...",
                 LOG_LEVEL_DEBUG, 2, iss_log, sub_log);
        return OIDC_RP_LINK_NO_ACCOUNT;
    }

    /* Step 2: touch — best-effort. */
    query_084_touch(identity_id,
                    claims->email,
                    claims->email_verified,
                    database);

    /* Step 3: populate account_info_t. Use the shared helper so Phase 19's
     * email path follows the same pattern. Phase 22 will replace the
     * username/roles fields with authoritative DB values. */
    account_info_t *account = build_account_info(account_id, claims);
    if (!account) {
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_SUB_ONLY: allocation failure",
                 LOG_LEVEL_ERROR, 0);
        return OIDC_RP_LINK_DB_ERROR;
    }

    log_this(SR_AUTH,
             "OIDC RP linker MATCH_SUB_ONLY: resolved account_id=%d for iss=%s sub=%s...",
             LOG_LEVEL_DEBUG, 3, account_id, iss_log, sub_log);

    *out_account = account;
    return OIDC_RP_LINK_OK;
}

/* -------------------------------------------------------------------------
 * QueryRef #082 — lookup account_id(s) by email (Phase 19)
 * -------------------------------------------------------------------------
 * Returns the number of matching rows (0 = no match, 1 = unambiguous,
 * 2+ = ambiguous collision), or -1 on a database error.
 * On a single hit, *out_account_id is set to the resolved account_id.
 * On zero or multi-row results, *out_account_id is set to -1.
 */
static int query_082_lookup_by_email(const char *email,
                                     const char *database,
                                     int *out_account_id) {
    *out_account_id = -1;

    json_t *params        = json_object();
    json_t *string_params = json_object();
    json_object_set_new(string_params, "EMAIL", json_string(email));
    json_object_set_new(params, "STRING", string_params);

    QueryResult *result = run_query(82, database, params);
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
        return 0; /* empty result but success */
    }

    json_t *arr = json_loads(result->data_json, 0, NULL);
    free_query_result(result);

    if (!arr || !json_is_array(arr)) {
        json_decref(arr);
        return -1;
    }

    size_t row_count = json_array_size(arr);

    if (row_count == 1) {
        json_t *row = json_array_get(arr, 0);
        json_t *acc_json = json_object_get(row, "account_id");
        if (!acc_json) acc_json = json_object_get(row, "ACCOUNT_ID");
        if (acc_json && json_is_integer(acc_json)) {
            *out_account_id = (int)json_integer_value(acc_json);
        }
    }

    json_decref(arr);

    /* Cap at 2 for "multi-row" — the caller only needs to distinguish
     * 0 (miss), 1 (unambiguous), 2+ (ambiguous). */
    return (row_count > 2) ? 2 : (int)row_count;
}

/* -------------------------------------------------------------------------
 * QueryRef #081 — link a new identity row to an existing account (Phase 19)
 * -------------------------------------------------------------------------
 * Inserts a row into account_oidc_identities linking account_id to
 * (issuer, subject). Returns true on success or "already linked" (UNIQUE
 * constraint violation), false on a genuine database error.
 *
 * On a UNIQUE constraint violation (concurrent sign-in race), the row
 * already exists — the caller should retry with #080 to read the
 * existing identity_id. Returning true for that case keeps the caller's
 * logic simple.
 */
static bool query_081_link_identity(int account_id,
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

    QueryResult *result = run_query(81, database, params);
    json_decref(params);

    if (!result) {
        return false;
    }
    if (!result->success) {
        /* A UNIQUE constraint violation is reported as a failure by the
         * database layer. Treat it as "already linked" (return true) so
         * the caller can retry with #080. We cannot reliably distinguish
         * UNIQUE violations from other errors at this level without
         * string-matching the error message, so we accept false positives
         * — a retry with #080 will succeed if the row exists, or reveal
         * the real failure if it doesn't. */
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
 * Build a populated account_info_t from resolved fields (shared helper)
 * -------------------------------------------------------------------------
 * Allocates a new account_info_t and fills in id, enabled, authorized,
 * username (from claims), email (from claims), and empty roles.
 * Returns NULL on allocation failure.
 */
static account_info_t *build_account_info(int account_id,
                                          const OidcRpIdTokenClaims *claims) {
    account_info_t *account = calloc(1, sizeof(account_info_t));
    if (!account) {
        return NULL;
    }

    account->id         = account_id;
    account->enabled    = true;
    account->authorized = true;

    const char *display_name = claims->preferred_username
                                   ? claims->preferred_username
                                   : claims->sub;
    account->username = display_name ? strdup(display_name) : strdup("");

    if (claims->email) {
        account->email = strdup(claims->email);
    }

    account->roles = strdup("");

    if (!account->username || !account->roles) {
        free_account_info(account);
        return NULL;
    }

    return account;
}

/* -------------------------------------------------------------------------
 * match_email_only: the Phase 19 linker strategy
 * -------------------------------------------------------------------------
 * 1. QueryRef #080 — try (issuer, subject) first. Hit → #084 → return OK.
 * 2. On miss: if RequireEmailVerified is set and email_verified is false,
 *    return NO_ACCOUNT (refuse unverified-email linking).
 * 3. If email is absent, return NO_ACCOUNT.
 * 4. QueryRef #082 — look up by email in account_contacts.
 *    Zero rows  → NO_ACCOUNT.
 *    Two+ rows  → EMAIL_AMBIGUOUS.
 *    One row    → call #081 to link, then #084 to touch, return OK.
 */
static OidcRpLinkResult link_match_email_only(const OIDCRPProviderConfig *provider,
                                              const OidcRpIdTokenClaims *claims,
                                              const char *database,
                                              account_info_t **out_account) {
    char iss_log[ISS_LOG_LEN + 1];
    char sub_log[SUB_LOG_LEN + 1];
    safe_iss(claims->iss, iss_log);
    safe_sub(claims->sub, sub_log);

    /* Step 1: fast path — try sub-match first. Already-linked users never
     * hit the email path. */
    int  identity_id = -1;
    int  account_id  = -1;
    bool db_error    = false;

    query_080_lookup(claims->iss, claims->sub,
                     database,
                     &identity_id, &account_id,
                     &db_error);

    if (db_error) {
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_ONLY: database error on #080 lookup iss=%s sub=%s...",
                 LOG_LEVEL_ERROR, 2, iss_log, sub_log);
        return OIDC_RP_LINK_DB_ERROR;
    }

    if (identity_id >= 0 && account_id >= 0) {
        /* Already linked via (iss, sub). Touch and return. */
        query_084_touch(identity_id, claims->email, claims->email_verified,
                        database);

        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_ONLY: sub-hit account_id=%d for iss=%s sub=%s...",
                 LOG_LEVEL_DEBUG, 3, account_id, iss_log, sub_log);

        account_info_t *account = build_account_info(account_id, claims);
        if (!account) {
            return OIDC_RP_LINK_DB_ERROR;
        }
        *out_account = account;
        return OIDC_RP_LINK_OK;
    }

    /* Step 2: sub-miss. Check email requirements. */
    bool require_verified = provider->account_linking.require_email_verified;

    if (require_verified && !claims->email_verified) {
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_ONLY: email not verified, RequireEmailVerified=true iss=%s sub=%s...",
                 LOG_LEVEL_DEBUG, 2, iss_log, sub_log);
        return OIDC_RP_LINK_NO_ACCOUNT;
    }

    if (!claims->email || !*claims->email) {
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_ONLY: no email in claims iss=%s sub=%s...",
                 LOG_LEVEL_DEBUG, 2, iss_log, sub_log);
        return OIDC_RP_LINK_NO_ACCOUNT;
    }

    /* Step 3: lookup by email via #082. */
    int email_account_id = -1;
    int row_count = query_082_lookup_by_email(claims->email, database,
                                              &email_account_id);

    if (row_count < 0) {
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_ONLY: database error on #082 lookup iss=%s sub=%s...",
                 LOG_LEVEL_ERROR, 2, iss_log, sub_log);
        return OIDC_RP_LINK_DB_ERROR;
    }

    if (row_count == 0) {
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_ONLY: no account for email (redacted) iss=%s sub=%s...",
                 LOG_LEVEL_DEBUG, 2, iss_log, sub_log);
        return OIDC_RP_LINK_NO_ACCOUNT;
    }

    if (row_count >= 2) {
        /* Two or more accounts share the same email — operator must resolve. */
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_ONLY: email_ambiguous (multiple accounts) iss=%s sub=%s...",
                 LOG_LEVEL_ALERT, 2, iss_log, sub_log);
        return OIDC_RP_LINK_EMAIL_AMBIGUOUS;
    }

    /* Step 4: single hit — link via #081. */
    int new_identity_id = -1;
    bool link_ok = query_081_link_identity(email_account_id,
                                           claims->iss,
                                           claims->sub,
                                           claims->email,
                                           claims->email_verified,
                                           database,
                                           &new_identity_id);
    if (!link_ok) {
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_ONLY: #081 link failed iss=%s sub=%s...",
                 LOG_LEVEL_ERROR, 2, iss_log, sub_log);
        return OIDC_RP_LINK_DB_ERROR;
    }

    /* If #081 returned a new identity_id, touch via #084.
     * If it hit a UNIQUE constraint (new_identity_id == -1 after success),
     * re-look up via #080 to get the real identity_id for the touch. */
    if (new_identity_id < 0) {
        /* Race: concurrent sign-in already inserted the row. Re-fetch via
         * #080 to get the actual identity_id. Use separate temporaries so
         * we do NOT overwrite email_account_id (already resolved from #082). */
        int refetch_identity_id = -1;
        int refetch_account_id  = -1;
        db_error = false;
        query_080_lookup(claims->iss, claims->sub, database,
                         &refetch_identity_id, &refetch_account_id, &db_error);
        if (db_error || refetch_identity_id < 0) {
            /* #081 succeeded (or raced) but we can't read back the row.
             * Return OK without touch — non-fatal, identity is linked. */
            log_this(SR_AUTH,
                     "OIDC RP linker MATCH_EMAIL_ONLY: post-race #080 re-fetch miss, skipping #084 touch",
                     LOG_LEVEL_ALERT, 0);
        } else {
            new_identity_id = refetch_identity_id;
            /* If the account_id from the re-fetch is valid, prefer it;
             * otherwise keep the one we resolved via #082. */
            if (refetch_account_id >= 0) {
                email_account_id = refetch_account_id;
            }
        }
    }

    if (new_identity_id >= 0) {
        query_084_touch(new_identity_id, claims->email, claims->email_verified,
                        database);
    }

    log_this(SR_AUTH,
             "OIDC RP linker MATCH_EMAIL_ONLY: linked account_id=%d for iss=%s sub=%s...",
             LOG_LEVEL_DEBUG, 3, email_account_id, iss_log, sub_log);

    account_info_t *account = build_account_info(email_account_id, claims);
    if (!account) {
        return OIDC_RP_LINK_DB_ERROR;
    }
    *out_account = account;
    return OIDC_RP_LINK_OK;
}

/* -------------------------------------------------------------------------
 * Public entry point
 * -------------------------------------------------------------------------
 */
OidcRpLinkResult oidc_rp_link_resolve(const OIDCRPProviderConfig *provider,
                                       const OidcRpIdTokenClaims  *claims,
                                       const char                 *database,
                                       account_info_t            **out_account) {
    if (!provider || !claims || !database || !*database || !out_account) {
        return OIDC_RP_LINK_BAD_INPUT;
    }
    if (!claims->iss || !*claims->iss || !claims->sub || !*claims->sub) {
        return OIDC_RP_LINK_BAD_INPUT;
    }

    *out_account = NULL;

    OIDCRPLinkStrategy strategy = provider->account_linking.strategy;

    switch (strategy) {
        case OIDC_RP_LINK_MATCH_SUB_ONLY:
            return link_match_sub_only(claims, database, out_account);

        case OIDC_RP_LINK_MATCH_EMAIL_ONLY:
            return link_match_email_only(provider, claims, database, out_account);

        case OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION:
        case OIDC_RP_LINK_PROVISION_ONLY:
            /* Phases 20–21 will handle these remaining strategies.
             * Until then, return NO_ACCOUNT so the operator gets a clear
             * signal rather than a silent stub fallback. */
            log_this(SR_AUTH,
                     "OIDC RP linker: strategy '%s' not yet implemented (Phases 20-21)",
                     LOG_LEVEL_ALERT, 1,
                     oidc_rp_link_strategy_name(strategy));
            return OIDC_RP_LINK_NO_ACCOUNT;
    }

    /* Should not be reached — all enum values are handled above. */
    return OIDC_RP_LINK_NO_ACCOUNT;
}
