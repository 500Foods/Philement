/**
 * @file oidc_rp_roles.h
 * @brief OIDC RP role-mapping — Phase 22.
 *
 * Resolves the `account_info_t::roles` field after the account linker has
 * identified an account but before `generate_jwt` is called. The four
 * mapping modes are:
 *
 *   DATABASE (default): call QueryRef #017 (`Get User Roles`) to load the
 *     account's roles from the `account_roles` table. IdP claims are ignored.
 *
 *   IDP_REALM_ROLES: copy `realm_access.roles` from the validated id_token
 *     claims directly. No database query. `IdpRolePrefix` is applied.
 *
 *   IDP_CLIENT_ROLES: copy `resource_access.<client_id>.roles` from the
 *     claims. Phase 12 populates `OidcRpIdTokenClaims::roles[]` from
 *     `realm_access.roles` only; `resource_access` roles are not yet parsed
 *     by the validator. For Phase 22, IDP_CLIENT_ROLES falls back to
 *     IDP_REALM_ROLES (both use the already-parsed roles[] array).
 *     This is documented in the implementation; a future phase may add a
 *     dedicated `client_roles[]` field to OidcRpIdTokenClaims.
 *
 *   MERGE: union of DATABASE roles and IDP_REALM_ROLES roles. Each role
 *     appears at most once (case-sensitive dedup). IdP-sourced roles get
 *     the `IdpRolePrefix` prefix; DB-sourced roles do not.
 *
 * The resulting roles string is a comma-separated list of role names,
 * e.g. `"admin,user"` or `"kc:reader,editor"`. Empty string `""` when
 * no roles apply. This matches the format `generate_jwt` already
 * embeds into the JWT `roles` claim.
 *
 * The function signature is designed for the callback's call site:
 *
 *   oidc_rp_roles_apply(provider, claims, database, account);
 *
 * On success, account->roles is replaced with a heap-allocated string.
 * On failure (OOM, DB error), account->roles is left as `""` (the linker's
 * default) so login continues with an empty roles list rather than blocking
 * the user. The failure is logged at ALERT level. This non-fatal discipline
 * mirrors QueryRef #084 touch (Phase 18 lesson #2).
 *
 * Test seam:
 *   `oidc_rp_roles_test_set_query_fn` / `_clear_query_fn` follow the same
 *   pattern as the linker seam. They are declared here and implemented in
 *   oidc_rp_roles.c.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_ROLES_H
#define OIDC_RP_ROLES_H

#include <src/api/auth/auth_service.h>
#include <src/config/config_oidc_rp.h>
#include "oidc_rp_idtoken.h"

/**
 * @brief Resolve and apply role mapping to `account->roles`.
 *
 * Called from `oidc_rp_callback.c` after `oidc_rp_link_resolve` succeeds
 * and before `generate_jwt`. Reads `provider->role_mapping.source` to
 * select the strategy, then overwrites `account->roles` with the resolved
 * comma-separated role list.
 *
 * Non-fatal: on any error the existing `account->roles` value is kept.
 *
 * @param provider   Active OIDC provider config (role_mapping fields used).
 * @param claims     Validated ID-token claims (roles[] for IdP sources).
 * @param database   Hydrogen database name (for QueryRef #017).
 * @param account    Resolved account; account->roles is updated in-place.
 */
void oidc_rp_roles_apply(const OIDCRPProviderConfig *provider,
                          const OidcRpIdTokenClaims   *claims,
                          const char                  *database,
                          account_info_t               *account);

/* -------------------------------------------------------------------------
 * Test seam
 * -------------------------------------------------------------------------
 * Same pattern as oidc_rp_link.h. Replaces execute_auth_query for Unity
 * tests; NULL in production (seam disabled). Each Unity binary installs
 * and removes the seam in setUp/tearDown.
 */

/** Signature matches execute_auth_query. */
typedef QueryResult *(*OidcRpRolesQueryFn)(int query_ref,
                                            const char *database,
                                            json_t *params);

/**
 * @brief Install a test seam to replace execute_auth_query.
 *
 * @param fn  Replacement function, or NULL to disable the override.
 */
void oidc_rp_roles_test_set_query_fn(OidcRpRolesQueryFn fn);

/**
 * @brief Remove the test seam (equivalent to set_query_fn(NULL)).
 */
void oidc_rp_roles_test_clear_query_fn(void);

/* ---------------------------------------------------------------------------
 * Internal helpers — NOT part of the stable public API. Exposed non-static
 * so Unity tests can call them directly. `RolesBuf` is the module's private
 * growable string builder (defined in oidc_rp_roles.c); forward-declared
 * here as opaque.
 * ------------------------------------------------------------------------- */
typedef struct RolesBuf RolesBuf;

QueryResult *run_query(int query_ref, const char *database, json_t *params);
bool roles_buf_init(RolesBuf *rb);
bool roles_buf_append(RolesBuf *rb, const char *role);
char *roles_buf_steal(RolesBuf *rb);
void roles_buf_free(RolesBuf *rb);
char *roles_from_idp(const OidcRpIdTokenClaims *claims, const char *prefix);
char **split_roles(const char *roles_str, size_t *out_count);
void free_roles_array(char **arr, size_t count);
char *roles_merge(const char *db_roles, const char *idp_roles);

#endif /* OIDC_RP_ROLES_H */
