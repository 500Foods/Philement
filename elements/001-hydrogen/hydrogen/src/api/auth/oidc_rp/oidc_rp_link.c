/*
 * OIDC RP account linker — dispatch coordinator (Phase 21 refactor).
 *
 * This file is now a thin coordinator. It owns:
 *   - The process-global test-seam pointer (g_oidc_rp_link_query_fn).
 *   - oidc_rp_link_result_name — the error-name table.
 *   - oidc_rp_link_resolve — argument validation + strategy dispatch.
 *
 * The per-strategy implementations live in dedicated modules:
 *   - oidc_rp_link_sub.c      (OIDC_RP_LINK_MATCH_SUB_ONLY)
 *   - oidc_rp_link_email.c    (OIDC_RP_LINK_MATCH_EMAIL_ONLY)
 *   - oidc_rp_link_provision.c (OIDC_RP_LINK_PROVISION_ONLY)
 *   - oidc_rp_link_default.c  (OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION)
 *
 * The shared query helpers, logging helpers, account_info_t builder, and
 * the seam's run_query wrapper live in oidc_rp_link_internal.{c,h}.
 *
 * Phase 21 also deletes oidc_rp_link_stub.{c,h} (the Phase 14 throwaway
 * scaffold) and removes the corresponding include from oidc_rp_callback.c.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/auth/auth_service.h>

// Local includes
#include "oidc_rp_link.h"
#include "oidc_rp_link_sub.h"
#include "oidc_rp_link_email.h"
#include "oidc_rp_link_provision.h"
#include "oidc_rp_link_default.h"

/* -------------------------------------------------------------------------
 * Test seam — storage
 * -------------------------------------------------------------------------
 * The extern declaration in oidc_rp_link_internal.h allows the strategy
 * modules and the internal helpers to read this pointer without owning it.
 */
OidcRpLinkQueryFn g_oidc_rp_link_query_fn = NULL;

void oidc_rp_link_test_set_query_fn(OidcRpLinkQueryFn fn) {
    g_oidc_rp_link_query_fn = fn;
}

void oidc_rp_link_test_clear_query_fn(void) {
    g_oidc_rp_link_query_fn = NULL;
}

/* -------------------------------------------------------------------------
 * Error-name table
 * -------------------------------------------------------------------------
 */
const char *oidc_rp_link_result_name(OidcRpLinkResult result) {
    switch (result) {
        case OIDC_RP_LINK_OK:                        return "ok";
        case OIDC_RP_LINK_NO_ACCOUNT:                return "no_account";
        case OIDC_RP_LINK_BAD_INPUT:                 return "bad_input";
        case OIDC_RP_LINK_DB_ERROR:                  return "db_error";
        case OIDC_RP_LINK_DISABLED:                  return "disabled";
        case OIDC_RP_LINK_ACCOUNT_DISABLED:          return "account_disabled";
        case OIDC_RP_LINK_EMAIL_AMBIGUOUS:           return "email_ambiguous";
        case OIDC_RP_LINK_PROVISION_DISALLOWED_EMAIL:return "provision_disallowed_email";
    }
    return "unknown";
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

    switch (provider->account_linking.strategy) {
        case OIDC_RP_LINK_MATCH_SUB_ONLY:
            return oidc_rp_link_match_sub_only(claims, database, out_account);

        case OIDC_RP_LINK_MATCH_EMAIL_ONLY:
            return oidc_rp_link_match_email_only(provider, claims, database, out_account);

        case OIDC_RP_LINK_PROVISION_ONLY:
            return oidc_rp_link_provision_only(provider, claims, database, out_account);

        case OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION:
            return oidc_rp_link_match_email_then_provision(provider, claims, database,
                                                           out_account);
    }

    /* All enum values are handled above; this line is unreachable but
     * satisfies -Wreturn-type on compilers that do not detect exhaustive
     * switch coverage. */
    return OIDC_RP_LINK_NO_ACCOUNT;
}
