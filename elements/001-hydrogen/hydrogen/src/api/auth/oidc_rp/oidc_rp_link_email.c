/*
 * OIDC RP account linker — `match_email_only` strategy (Phase 19).
 *
 * Strategy:
 *   1. QueryRef #080 — fast path for already-linked users.
 *   2. On sub-miss: if RequireEmailVerified and email_verified is false,
 *      return NO_ACCOUNT.
 *   3. If no email in claims, return NO_ACCOUNT.
 *   4. QueryRef #082 — look up by email in account_contacts.
 *      Zero rows  → NO_ACCOUNT.
 *      Two+ rows  → EMAIL_AMBIGUOUS.
 *      One row    → call #081 to link, #084 to touch, return OK.
 *
 * UNIQUE race handling: if #081 succeeds but returns no identity_id
 * (because a concurrent sign-in already inserted the row), re-fetch via
 * #080 to get the real identity_id for the touch. This avoids blocking
 * the user on a concurrent first-login collision.
 *
 * Extracted from the original oidc_rp_link.c monolith in Phase 21.
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "oidc_rp_link_email.h"
#include "oidc_rp_link_internal.h"

OidcRpLinkResult oidc_rp_link_match_email_only(const OIDCRPProviderConfig *provider,
                                                const OidcRpIdTokenClaims *claims,
                                                const char *database,
                                                account_info_t **out_account) {
    char iss_log[OIDC_RP_LINK_ISS_LOG_LEN + 1];
    char sub_log[OIDC_RP_LINK_SUB_LOG_LEN + 1];
    oidc_rp_link_safe_iss(claims->iss, iss_log);
    oidc_rp_link_safe_sub(claims->sub, sub_log);

    /* Step 1: fast path — already-linked users avoid the email path. */
    int  identity_id = -1;
    int  account_id  = -1;
    bool db_error    = false;

    oidc_rp_link_query_080_lookup(claims->iss, claims->sub,
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
        oidc_rp_link_query_084_touch(identity_id, claims->email,
                                     claims->email_verified, database);
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_ONLY: sub-hit account_id=%d for iss=%s sub=%s...",
                 LOG_LEVEL_DEBUG, 3, account_id, iss_log, sub_log);
        account_info_t *account = oidc_rp_link_build_account_info(account_id, claims);
        if (!account) return OIDC_RP_LINK_DB_ERROR;
        *out_account = account;
        return OIDC_RP_LINK_OK;
    }

    /* Step 2: sub-miss — check email requirements. */
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
    int row_count = oidc_rp_link_query_082_lookup_by_email(claims->email,
                                                            database,
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
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_ONLY: email_ambiguous (multiple accounts) iss=%s sub=%s...",
                 LOG_LEVEL_ALERT, 2, iss_log, sub_log);
        return OIDC_RP_LINK_EMAIL_AMBIGUOUS;
    }

    /* Step 4: single hit — link via #081. */
    int new_identity_id = -1;
    bool link_ok = oidc_rp_link_query_081_link_identity(email_account_id,
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

    /* If #081 hit a UNIQUE race (new_identity_id == -1), re-fetch via #080
     * to get the real identity_id. Use separate temporaries so we do NOT
     * overwrite email_account_id (Phase 19 lesson #3). */
    if (new_identity_id < 0) {
        int refetch_identity_id = -1;
        int refetch_account_id  = -1;
        db_error = false;
        oidc_rp_link_query_080_lookup(claims->iss, claims->sub, database,
                                      &refetch_identity_id,
                                      &refetch_account_id,
                                      &db_error);
        if (db_error || refetch_identity_id < 0) {
            log_this(SR_AUTH,
                     "OIDC RP linker MATCH_EMAIL_ONLY: post-race #080 re-fetch miss, skipping #084 touch",
                     LOG_LEVEL_ALERT, 0);
        } else {
            new_identity_id = refetch_identity_id;
            if (refetch_account_id >= 0) {
                email_account_id = refetch_account_id;
            }
        }
    }

    if (new_identity_id >= 0) {
        oidc_rp_link_query_084_touch(new_identity_id, claims->email,
                                     claims->email_verified, database);
    }

    log_this(SR_AUTH,
             "OIDC RP linker MATCH_EMAIL_ONLY: linked account_id=%d for iss=%s sub=%s...",
             LOG_LEVEL_DEBUG, 3, email_account_id, iss_log, sub_log);

    account_info_t *account = oidc_rp_link_build_account_info(email_account_id, claims);
    if (!account) return OIDC_RP_LINK_DB_ERROR;
    *out_account = account;
    return OIDC_RP_LINK_OK;
}
