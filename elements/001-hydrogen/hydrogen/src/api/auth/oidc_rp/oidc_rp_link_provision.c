/*
 * OIDC RP account linker — `provision_only` strategy (Phase 20).
 *
 * Strategy:
 *   1. QueryRef #080 — fast path for already-provisioned users.
 *   2. Validate prerequisites:
 *      a. ProvisionDefaults.Enabled must be true → else NO_ACCOUNT.
 *      b. email must be present → else NO_ACCOUNT.
 *      c. email_verified must be true if RequireEmailVerified → else NO_ACCOUNT.
 *      d. email domain must be on AllowedEmailDomains → else
 *         PROVISION_DISALLOWED_EMAIL.
 *   3. QueryRef #083 — provision new accounts row (password_hash = NULL).
 *   4. QueryRef #081 — link the new account to the OIDC identity.
 *   5. QueryRef #084 — touch last_seen_at.
 *   6. Default-role assignment is DEFERRED to Phase 22.
 *
 * The "orphan account" path (step 3 succeeds but step 4 fails) is logged
 * loudly and returns DB_ERROR. The user's next sign-in will retry step 4.
 * Phase 22 may add a transactional wrapper if orphan rows accumulate.
 *
 * Extracted from the original oidc_rp_link.c monolith in Phase 21.
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "oidc_rp_link_provision.h"
#include "oidc_rp_link_internal.h"

#include <ctype.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Domain allow-list helpers
 * -------------------------------------------------------------------------
 */

static bool domains_equal_ci(const char *a, const char *b) {
    if (!a || !b) return false;
    while (*a && *b) {
        char ca = (char)tolower((unsigned char)*a);
        char cb = (char)tolower((unsigned char)*b);
        if (ca != cb) return false;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static const char *email_domain(const char *email) {
    if (!email || !*email) return NULL;
    const char *at = strrchr(email, '@');
    if (!at)         return NULL;
    if (at == email) return NULL; /* "@example.com" — empty local part */
    if (!at[1])      return NULL; /* "alice@" — empty domain */
    return at + 1;
}

static bool email_domain_allowed(const OIDCRPAccountLinking *linking,
                                  const char *email) {
    if (linking->allowed_email_domain_count == 0) {
        /* No allow-list = no restriction (Phase 20 lesson #3). */
        return true;
    }
    const char *dom = email_domain(email);
    if (!dom) return false;
    for (size_t i = 0; i < linking->allowed_email_domain_count; i++) {
        const char *entry = linking->allowed_email_domains[i];
        if (entry && domains_equal_ci(dom, entry)) return true;
    }
    return false;
}

/* -------------------------------------------------------------------------
 * Public strategy entry point
 * -------------------------------------------------------------------------
 */
OidcRpLinkResult oidc_rp_link_provision_only(const OIDCRPProviderConfig *provider,
                                              const OidcRpIdTokenClaims *claims,
                                              const char *database,
                                              account_info_t **out_account) {
    char iss_log[OIDC_RP_LINK_ISS_LOG_LEN + 1];
    char sub_log[OIDC_RP_LINK_SUB_LOG_LEN + 1];
    oidc_rp_link_safe_iss(claims->iss, iss_log);
    oidc_rp_link_safe_sub(claims->sub, sub_log);

    /* Step 1: fast path — already-provisioned users never re-provision. */
    int  identity_id = -1;
    int  account_id  = -1;
    bool db_error    = false;

    oidc_rp_link_query_080_lookup(claims->iss, claims->sub,
                                  database,
                                  &identity_id, &account_id,
                                  &db_error);

    if (db_error) {
        log_this(SR_AUTH,
                 "OIDC RP linker PROVISION_ONLY: database error on #080 lookup iss=%s sub=%s...",
                 LOG_LEVEL_ERROR, 2, iss_log, sub_log);
        return OIDC_RP_LINK_DB_ERROR;
    }

    if (identity_id >= 0 && account_id >= 0) {
        oidc_rp_link_query_084_touch(identity_id, claims->email,
                                     claims->email_verified, database);
        log_this(SR_AUTH,
                 "OIDC RP linker PROVISION_ONLY: sub-hit account_id=%d for iss=%s sub=%s...",
                 LOG_LEVEL_DEBUG, 3, account_id, iss_log, sub_log);
        account_info_t *account = oidc_rp_link_build_account_info(account_id, claims);
        if (!account) return OIDC_RP_LINK_DB_ERROR;
        *out_account = account;
        return OIDC_RP_LINK_OK;
    }

    /* Step 2a: provisioning enabled? */
    const OIDCRPAccountLinking *linking = &provider->account_linking;

    if (!linking->provision_defaults.enabled) {
        log_this(SR_AUTH,
                 "OIDC RP linker PROVISION_ONLY: ProvisionDefaults.Enabled=false iss=%s sub=%s...",
                 LOG_LEVEL_ALERT, 2, iss_log, sub_log);
        return OIDC_RP_LINK_NO_ACCOUNT;
    }

    /* Step 2b: email present? */
    if (!claims->email || !*claims->email) {
        log_this(SR_AUTH,
                 "OIDC RP linker PROVISION_ONLY: no email in claims iss=%s sub=%s...",
                 LOG_LEVEL_ALERT, 2, iss_log, sub_log);
        return OIDC_RP_LINK_NO_ACCOUNT;
    }

    /* Step 2c: email verified? */
    if (linking->require_email_verified && !claims->email_verified) {
        log_this(SR_AUTH,
                 "OIDC RP linker PROVISION_ONLY: email not verified, RequireEmailVerified=true iss=%s sub=%s...",
                 LOG_LEVEL_ALERT, 2, iss_log, sub_log);
        return OIDC_RP_LINK_NO_ACCOUNT;
    }

    /* Step 2d: domain on AllowedEmailDomains? */
    if (!email_domain_allowed(linking, claims->email)) {
        const char *dom = email_domain(claims->email);
        log_this(SR_AUTH,
                 "OIDC RP linker PROVISION_ONLY: email domain '%s' not on AllowedEmailDomains iss=%s sub=%s...",
                 LOG_LEVEL_ALERT, 3,
                 dom ? dom : "(malformed)", iss_log, sub_log);
        return OIDC_RP_LINK_PROVISION_DISALLOWED_EMAIL;
    }

    /* Step 3: provision via #083. */
    int new_account_id = oidc_rp_link_query_083_provision(claims, database);
    if (new_account_id < 0) {
        log_this(SR_AUTH,
                 "OIDC RP linker PROVISION_ONLY: #083 provision failed iss=%s sub=%s...",
                 LOG_LEVEL_ERROR, 2, iss_log, sub_log);
        return OIDC_RP_LINK_DB_ERROR;
    }

    /* Step 4: link the new account via #081. */
    int new_identity_id = -1;
    bool link_ok = oidc_rp_link_query_081_link_identity(new_account_id,
                                                         claims->iss,
                                                         claims->sub,
                                                         claims->email,
                                                         claims->email_verified,
                                                         database,
                                                         &new_identity_id);
    if (!link_ok) {
        log_this(SR_AUTH,
                 "OIDC RP linker PROVISION_ONLY: #081 link failed AFTER #083 provisioned account_id=%d "
                 "(orphan account; next sign-in will retry #081) iss=%s sub=%s...",
                 LOG_LEVEL_ERROR, 3, new_account_id, iss_log, sub_log);
        return OIDC_RP_LINK_DB_ERROR;
    }

    /* Step 5: touch via #084 (best-effort). */
    if (new_identity_id >= 0) {
        oidc_rp_link_query_084_touch(new_identity_id, claims->email,
                                     claims->email_verified, database);
    }

    /* Step 6: default-role assignment deferred to Phase 22. Log intent. */
    if (linking->provision_defaults.default_role_count > 0) {
        log_this(SR_AUTH,
                 "OIDC RP linker PROVISION_ONLY: %zu default roles configured for "
                 "account_id=%d (assignment deferred to Phase 22)",
                 LOG_LEVEL_STATE, 2,
                 linking->provision_defaults.default_role_count, new_account_id);
    }

    log_this(SR_AUTH,
             "OIDC RP linker PROVISION_ONLY: provisioned account_id=%d for iss=%s sub=%s...",
             LOG_LEVEL_STATE, 3, new_account_id, iss_log, sub_log);

    account_info_t *account = oidc_rp_link_build_account_info(new_account_id, claims);
    if (!account) return OIDC_RP_LINK_DB_ERROR;
    *out_account = account;
    return OIDC_RP_LINK_OK;
}
