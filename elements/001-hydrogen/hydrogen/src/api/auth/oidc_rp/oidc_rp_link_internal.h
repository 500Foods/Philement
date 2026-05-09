/**
 * @file oidc_rp_link_internal.h
 * @brief OIDC RP account linker — shared internal helpers.
 *
 * This header is ONLY for use by the per-strategy linker modules:
 *   - oidc_rp_link_sub.c      (match_sub_only)
 *   - oidc_rp_link_email.c    (match_email_only)
 *   - oidc_rp_link_provision.c (provision_only)
 *   - oidc_rp_link_default.c  (match_email_then_provision)
 *   - oidc_rp_link.c          (dispatch coordinator)
 *
 * It must NOT be included from outside the oidc_rp/ directory. Public
 * callers use oidc_rp_link.h only.
 *
 * Shared surfaces:
 *   - Test seam: `oidc_rp_link_run_query` routes through the seam when
 *     installed, otherwise delegates to `execute_auth_query`.
 *   - Logging helpers: `oidc_rp_link_safe_iss` / `oidc_rp_link_safe_sub`
 *     (truncate identifiers for safe log output).
 *   - Account info builder: `oidc_rp_link_build_account_info`.
 *   - QueryRef wrappers: `query_080_lookup`, `query_081_link_identity`,
 *     `query_082_lookup_by_email`, `query_083_provision`,
 *     `query_084_touch`.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_LINK_INTERNAL_H
#define OIDC_RP_LINK_INTERNAL_H

#include <stdbool.h>

#include <src/api/auth/auth_service.h>
#include <src/config/config_oidc_rp.h>
#include "oidc_rp_link.h"
#include "oidc_rp_idtoken.h"

/* -------------------------------------------------------------------------
 * Log-safe identifier lengths
 * -------------------------------------------------------------------------
 * Truncation prevents foreign IdP identifiers from bloating log files.
 * The lengths are enough for operator diagnostics without exposing
 * full opaque subject strings.
 */
#define OIDC_RP_LINK_ISS_LOG_LEN 64
#define OIDC_RP_LINK_SUB_LOG_LEN 16

/**
 * @brief Truncate an issuer string for safe log output.
 *
 * @param iss  Source string (may be NULL).
 * @param out  Caller-supplied buffer of size OIDC_RP_LINK_ISS_LOG_LEN + 1.
 */
void oidc_rp_link_safe_iss(const char *iss,
                            char out[OIDC_RP_LINK_ISS_LOG_LEN + 1]);

/**
 * @brief Truncate a subject string for safe log output.
 *
 * @param sub  Source string (may be NULL).
 * @param out  Caller-supplied buffer of size OIDC_RP_LINK_SUB_LOG_LEN + 1.
 */
void oidc_rp_link_safe_sub(const char *sub,
                            char out[OIDC_RP_LINK_SUB_LOG_LEN + 1]);

/* -------------------------------------------------------------------------
 * Test seam
 * -------------------------------------------------------------------------
 * Installed by `oidc_rp_link_test_set_query_fn`; NULL in production.
 * All QueryRef wrappers below call through this function pointer so that
 * Unity tests can inject canned responses without a live database.
 */

/**
 * @brief Route a QueryRef call through the test seam (if installed).
 *
 * In production (seam NULL) this calls `execute_auth_query` directly.
 * In tests (seam set by `oidc_rp_link_test_set_query_fn`) the seam
 * function is called instead.
 */
QueryResult *oidc_rp_link_run_query(int query_ref,
                                    const char *database,
                                    json_t *params);

/* -------------------------------------------------------------------------
 * Account info builder
 * -------------------------------------------------------------------------
 */

/**
 * @brief Allocate and populate an account_info_t from an account_id and claims.
 *
 * Sets id, enabled=true, authorized=true, username (preferred_username
 * or sub as fallback), email (from claims if present), and roles="".
 *
 * Phase 22 will replace the username and roles fields with authoritative
 * DB values from the account_roles join. For now the claims-based values
 * are the correct OIDC-path defaults.
 *
 * @param account_id  Resolved account_id.
 * @param claims      Validated ID-token claims.
 * @return            New heap-allocated account_info_t, or NULL on OOM.
 *                    Caller must call free_account_info() when done.
 */
account_info_t *oidc_rp_link_build_account_info(
    int account_id,
    const OidcRpIdTokenClaims *claims);

/* -------------------------------------------------------------------------
 * QueryRef wrappers
 * -------------------------------------------------------------------------
 * Each wrapper encapsulates the JSON parameter building and result
 * parsing for one QueryRef. They all call `oidc_rp_link_run_query`
 * so the test seam is transparent.
 */

/**
 * @brief QueryRef #080 — look up an OIDC identity by (issuer, subject).
 *
 * @param issuer          Issuer string from validated ID token.
 * @param subject         Subject string from validated ID token.
 * @param database        Hydrogen database name.
 * @param out_identity_id Set to the identity_id on a hit, -1 on miss/error.
 * @param out_account_id  Set to the account_id on a hit, -1 on miss/error.
 * @param out_db_error    Set to true on a database error (not a clean miss).
 */
void oidc_rp_link_query_080_lookup(const char *issuer,
                                   const char *subject,
                                   const char *database,
                                   int *out_identity_id,
                                   int *out_account_id,
                                   bool *out_db_error);

/**
 * @brief QueryRef #081 — link a new OIDC identity to an existing account.
 *
 * Inserts a row into account_oidc_identities. On a UNIQUE constraint
 * violation (concurrent sign-in race), returns true with *out_identity_id
 * == -1 so the caller can retry with #080.
 *
 * @param account_id       Target account_id.
 * @param issuer           Issuer from validated ID token.
 * @param subject          Subject from validated ID token.
 * @param email            Email (may be NULL).
 * @param email_verified   Whether the email is verified.
 * @param database         Hydrogen database name.
 * @param out_identity_id  Set to the new identity_id on success, -1 on
 *                         UNIQUE race (also reported as success).
 * @return                 true on success or UNIQUE race; false on genuine error.
 */
bool oidc_rp_link_query_081_link_identity(int account_id,
                                          const char *issuer,
                                          const char *subject,
                                          const char *email,
                                          bool email_verified,
                                          const char *database,
                                          int *out_identity_id);

/**
 * @brief QueryRef #082 — look up account_id(s) by email.
 *
 * @param email           Email address to look up.
 * @param database        Hydrogen database name.
 * @param out_account_id  Set to the account_id when exactly one row is found.
 * @return                Number of rows (0 = miss, 1 = unambiguous, 2 = ambiguous),
 *                        or -1 on database error.
 */
int oidc_rp_link_query_082_lookup_by_email(const char *email,
                                           const char *database,
                                           int *out_account_id);

/**
 * @brief QueryRef #083 — provision a new accounts row from OIDC claims.
 *
 * Creates the accounts row only. Caller MUST immediately call #081 to
 * insert the account_oidc_identities link (per Phase 17 lesson #3).
 *
 * @param claims    Validated ID-token claims.
 * @param database  Hydrogen database name.
 * @return          New account_id on success, -1 on failure.
 */
int oidc_rp_link_query_083_provision(const OidcRpIdTokenClaims *claims,
                                     const char *database);

/**
 * @brief QueryRef #084 — touch last_seen_at, email, email_verified.
 *
 * Best-effort: failure is logged at ALERT level but does not block login.
 *
 * @param identity_id    Identity row to update.
 * @param email          Email (may be NULL).
 * @param email_verified Whether the email is verified.
 * @param database       Hydrogen database name.
 */
void oidc_rp_link_query_084_touch(int identity_id,
                                  const char *email,
                                  bool email_verified,
                                  const char *database);

#endif /* OIDC_RP_LINK_INTERNAL_H */
