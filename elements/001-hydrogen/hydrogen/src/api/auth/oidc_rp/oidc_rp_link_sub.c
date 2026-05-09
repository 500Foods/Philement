/*
 * OIDC RP account linker — `match_sub_only` strategy (Phase 18).
 *
 * Strategy:
 *   1. QueryRef #080 — look up (issuer, subject). Miss → NO_ACCOUNT.
 *   2. QueryRef #084 — update last_seen_at (best-effort; non-fatal).
 *   3. Build account_info_t from the resolved account_id and claims.
 *
 * The logic is extracted from the original oidc_rp_link.c (Phase 18–20
 * monolith) into this dedicated module so the per-strategy source files
 * stay under the project's 1,000-line cap (Phase 21 refactor).
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "oidc_rp_link_sub.h"
#include "oidc_rp_link_internal.h"

OidcRpLinkResult oidc_rp_link_match_sub_only(const OidcRpIdTokenClaims *claims,
                                              const char *database,
                                              account_info_t **out_account) {
    char iss_log[OIDC_RP_LINK_ISS_LOG_LEN + 1];
    char sub_log[OIDC_RP_LINK_SUB_LOG_LEN + 1];
    oidc_rp_link_safe_iss(claims->iss, iss_log);
    oidc_rp_link_safe_sub(claims->sub, sub_log);

    int  identity_id = -1;
    int  account_id  = -1;
    bool db_error    = false;

    oidc_rp_link_query_080_lookup(claims->iss, claims->sub,
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

    /* Touch — best-effort; non-fatal. */
    oidc_rp_link_query_084_touch(identity_id,
                                 claims->email,
                                 claims->email_verified,
                                 database);

    account_info_t *account = oidc_rp_link_build_account_info(account_id, claims);
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
