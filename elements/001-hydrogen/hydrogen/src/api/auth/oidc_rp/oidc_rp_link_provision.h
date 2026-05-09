/**
 * @file oidc_rp_link_provision.h
 * @brief OIDC RP account linker — `provision_only` strategy (Phase 20).
 *
 * 1. Try QueryRef #080 first (sub fast-path for already-provisioned users).
 * 2. Validate prerequisites: Enabled, email present, email_verified,
 *    AllowedEmailDomains.
 * 3. Provision via QueryRef #083, link via #081, touch via #084.
 * 4. Default-role assignment is DEFERRED to Phase 22.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_LINK_PROVISION_H
#define OIDC_RP_LINK_PROVISION_H

#include <src/config/config_oidc_rp.h>
#include "oidc_rp_link.h"
#include "oidc_rp_idtoken.h"

/**
 * @brief Run the `provision_only` linking strategy.
 *
 * @param provider     Active provider config (for AllowedEmailDomains etc.).
 * @param claims       Validated ID-token claims.
 * @param database     Hydrogen database name.
 * @param out_account  On OIDC_RP_LINK_OK, newly-allocated account_info_t.
 * @return             Typed result code.
 */
OidcRpLinkResult oidc_rp_link_provision_only(const OIDCRPProviderConfig *provider,
                                              const OidcRpIdTokenClaims *claims,
                                              const char *database,
                                              account_info_t **out_account);

#endif /* OIDC_RP_LINK_PROVISION_H */
