/**
 * @file oidc_rp_link_default.h
 * @brief OIDC RP account linker — `match_email_then_provision` strategy (Phase 21).
 *
 * This is the default strategy. It composes all three earlier strategies in
 * order of preference:
 *
 *   1. Sub-match via QueryRef #080 (already-linked users — always fastest).
 *   2. Email-link via QueryRef #082/#081 (first OIDC sign-in for an existing
 *      password-login account — links the OIDC identity to the account).
 *   3. Provision via QueryRef #083 (brand-new user — creates the account row,
 *      links the OIDC identity, defers default-role assignment to Phase 22).
 *   4. NO_ACCOUNT if none of the above succeed or are configured to run.
 *
 * Prerequisites for each step are checked against the active provider
 * config (RequireEmailVerified, AllowedEmailDomains, ProvisionDefaults).
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_LINK_DEFAULT_H
#define OIDC_RP_LINK_DEFAULT_H

#include <src/config/config_oidc_rp.h>
#include "oidc_rp_link.h"
#include "oidc_rp_idtoken.h"

/**
 * @brief Run the `match_email_then_provision` linking strategy.
 *
 * @param provider     Active provider config.
 * @param claims       Validated ID-token claims.
 * @param database     Hydrogen database name.
 * @param out_account  On OIDC_RP_LINK_OK, newly-allocated account_info_t.
 * @return             Typed result code.
 */
OidcRpLinkResult oidc_rp_link_match_email_then_provision(
    const OIDCRPProviderConfig *provider,
    const OidcRpIdTokenClaims  *claims,
    const char                 *database,
    account_info_t            **out_account);

#endif /* OIDC_RP_LINK_DEFAULT_H */
