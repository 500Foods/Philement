/*
 * OIDC Relying Party (RP) Configuration
 *
 * Defines the configuration structure and defaults for Hydrogen acting as an
 * OpenID Connect Relying Party against an external Identity Provider such as
 * Keycloak. This is **distinct** from the existing OIDCConfig
 * (config_oidc.{c,h}), which is for Hydrogen acting as an OIDC Provider.
 *
 * The RP flow terminates inside Hydrogen, where it mints a Hydrogen JWT that
 * is indistinguishable from a password-login JWT. The Lithium SPA only ever
 * holds the Hydrogen JWT; the IdP's id_token / access_token never leave
 * Hydrogen.
 *
 * Configuration covers:
 * - One or more upstream OIDC providers (currently 500passwords / Keycloak).
 * - Per-provider ClientId (public) + ClientSecret (sensitive, env-sourced).
 * - Discovery + JWKS cache TTLs.
 * - State store and handoff store TTLs.
 * - Account-linking strategy: match_sub_only, match_email_only,
 *   match_email_then_provision, provision_only.
 * - Role-mapping strategy: database, idp_realm_roles, idp_client_roles, merge.
 *
 * Validation is deferred to launch readiness checks (matching the convention
 * used by config_oidc.{c,h} and other config sections).
 */

#ifndef CONFIG_OIDC_RP_H
#define CONFIG_OIDC_RP_H

#include <stdbool.h>
#include <stddef.h>
#include <jansson.h>
#include "config_forward.h"  // For AppConfig forward declaration

// Hard cap on providers, mirroring the small fixed-array pattern used by
// other sections (e.g. database connections). The plan ships only one
// provider initially (500passwords) but the schema is array-shaped to
// support adding Google/GitHub/etc. without code churn.
#define OIDC_RP_MAX_PROVIDERS 8

// Hard cap on AllowedEmailDomains and DefaultRoleNames per provider. Both
// are small, operator-managed lists; the cap protects against malformed
// configs from exploding memory.
#define OIDC_RP_MAX_EMAIL_DOMAINS 16
#define OIDC_RP_MAX_DEFAULT_ROLES 16

// Account-linking strategy for resolving an IdP identity to an accounts row.
//
// MATCH_EMAIL_THEN_PROVISION (default): try (iss,sub) → match by verified
//   email → provision new account if enabled and allowed by domain list.
// MATCH_SUB_ONLY: trust (iss,sub) only; fail if not pre-linked.
// MATCH_EMAIL_ONLY: like above but never auto-create.
// PROVISION_ONLY: always create on first login (only safe with strict
//   AllowedEmailDomains).
typedef enum OIDCRPLinkStrategy {
    OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION = 0,
    OIDC_RP_LINK_MATCH_SUB_ONLY = 1,
    OIDC_RP_LINK_MATCH_EMAIL_ONLY = 2,
    OIDC_RP_LINK_PROVISION_ONLY = 3
} OIDCRPLinkStrategy;

// Source of role membership for the minted Hydrogen JWT.
//
// DATABASE (default): trust the accounts→account_roles join. IdP role
//   claims are ignored. Cleanest.
// IDP_REALM_ROLES: copy realm_access.roles from the IdP id_token.
// IDP_CLIENT_ROLES: copy resource_access.<client>.roles from the IdP id_token.
// MERGE: union of database + idp_realm_roles.
typedef enum OIDCRPRoleSource {
    OIDC_RP_ROLE_SRC_DATABASE = 0,
    OIDC_RP_ROLE_SRC_IDP_REALM_ROLES = 1,
    OIDC_RP_ROLE_SRC_IDP_CLIENT_ROLES = 2,
    OIDC_RP_ROLE_SRC_MERGE = 3
} OIDCRPRoleSource;

// Token-endpoint authentication method (RFC 6749 §2.3.1, §3.2.1).
//
// CLIENT_SECRET_BASIC (default): credentials in the HTTP Basic auth
//   header. URL-encoded per RFC 6749 §2.3.1 before base64.
// CLIENT_SECRET_POST: credentials passed as form parameters in the
//   request body. Less common; some IdPs require it.
typedef enum OIDCRPAuthMethod {
    OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC = 0,
    OIDC_RP_AUTH_METHOD_CLIENT_SECRET_POST = 1
} OIDCRPAuthMethod;

// Provisioning defaults applied when a new account is auto-created.
typedef struct OIDCRPProvisionDefaults {
    bool enabled;                     // Master switch for auto-provisioning
    bool authorized;                  // Initial accounts.authorized value
    char* default_role_names[OIDC_RP_MAX_DEFAULT_ROLES];
    size_t default_role_count;
} OIDCRPProvisionDefaults;

// Account-linking configuration. See OIDCRPLinkStrategy for semantics.
typedef struct OIDCRPAccountLinking {
    OIDCRPLinkStrategy strategy;
    OIDCRPProvisionDefaults provision_defaults;
    char* allowed_email_domains[OIDC_RP_MAX_EMAIL_DOMAINS];
    size_t allowed_email_domain_count;
    bool require_email_verified;
} OIDCRPAccountLinking;

// Role-mapping configuration.
//
// `idp_role_claim` is the JSON Pointer-style path within the id_token used
// for IDP_REALM_ROLES / MERGE strategies (default "realm_access.roles").
// `idp_role_prefix` is an optional prefix prepended to each IdP-sourced
// role in the JWT (e.g. "kc:") so admins can tell where a role came from.
typedef struct OIDCRPRoleMapping {
    OIDCRPRoleSource source;
    char* idp_role_claim;
    char* idp_role_prefix;
} OIDCRPRoleMapping;

// Per-provider configuration. The runtime keeps an array of these; index 0
// is the only provider initially configured (500passwords).
typedef struct OIDCRPProviderConfig {
    char* name;                       // Stable id, e.g. "500passwords"
    char* label;                      // UI display label, e.g. "500 Passwords"
    char* issuer;                     // Discovery base URL
    char* client_id;                  // PUBLIC — appears in browser-facing URLs
    char* client_secret;              // SENSITIVE — back-channel only
    char* redirect_uri;               // Configured Hydrogen callback URL
    char* scopes;                     // Space-separated, default "openid profile email"
    char* system_api_key;             // SENSITIVE — used to mint the Hydrogen JWT
    bool verify_ssl;                  // TLS verification (mandatory in prod)
    OIDCRPAuthMethod auth_method;     // Token-endpoint auth method (RFC 6749 §2.3.1)

    // Algorithm allow-list for ID-token signature verification.
    char* allowed_algorithms[8];
    size_t allowed_algorithm_count;

    // Cache + TTL knobs (seconds).
    int discovery_cache_seconds;
    int jwks_cache_seconds;
    int state_ttl_seconds;
    int handoff_ttl_seconds;
    bool bind_handoff_to_ip;

    OIDCRPAccountLinking account_linking;
    OIDCRPRoleMapping role_mapping;
} OIDCRPProviderConfig;

// Top-level OIDC RP configuration block.
//
// `enabled = false` is the default; the section is entirely optional in
// JSON. When disabled, the RP endpoints will return 503 (wired in
// Phase 6).
typedef struct OIDCRelyingPartyConfig {
    bool enabled;
    OIDCRPProviderConfig providers[OIDC_RP_MAX_PROVIDERS];
    size_t provider_count;
    char* database;                   // Default DB name for minted JWT (overridable per request)
} OIDCRelyingPartyConfig;

/*
 * Load OIDC RP configuration from JSON.
 *
 * Reads the optional top-level "OIDC_RP" section. Missing section yields
 * `enabled = false` and no providers; this is **not** an error.
 *
 * Sensitive values (ClientSecret, SystemApiKey) flow through the same
 * ${env.X} substitution mechanism as every other secret in Hydrogen.
 *
 * @param root JSON root object containing configuration
 * @param config Pointer to AppConfig structure to update
 * @return true on success, false on hard error (e.g. malformed types)
 */
bool load_oidc_rp_config(json_t* root, AppConfig* config);

/*
 * Free all dynamically allocated memory in an OIDCRelyingPartyConfig.
 * Safely handles NULL and partially-initialized configs.
 *
 * @param config Pointer to OIDCRelyingPartyConfig to clean up
 */
void cleanup_oidc_rp_config(OIDCRelyingPartyConfig* config);

/*
 * Dump the current state of the OIDC RP config to the configuration log.
 * Sensitive fields are redacted via DUMP_SECRET.
 *
 * @param config Pointer to OIDCRelyingPartyConfig to dump
 */
void dump_oidc_rp_config(const OIDCRelyingPartyConfig* config);

/*
 * Convert a link-strategy string to its enum value.
 * Unknown strings return OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION.
 */
OIDCRPLinkStrategy oidc_rp_parse_link_strategy(const char* str);

/*
 * Convert a role-source string to its enum value.
 * Unknown strings return OIDC_RP_ROLE_SRC_DATABASE.
 */
OIDCRPRoleSource oidc_rp_parse_role_source(const char* str);

/*
 * Convert an auth-method string to its enum value.
 * Unknown strings return OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC.
 */
OIDCRPAuthMethod oidc_rp_parse_auth_method(const char* str);

/*
 * Convert a link-strategy enum to a stable string (for logging / dump).
 */
const char* oidc_rp_link_strategy_name(OIDCRPLinkStrategy strategy);

/*
 * Convert a role-source enum to a stable string (for logging / dump).
 */
const char* oidc_rp_role_source_name(OIDCRPRoleSource source);

/*
 * Convert an auth-method enum to a stable string (for logging / dump).
 */
const char* oidc_rp_auth_method_name(OIDCRPAuthMethod method);

#endif /* CONFIG_OIDC_RP_H */
