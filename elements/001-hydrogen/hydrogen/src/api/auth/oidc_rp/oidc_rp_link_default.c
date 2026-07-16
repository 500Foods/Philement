/*
 * OIDC RP account linker — `match_email_then_provision` strategy (Phase 21).
 *
 * This is the default strategy, composing phases 18–20 in order of preference:
 *
 *   Step 1 — Sub fast-path (Phase 18):
 *     QueryRef #080: look up (issuer, subject). Hit → #084 → return OK.
 *
 *   Step 2 — Email-link path (Phase 19):
 *     On sub-miss, if email is verified (or RequireEmailVerified=false) and
 *     email is present, call QueryRef #082. Single account hit → #081 link
 *     → #084 touch → return OK. Multiple accounts → EMAIL_AMBIGUOUS. Zero
 *     accounts → fall through to step 3.
 *
 *   Step 3 — Provision path (Phase 20):
 *     On email-miss (no matching account), if ProvisionDefaults.Enabled=true
 *     and all prerequisites pass (email present, verified if required, domain
 *     allowed), provision via #083 → link via #081 → #084 touch → return OK.
 *
 *   Step 4 — No match:
 *     If none of the above succeed (or are configured to run), return
 *     NO_ACCOUNT.
 *
 * Key decision points:
 *
 *   - EMAIL_AMBIGUOUS short-circuits: two accounts with the same email is an
 *     admin-resolvable data issue. We do NOT fall through to provisioning in
 *     that case (that would create a third account, making things worse).
 *
 *   - Unverified email: if RequireEmailVerified=true and email_verified is
 *     false, we skip both the email-link step AND the provisioning step
 *     (because email is the key for both). We return NO_ACCOUNT rather than
 *     creating an account we cannot verify.
 *
 *   - Missing email: if the IdP did not provide an email claim, both the
 *     email-link and provisioning steps are skipped (they both require email).
 *     Only the sub-match step can succeed.
 *
 *   - PROVISION_DISALLOWED_EMAIL: this terminates the strategy (we do not
 *     fall through to anything else — there is nothing else to try).
 *
 *   - DB errors at any step terminate early rather than falling through;
 *     partial-state repairs are the operator's responsibility.
 *
 * Default-role assignment is still DEFERRED to Phase 22 (same as Phase 20).
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "oidc_rp_link_default.h"
#include "oidc_rp_link_internal.h"

#include <ctype.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Domain allow-list helpers (duplicated from oidc_rp_link_provision.c
 * because they are file-local helpers — promoting them to internal.h
 * would expose C implementation details in a public header).
 * -------------------------------------------------------------------------
 */

bool domains_equal_ci_def(const char *a, const char *b) {
    if (!a || !b) return false;
    while (*a && *b) {
        char ca = (char)tolower((unsigned char)*a);
        char cb = (char)tolower((unsigned char)*b);
        if (ca != cb) return false;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

const char *email_domain_def(const char *email) {
    if (!email || !*email) return NULL;
    const char *at = strrchr(email, '@');
    if (!at)         return NULL;
    if (at == email) return NULL;
    if (!at[1])      return NULL;
    return at + 1;
}

bool email_domain_allowed_def(const OIDCRPAccountLinking *linking,
                                      const char *email) {
    if (linking->allowed_email_domain_count == 0) return true;
    const char *dom = email_domain_def(email);
    if (!dom) return false;
    for (size_t i = 0; i < linking->allowed_email_domain_count; i++) {
        const char *entry = linking->allowed_email_domains[i];
        if (entry && domains_equal_ci_def(dom, entry)) return true;
    }
    return false;
}

/* -------------------------------------------------------------------------
 * Public strategy entry point
 * -------------------------------------------------------------------------
 */
OidcRpLinkResult oidc_rp_link_match_email_then_provision(
    const OIDCRPProviderConfig *provider,
    const OidcRpIdTokenClaims  *claims,
    const char                 *database,
    account_info_t            **out_account)
{
    char iss_log[OIDC_RP_LINK_ISS_LOG_LEN + 1];
    char sub_log[OIDC_RP_LINK_SUB_LOG_LEN + 1];
    oidc_rp_link_safe_iss(claims->iss, iss_log);
    oidc_rp_link_safe_sub(claims->sub, sub_log);

    const OIDCRPAccountLinking *linking = &provider->account_linking;

    /* ------------------------------------------------------------------ */
    /* Step 1: Sub fast-path — already linked users never hit the slower   */
    /*         email or provision paths.                                    */
    /* ------------------------------------------------------------------ */
    int  identity_id = -1;
    int  account_id  = -1;
    bool db_error    = false;

    oidc_rp_link_query_080_lookup(claims->iss, claims->sub,
                                  database,
                                  &identity_id, &account_id,
                                  &db_error);

    if (db_error) {
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_THEN_PROVISION: database error on #080 "
                 "lookup iss=%s sub=%s...",
                 LOG_LEVEL_ERROR, 2, iss_log, sub_log);
        return OIDC_RP_LINK_DB_ERROR;
    }

    if (identity_id >= 0 && account_id >= 0) {
        oidc_rp_link_query_084_touch(identity_id, claims->email,
                                     claims->email_verified, database);
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_THEN_PROVISION: sub-hit account_id=%d "
                 "for iss=%s sub=%s...",
                 LOG_LEVEL_DEBUG, 3, account_id, iss_log, sub_log);
        account_info_t *account = oidc_rp_link_build_account_info(account_id, claims);
        if (!account) return OIDC_RP_LINK_DB_ERROR;
        *out_account = account;
        return OIDC_RP_LINK_OK;
    }

    /* ------------------------------------------------------------------ */
    /* Check whether email is usable for steps 2 and 3.                   */
    /* ------------------------------------------------------------------ */
    bool email_usable = claims->email && *claims->email;
    bool email_ok_for_link = email_usable &&
        (!linking->require_email_verified || claims->email_verified);

    /* ------------------------------------------------------------------ */
    /* Step 2: Email-link path — existing password-account user signs in   */
    /*         via OIDC for the first time.                                */
    /* ------------------------------------------------------------------ */
    if (email_ok_for_link) {
        int email_account_id = -1;
        int row_count = oidc_rp_link_query_082_lookup_by_email(claims->email,
                                                                database,
                                                                &email_account_id);

        if (row_count < 0) {
            log_this(SR_AUTH,
                     "OIDC RP linker MATCH_EMAIL_THEN_PROVISION: database error on #082 "
                     "lookup iss=%s sub=%s...",
                     LOG_LEVEL_ERROR, 2, iss_log, sub_log);
            return OIDC_RP_LINK_DB_ERROR;
        }

        if (row_count >= 2) {
            /* Two accounts share the same email — admin must resolve. Do NOT
             * fall through to provisioning; that would create a third
             * account and make the admin's job harder. */
            log_this(SR_AUTH,
                     "OIDC RP linker MATCH_EMAIL_THEN_PROVISION: email_ambiguous "
                     "(multiple accounts) iss=%s sub=%s...",
                     LOG_LEVEL_ALERT, 2, iss_log, sub_log);
            return OIDC_RP_LINK_EMAIL_AMBIGUOUS;
        }

        if (row_count == 1 && email_account_id >= 0) {
            /* Single unambiguous match — link the OIDC identity to the
             * existing account (first-time OIDC sign-in for a password user). */
            int new_identity_id = -1;
            bool link_ok = oidc_rp_link_query_081_link_identity(
                email_account_id,
                claims->iss, claims->sub, claims->email,
                claims->email_verified, database,
                &new_identity_id);

            if (!link_ok) {
                log_this(SR_AUTH,
                         "OIDC RP linker MATCH_EMAIL_THEN_PROVISION: #081 link failed "
                         "iss=%s sub=%s...",
                         LOG_LEVEL_ERROR, 2, iss_log, sub_log);
                return OIDC_RP_LINK_DB_ERROR;
            }

            /* Handle UNIQUE race: re-fetch if #081 returned no identity_id. */
            if (new_identity_id < 0) {
                int refetch_identity_id = -1;
                int refetch_account_id  = -1;
                db_error = false;
                oidc_rp_link_query_080_lookup(claims->iss, claims->sub, database,
                                              &refetch_identity_id,
                                              &refetch_account_id,
                                              &db_error);
                if (!db_error && refetch_identity_id >= 0) {
                    new_identity_id = refetch_identity_id;
                    if (refetch_account_id >= 0) {
                        email_account_id = refetch_account_id;
                    }
                } else {
                    log_this(SR_AUTH,
                             "OIDC RP linker MATCH_EMAIL_THEN_PROVISION: "
                             "post-race #080 re-fetch miss, skipping #084 touch",
                             LOG_LEVEL_ALERT, 0);
                }
            }

            if (new_identity_id >= 0) {
                oidc_rp_link_query_084_touch(new_identity_id, claims->email,
                                             claims->email_verified, database);
            }

            log_this(SR_AUTH,
                     "OIDC RP linker MATCH_EMAIL_THEN_PROVISION: email-linked "
                     "account_id=%d for iss=%s sub=%s...",
                     LOG_LEVEL_DEBUG, 3, email_account_id, iss_log, sub_log);

            account_info_t *account =
                oidc_rp_link_build_account_info(email_account_id, claims);
            if (!account) return OIDC_RP_LINK_DB_ERROR;
            *out_account = account;
            return OIDC_RP_LINK_OK;
        }

        /* row_count == 0: no existing account with this email — fall through
         * to provisioning if enabled. */
    }

    /* ------------------------------------------------------------------ */
    /* Step 3: Provision path — brand-new user.                            */
    /* ------------------------------------------------------------------ */
    if (!linking->provision_defaults.enabled) {
        /* Provisioning disabled — nothing left to try. */
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_THEN_PROVISION: no match and "
                 "ProvisionDefaults.Enabled=false iss=%s sub=%s...",
                 LOG_LEVEL_DEBUG, 2, iss_log, sub_log);
        return OIDC_RP_LINK_NO_ACCOUNT;
    }

    if (!email_usable) {
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_THEN_PROVISION: no email in claims, "
                 "cannot provision iss=%s sub=%s...",
                 LOG_LEVEL_ALERT, 2, iss_log, sub_log);
        return OIDC_RP_LINK_NO_ACCOUNT;
    }

    if (linking->require_email_verified && !claims->email_verified) {
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_THEN_PROVISION: email not verified, "
                 "RequireEmailVerified=true, cannot provision iss=%s sub=%s...",
                 LOG_LEVEL_ALERT, 2, iss_log, sub_log);
        return OIDC_RP_LINK_NO_ACCOUNT;
    }

    if (!email_domain_allowed_def(linking, claims->email)) {
        const char *dom = email_domain_def(claims->email);
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_THEN_PROVISION: email domain '%s' not on "
                 "AllowedEmailDomains iss=%s sub=%s...",
                 LOG_LEVEL_ALERT, 3,
                 dom ? dom : "(malformed)", iss_log, sub_log);
        return OIDC_RP_LINK_PROVISION_DISALLOWED_EMAIL;
    }

    int new_account_id = oidc_rp_link_query_083_provision(claims, database);
    if (new_account_id < 0) {
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_THEN_PROVISION: #083 provision failed "
                 "iss=%s sub=%s...",
                 LOG_LEVEL_ERROR, 2, iss_log, sub_log);
        return OIDC_RP_LINK_DB_ERROR;
    }

    int prov_identity_id = -1;
    bool link_ok = oidc_rp_link_query_081_link_identity(
        new_account_id,
        claims->iss, claims->sub, claims->email,
        claims->email_verified, database,
        &prov_identity_id);

    if (!link_ok) {
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_THEN_PROVISION: #081 link failed AFTER "
                 "#083 provisioned account_id=%d (orphan account; next sign-in retries) "
                 "iss=%s sub=%s...",
                 LOG_LEVEL_ERROR, 3, new_account_id, iss_log, sub_log);
        return OIDC_RP_LINK_DB_ERROR;
    }

    if (prov_identity_id >= 0) {
        oidc_rp_link_query_084_touch(prov_identity_id, claims->email,
                                     claims->email_verified, database);
    }

    if (linking->provision_defaults.default_role_count > 0) {
        log_this(SR_AUTH,
                 "OIDC RP linker MATCH_EMAIL_THEN_PROVISION: %zu default roles configured for "
                 "account_id=%d (assignment deferred to Phase 22)",
                 LOG_LEVEL_STATE, 2,
                 linking->provision_defaults.default_role_count, new_account_id);
    }

    log_this(SR_AUTH,
             "OIDC RP linker MATCH_EMAIL_THEN_PROVISION: provisioned account_id=%d "
             "for iss=%s sub=%s...",
             LOG_LEVEL_STATE, 3, new_account_id, iss_log, sub_log);

    account_info_t *account = oidc_rp_link_build_account_info(new_account_id, claims);
    if (!account) return OIDC_RP_LINK_DB_ERROR;
    *out_account = account;
    return OIDC_RP_LINK_OK;
}
