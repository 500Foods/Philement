/**
 * @file oidc_rp_link_email.h
 * @brief OIDC RP account linker — `match_email_only` strategy (Phase 19).
 *
 * 1. Try QueryRef #080 first (sub fast-path).
 * 2. On sub-miss, look up the email via QueryRef #082.
 * 3. Single hit → insert identity link via #081, touch via #084, return OK.
 * 4. Two+ hits → EMAIL_AMBIGUOUS.
 * 5. Zero hits (or unverified when required) → NO_ACCOUNT.
 *
 * Requires the provider config for `RequireEmailVerified`.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_LINK_EMAIL_H
#define OIDC_RP_LINK_EMAIL_H

#include <src/config/config_oidc_rp.h>
#include "oidc_rp_link.h"
#include "oidc_rp_idtoken.h"

/**
 * @brief Run the `match_email_only` linking strategy.
 *
 * @param provider     Active provider config (for RequireEmailVerified).
 * @param claims       Validated ID-token claims.
 * @param database     Hydrogen database name.
 * @param out_account  On OIDC_RP_LINK_OK, newly-allocated account_info_t.
 * @return             Typed result code.
 */
OidcRpLinkResult oidc_rp_link_match_email_only(const OIDCRPProviderConfig *provider,
                                                const OidcRpIdTokenClaims *claims,
                                                const char *database,
                                                account_info_t **out_account);

#endif /* OIDC_RP_LINK_EMAIL_H */
