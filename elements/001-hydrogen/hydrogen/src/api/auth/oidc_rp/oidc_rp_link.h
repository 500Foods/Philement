/**
 * @file oidc_rp_link.h
 * @brief OIDC RP account linker — resolve an IdP identity to a Hydrogen account.
 *
 * Public API for the account-linking step of the OIDC RP flow. The
 * implementation is split across dedicated per-strategy modules:
 *
 *   - oidc_rp_link_sub.c      — `match_sub_only`   (Phase 18)
 *   - oidc_rp_link_email.c    — `match_email_only`  (Phase 19)
 *   - oidc_rp_link_provision.c — `provision_only`   (Phase 20)
 *   - oidc_rp_link_default.c  — `match_email_then_provision` (Phase 21)
 *
 * Shared helpers (QueryRef wrappers, account_info_t builder, test seam)
 * live in oidc_rp_link_internal.{c,h}.
 *
 * Strategy overview:
 *
 *   `match_sub_only` — trust the (iss, sub) tuple only; fail if no link.
 *   `match_email_only` — fast-path via (iss, sub); on miss, link by
 *     verified email; EMAIL_AMBIGUOUS if two accounts share the email.
 *   `provision_only` — fast-path via (iss, sub); on miss, provision a
 *     new account if prerequisites (Enabled, email_verified, domain) pass.
 *   `match_email_then_provision` (default) — fast-path via (iss, sub);
 *     on miss, try email-link; on email-miss, provision if allowed.
 *
 * Phase 22 will add role-mapping (default-role assignment for provisioned
 * accounts and IdP role-claim → Hydrogen role mapping).
 *
 * Logging policy:
 *   - NEVER log `email` on the success path.
 *   - Log `iss` (truncated to 64 chars) and `sub` (truncated to 16 chars)
 *     on every outcome; log `account_id` on success.
 *   - Use `SR_AUTH` subsystem with `OIDC_RP` prefix.
 *
 * Thread safety: all public functions are reentrant.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_LINK_H
#define OIDC_RP_LINK_H

#include <stdbool.h>

#include <src/api/auth/auth_service.h>
#include <src/config/config_oidc_rp.h>
#include "oidc_rp_idtoken.h"

/**
 * @brief Typed result code for the account-linking step.
 *
 * Stable across releases. `oidc_rp_callback.c` maps each value to
 * the corresponding `?oidc_error=<name>` redirect parameter.
 */
typedef enum OidcRpLinkResult {
    OIDC_RP_LINK_OK = 0,              /**< Identity resolved to an account. */
    OIDC_RP_LINK_NO_ACCOUNT = 1,      /**< No matching account found (any strategy). */
    OIDC_RP_LINK_BAD_INPUT = 2,       /**< NULL/invalid argument. */
    OIDC_RP_LINK_DB_ERROR = 3,        /**< Database query failed. */
    OIDC_RP_LINK_DISABLED = 4,        /**< OIDC feature is disabled. */
    OIDC_RP_LINK_ACCOUNT_DISABLED = 5,/**< Account exists but is disabled. */
    OIDC_RP_LINK_EMAIL_AMBIGUOUS = 6, /**< Two accounts share the same email;
                                           admin must merge before linking.
                                           Added in Phase 19. */
    OIDC_RP_LINK_PROVISION_DISALLOWED_EMAIL = 7
                                      /**< Provisioning was attempted but the
                                           IdP-supplied email domain is not on
                                           the configured `AllowedEmailDomains`
                                           list. Added in Phase 20. */
} OidcRpLinkResult;

/**
 * @brief Convert a link-result code to a stable lowercase string.
 *
 * Used for logging and for mapping to `?oidc_error=<name>` redirect
 * parameters. Never returns NULL — unknown values map to `"unknown"`.
 *
 * @param result  The result code.
 * @return        Stable lowercase string.
 */
const char *oidc_rp_link_result_name(OidcRpLinkResult result);

/**
 * @brief Resolve a validated OIDC identity to a Hydrogen account.
 *
 * Applies the strategy from `provider->account_linking.strategy` to
 * find or create an `accounts` row and return a populated
 * `account_info_t`.
 *
 * All four strategies are implemented as of Phase 21. See the per-strategy
 * source files for detailed flow descriptions:
 *   - oidc_rp_link_sub.c      (match_sub_only)
 *   - oidc_rp_link_email.c    (match_email_only)
 *   - oidc_rp_link_provision.c (provision_only)
 *   - oidc_rp_link_default.c  (match_email_then_provision)
 *
 * On `OIDC_RP_LINK_OK`, `*out_account` is set to a newly-allocated
 * `account_info_t*` with `id`, `enabled`, `authorized`, `username`,
 * `email`, and `roles` populated. Caller must `free_account_info`
 * the result.
 *
 * On any failure, `*out_account` is set to NULL and an appropriate
 * typed result is returned.
 *
 * @param provider      Active provider config. Must be non-NULL.
 * @param claims        Validated ID-token claims from Phase 12. Must
 *                      be non-NULL with non-empty `iss` and `sub`.
 * @param database      Hydrogen database name to query against. Must
 *                      be non-NULL and non-empty.
 * @param out_account   On `OIDC_RP_LINK_OK`, set to a newly-allocated
 *                      `account_info_t*`. On failure, set to NULL.
 *                      Must be non-NULL.
 * @return              Typed result code.
 */
OidcRpLinkResult oidc_rp_link_resolve(const OIDCRPProviderConfig *provider,
                                       const OidcRpIdTokenClaims  *claims,
                                       const char                 *database,
                                       account_info_t            **out_account);

/**
 * @brief Test seam: override the database lookup for unit tests.
 *
 * When non-NULL, `oidc_rp_link_resolve` calls this function instead
 * of `execute_auth_query`. Used exclusively by Unity tests to inject
 * canned QueryResult responses without requiring a live database.
 *
 * Set to NULL (the default) to restore normal behaviour.
 *
 * @param fn  Function pointer with the same signature as
 *            `execute_auth_query`, or NULL to disable the override.
 */

/* Re-use the same signature as execute_auth_query.                   */
/* QueryResult is declared in src/api/auth/auth_service.h via         */
/* src/database/database.h — no forward declaration needed here.      */
typedef struct QueryResult QueryResult;

typedef QueryResult *(*OidcRpLinkQueryFn)(int query_ref,
                                          const char *database,
                                          json_t *params);

/**
 * @brief Install a test-seam query function.
 *
 * Only available in non-NDEBUG builds. The seam is process-global and
 * is NOT thread-safe during installation/removal — install before any
 * concurrent callers and remove after they all finish. In Unity tests,
 * install in `setUp` and remove in `tearDown`.
 *
 * @param fn  Replacement for `execute_auth_query`, or NULL to disable.
 */
void oidc_rp_link_test_set_query_fn(OidcRpLinkQueryFn fn);

/**
 * @brief Clear the test-seam query function (convenience wrapper).
 *
 * Equivalent to `oidc_rp_link_test_set_query_fn(NULL)`.
 */
void oidc_rp_link_test_clear_query_fn(void);

#endif /* OIDC_RP_LINK_H */
