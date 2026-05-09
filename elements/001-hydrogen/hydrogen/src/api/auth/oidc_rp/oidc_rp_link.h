/**
 * @file oidc_rp_link.h
 * @brief OIDC RP account linker — resolve an IdP identity to a Hydrogen account.
 *
 * Phase 18 of the OIDC plan (`docs/OIDC-PLAN.md`) implemented
 * `match_sub_only`: look up the OIDC identity by `(issuer, subject)` via
 * QueryRef #080; on a hit, update `last_seen_at` via QueryRef #084 and
 * return the resolved account.
 *
 * Phase 19 adds `match_email_only` (QueryRefs #080, #082, #081):
 *   1. Try QueryRef #080 (sub-match) first — already-linked users stay fast.
 *   2. On sub-miss, if `email_verified == true` (and `RequireEmailVerified`
 *      config allows it), call QueryRef #082 (lookup by email in
 *      `account_contacts`).
 *   3. On a single-row hit: call QueryRef #081 to insert a new
 *      `account_oidc_identities` row, linking the account. Return the account.
 *   4. On a multi-row hit (two accounts with the same email): reject with
 *      `OIDC_RP_LINK_EMAIL_AMBIGUOUS` — admin must merge accounts first.
 *   5. On a miss (or unverified email): return `OIDC_RP_LINK_NO_ACCOUNT`.
 *
 * Subsequent phases extend this module further:
 *   - Phase 20 adds auto-provisioning (QueryRef #083).
 *   - Phase 21 composes all strategies into the default
 *     `match_email_then_provision` flow and deletes
 *     `oidc_rp_link_stub.{c,h}`.
 *
 * The strategy to apply is read from the active provider's
 * `account_linking.strategy` field (parsed in Phase 5). Phases 18–19
 * handle `OIDC_RP_LINK_MATCH_SUB_ONLY` and `OIDC_RP_LINK_MATCH_EMAIL_ONLY`;
 * remaining strategies fall through to the stub linker until implemented.
 *
 * Logging policy:
 *   - NEVER log `email` on the success path. Only log `email` on
 *     failed-link events so operators can diagnose mismatches.
 *   - Always log `iss` (truncated to 64 chars), `sub` (truncated to
 *     16 chars), `account_id` (on success), and `strategy`.
 *   - Use `SR_AUTH` subsystem with `OIDC_RP` prefix per the plan's
 *     `docs/OIDC-PLAN.md` § "Logging".
 *
 * Thread safety: all public functions are reentrant. The database
 * queue they invoke is itself thread-safe.
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
    OIDC_RP_LINK_EMAIL_AMBIGUOUS = 6  /**< Two accounts share the same email;
                                           admin must merge before linking.
                                           Added in Phase 19. */
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
 * Phases 18–19 implement `OIDC_RP_LINK_MATCH_SUB_ONLY` and
 * `OIDC_RP_LINK_MATCH_EMAIL_ONLY`. For `match_sub_only`:
 *   1. Call QueryRef #080 with `(issuer, subject)` from `claims`.
 *   2. On zero rows: return `OIDC_RP_LINK_NO_ACCOUNT`.
 *   3. On one row: call QueryRef #084 to update `last_seen_at`,
 *      then populate the `account_info_t`.
 *
 * For `match_email_only`:
 *   1. Try #080 first (already-linked users stay fast).
 *   2. On sub-miss and email_verified: call QueryRef #082 (lookup by email).
 *   3. One row → call QueryRef #081 to link, then return account.
 *   4. Two+ rows → return `OIDC_RP_LINK_EMAIL_AMBIGUOUS`.
 *   5. Zero rows (or unverified email) → return `OIDC_RP_LINK_NO_ACCOUNT`.
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
