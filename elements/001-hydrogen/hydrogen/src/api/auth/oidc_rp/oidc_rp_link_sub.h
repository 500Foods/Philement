/**
 * @file oidc_rp_link_sub.h
 * @brief OIDC RP account linker — `match_sub_only` strategy (Phase 18).
 *
 * Looks up the OIDC identity by (issuer, subject) via QueryRef #080.
 * On a hit, updates `last_seen_at` via QueryRef #084 and returns the
 * resolved account. On a miss, returns OIDC_RP_LINK_NO_ACCOUNT.
 *
 * This module is called by the dispatch coordinator in oidc_rp_link.c;
 * it must not be called directly from outside the oidc_rp/ directory.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_LINK_SUB_H
#define OIDC_RP_LINK_SUB_H

#include "oidc_rp_link.h"
#include "oidc_rp_idtoken.h"

/**
 * @brief Run the `match_sub_only` linking strategy.
 *
 * @param claims       Validated ID-token claims (non-NULL, non-empty iss/sub).
 * @param database     Hydrogen database name.
 * @param out_account  On OIDC_RP_LINK_OK, newly-allocated account_info_t.
 * @return             Typed result code.
 */
OidcRpLinkResult oidc_rp_link_match_sub_only(const OidcRpIdTokenClaims *claims,
                                              const char *database,
                                              account_info_t **out_account);

#endif /* OIDC_RP_LINK_SUB_H */
