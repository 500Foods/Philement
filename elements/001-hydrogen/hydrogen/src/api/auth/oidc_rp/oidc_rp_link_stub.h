/**
 * @file oidc_rp_link_stub.h
 * @brief OIDC RP account linker — Phase 14 throwaway stub.
 *
 * Phase 14 of the OIDC plan (`docs/OIDC-PLAN.md`) wires the full
 * `/start → IdP → /callback → mint JWT → handoff` chain end-to-end.
 * The "resolve OIDC identity to a Hydrogen account" step needs the
 * `account_oidc_identities` table (Phase 15) and the linker
 * QueryRefs (Phase 17) to be real, neither of which exists yet.
 *
 * This module is the deliberate throwaway: it short-circuits the
 * linker step by returning the *single existing* test account
 * (`account_id = 1`, looked up via the regular password-login
 * `lookup_account` helper). That gives Phase 14 a fully working
 * end-to-end black-box test without dragging schema work into the
 * critical path. Phase 21 deletes this file entirely once
 * `oidc_rp_link.c` (the real linker) replaces it.
 *
 * The only consumer is `oidc_rp_callback.c`. The header lives next
 * to it on purpose — when Phase 21 lands, both it and its `.c`
 * companion are deleted in a single commit, with `oidc_rp_link.h`
 * taking over.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_LINK_STUB_H
#define OIDC_RP_LINK_STUB_H

#include <stdbool.h>

#include <src/api/auth/auth_service.h>
#include "oidc_rp_idtoken.h"

/**
 * @brief Resolve the validated ID-token claims to a Hydrogen account.
 *
 * Phase 14 stub: ignores the claims entirely and looks up the
 * fixed test account `"admin"` via the normal `lookup_account`
 * helper, yielding the same `account_info_t*` that the password
 * login flow uses. Returns NULL when the test account does not
 * exist, when the database is unreachable, or when the lookup
 * helper fails for any other reason.
 *
 * Phase 21 replaces this with the real four-strategy linker
 * (`match_sub_only`, `match_email_only`,
 * `match_email_then_provision`, `provision_only`) that consults
 * `account_oidc_identities` via QueryRefs `#080`–`#084`.
 *
 * @param claims    Validated ID-token claims from
 *                  `oidc_rp_validate_id_token` (Phase 12). May be
 *                  NULL — the stub ignores it.
 * @param database  The Hydrogen database name to look up against
 *                  (typically `"Lithium"`).
 * @return Newly-allocated `account_info_t*` on success, NULL on
 *         failure. Caller must `free_account_info()` the result.
 */
account_info_t *oidc_rp_link_stub_resolve(const OidcRpIdTokenClaims *claims,
                                          const char *database);

#endif /* OIDC_RP_LINK_STUB_H */
