/*
 * OIDC Relying Party (RP) Configuration Implementation
 *
 * Loads, dumps, and cleans up the OIDC_RP top-level config block. Distinct
 * from config_oidc.{c,h} (which is for Hydrogen-as-Provider).
 *
 * Validation is intentionally minimal here; it is performed at launch
 * readiness time, matching the convention of other sections.
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "config_oidc_rp.h"

// String tables for enum <-> name conversion. Using small static tables
// instead of macros because they are also referenced by the dump
// implementation.
static const char* k_link_strategy_names[] = {
    "match_email_then_provision",  // OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION
    "match_sub_only",              // OIDC_RP_LINK_MATCH_SUB_ONLY
    "match_email_only",            // OIDC_RP_LINK_MATCH_EMAIL_ONLY
    "provision_only"               // OIDC_RP_LINK_PROVISION_ONLY
};

static const char* k_role_source_names[] = {
    "database",                    // OIDC_RP_ROLE_SRC_DATABASE
    "idp_realm_roles",             // OIDC_RP_ROLE_SRC_IDP_REALM_ROLES
    "idp_client_roles",            // OIDC_RP_ROLE_SRC_IDP_CLIENT_ROLES
    "merge"                        // OIDC_RP_ROLE_SRC_MERGE
};

static const char* k_auth_method_names[] = {
    "client_secret_basic",         // OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC
    "client_secret_post"           // OIDC_RP_AUTH_METHOD_CLIENT_SECRET_POST
};

OIDCRPLinkStrategy oidc_rp_parse_link_strategy(const char* str) {
    if (!str) return OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION;
    for (size_t i = 0; i < sizeof(k_link_strategy_names) / sizeof(k_link_strategy_names[0]); i++) {
        if (strcmp(str, k_link_strategy_names[i]) == 0) {
            return (OIDCRPLinkStrategy)i;
        }
    }
    return OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION;
}

OIDCRPRoleSource oidc_rp_parse_role_source(const char* str) {
    if (!str) return OIDC_RP_ROLE_SRC_DATABASE;
    for (size_t i = 0; i < sizeof(k_role_source_names) / sizeof(k_role_source_names[0]); i++) {
        if (strcmp(str, k_role_source_names[i]) == 0) {
            return (OIDCRPRoleSource)i;
        }
    }
    return OIDC_RP_ROLE_SRC_DATABASE;
}

const char* oidc_rp_link_strategy_name(OIDCRPLinkStrategy strategy) {
    size_t idx = (size_t)strategy;
    if (idx >= sizeof(k_link_strategy_names) / sizeof(k_link_strategy_names[0])) {
        return "unknown";
    }
    return k_link_strategy_names[idx];
}

const char* oidc_rp_role_source_name(OIDCRPRoleSource source) {
    size_t idx = (size_t)source;
    if (idx >= sizeof(k_role_source_names) / sizeof(k_role_source_names[0])) {
        return "unknown";
    }
    return k_role_source_names[idx];
}

OIDCRPAuthMethod oidc_rp_parse_auth_method(const char* str) {
    if (!str) return OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC;
    for (size_t i = 0; i < sizeof(k_auth_method_names) / sizeof(k_auth_method_names[0]); i++) {
        if (strcmp(str, k_auth_method_names[i]) == 0) {
            return (OIDCRPAuthMethod)i;
        }
    }
    return OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC;
}

const char* oidc_rp_auth_method_name(OIDCRPAuthMethod method) {
    size_t idx = (size_t)method;
    if (idx >= sizeof(k_auth_method_names) / sizeof(k_auth_method_names[0])) {
        return "unknown";
    }
    return k_auth_method_names[idx];
}

// Resolve a JSON string value (with ${env.X} substitution) into a freshly
// allocated heap string. Returns NULL if the JSON node is missing or not a
// string. The caller owns the returned pointer.
static char* take_string_or_null(json_t* obj, const char* key) {
    if (!obj || !json_is_object(obj)) return NULL;
    json_t* node = json_object_get(obj, key);
    if (!node || !json_is_string(node)) return NULL;
    const char* raw = json_string_value(node);
    if (!raw) return NULL;
    char* resolved = process_env_variable_string(raw);
    if (resolved) return resolved;
    return strdup(raw);
}

// Read a boolean field with a default if missing or wrong type.
static bool take_bool_or_default(json_t* obj, const char* key, bool default_value) {
    if (!obj || !json_is_object(obj)) return default_value;
    json_t* node = json_object_get(obj, key);
    if (!node || !json_is_boolean(node)) return default_value;
    return json_boolean_value(node);
}

// Read an integer field with a default if missing or wrong type.
static int take_int_or_default(json_t* obj, const char* key, int default_value) {
    if (!obj || !json_is_object(obj)) return default_value;
    json_t* node = json_object_get(obj, key);
    if (!node || !json_is_integer(node)) return default_value;
    return (int)json_integer_value(node);
}

// Populate a fixed-capacity string array from a JSON array. Each entry is
// trimmed via process_env_variable_string. Existing entries (if any) are
// freed before being overwritten. Returns the number of entries stored.
static size_t take_string_array(json_t* obj, const char* key,
                                char** out_array, size_t capacity) {
    if (!obj || !json_is_object(obj) || !out_array || capacity == 0) return 0;
    json_t* node = json_object_get(obj, key);
    if (!node || !json_is_array(node)) return 0;

    size_t n = json_array_size(node);
    if (n > capacity) n = capacity;

    size_t stored = 0;
    for (size_t i = 0; i < n; i++) {
        json_t* item = json_array_get(node, i);
        if (!item || !json_is_string(item)) continue;
        const char* raw = json_string_value(item);
        if (!raw || !*raw) continue;
        char* resolved = process_env_variable_string(raw);
        out_array[stored] = resolved ? resolved : strdup(raw);
        if (out_array[stored]) {
            stored++;
        }
    }
    return stored;
}

// Apply default scalar values to a provider before JSON overrides.
static void provider_apply_defaults(OIDCRPProviderConfig* p) {
    p->scopes = strdup("openid profile email");
    p->verify_ssl = true;
    p->auth_method = OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC;
    p->discovery_cache_seconds = 3600;
    p->jwks_cache_seconds = 3600;
    p->state_ttl_seconds = 600;
    p->handoff_ttl_seconds = 60;
    p->bind_handoff_to_ip = true;

    // Default algorithm allow-list: RS256 only (Keycloak default).
    p->allowed_algorithms[0] = strdup("RS256");
    p->allowed_algorithm_count = 1;

    // Linking defaults: match_email_then_provision, email-verified required,
    // provisioning enabled with authorized=true. AllowedEmailDomains starts
    // empty; operators must set it explicitly when using provisioning.
    p->account_linking.strategy = OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION;
    p->account_linking.require_email_verified = true;
    p->account_linking.provision_defaults.enabled = true;
    p->account_linking.provision_defaults.authorized = true;
    p->account_linking.provision_defaults.default_role_count = 0;
    p->account_linking.allowed_email_domain_count = 0;

    // Role mapping defaults: trust the database; ignore IdP role claims.
    p->role_mapping.source = OIDC_RP_ROLE_SRC_DATABASE;
    p->role_mapping.idp_role_claim = strdup("realm_access.roles");
    p->role_mapping.idp_role_prefix = strdup("");
}

// Free everything inside a single provider. Does not touch the slot itself.
static void provider_cleanup(OIDCRPProviderConfig* p) {
    if (!p) return;
    free(p->name);
    free(p->label);
    free(p->issuer);
    free(p->client_id);
    free(p->client_secret);
    free(p->redirect_uri);
    free(p->scopes);
    free(p->system_api_key);

    for (size_t i = 0; i < p->allowed_algorithm_count; i++) {
        free(p->allowed_algorithms[i]);
    }
    p->allowed_algorithm_count = 0;

    free(p->role_mapping.idp_role_claim);
    free(p->role_mapping.idp_role_prefix);

    for (size_t i = 0; i < p->account_linking.provision_defaults.default_role_count; i++) {
        free(p->account_linking.provision_defaults.default_role_names[i]);
    }
    p->account_linking.provision_defaults.default_role_count = 0;

    for (size_t i = 0; i < p->account_linking.allowed_email_domain_count; i++) {
        free(p->account_linking.allowed_email_domains[i]);
    }
    p->account_linking.allowed_email_domain_count = 0;

    memset(p, 0, sizeof(*p));
}

// Load a single provider from its JSON object into slot `p`.
// `p` must already be zeroed by the caller (memset).
static void provider_load_from_json(OIDCRPProviderConfig* p, json_t* obj) {
    provider_apply_defaults(p);

    if (!obj || !json_is_object(obj)) return;

    // --- Identifiers ---
    char* v;
    if ((v = take_string_or_null(obj, "Name"))) {
        free(p->name); p->name = v;
    }
    if ((v = take_string_or_null(obj, "Label"))) {
        free(p->label); p->label = v;
    }
    if ((v = take_string_or_null(obj, "Issuer"))) {
        free(p->issuer); p->issuer = v;
    }
    if ((v = take_string_or_null(obj, "ClientId"))) {
        free(p->client_id); p->client_id = v;
    }
    if ((v = take_string_or_null(obj, "ClientSecret"))) {
        free(p->client_secret); p->client_secret = v;
    }
    if ((v = take_string_or_null(obj, "RedirectUri"))) {
        free(p->redirect_uri); p->redirect_uri = v;
    }
    if ((v = take_string_or_null(obj, "SystemApiKey"))) {
        free(p->system_api_key); p->system_api_key = v;
    }

    // --- Scalars with defaults ---
    if ((v = take_string_or_null(obj, "Scopes"))) {
        free(p->scopes); p->scopes = v;
    }
    p->verify_ssl = take_bool_or_default(obj, "VerifySsl", p->verify_ssl);

    // AuthMethod: optional. Default is client_secret_basic.
    char* am_str = take_string_or_null(obj, "AuthMethod");
    if (am_str) {
        p->auth_method = oidc_rp_parse_auth_method(am_str);
        free(am_str);
    }

    p->discovery_cache_seconds = take_int_or_default(obj, "DiscoveryCacheSeconds", p->discovery_cache_seconds);
    p->jwks_cache_seconds = take_int_or_default(obj, "JwksCacheSeconds", p->jwks_cache_seconds);
    p->state_ttl_seconds = take_int_or_default(obj, "StateTtlSeconds", p->state_ttl_seconds);
    p->handoff_ttl_seconds = take_int_or_default(obj, "HandoffTtlSeconds", p->handoff_ttl_seconds);
    p->bind_handoff_to_ip = take_bool_or_default(obj, "BindHandoffToIp", p->bind_handoff_to_ip);

    // --- Allowed algorithms (replace defaults if specified) ---
    json_t* algos = json_object_get(obj, "AllowedAlgorithms");
    if (algos && json_is_array(algos)) {
        // Free the default RS256 entry first.
        for (size_t i = 0; i < p->allowed_algorithm_count; i++) {
            free(p->allowed_algorithms[i]);
            p->allowed_algorithms[i] = NULL;
        }
        p->allowed_algorithm_count = 0;

        size_t cap = sizeof(p->allowed_algorithms) / sizeof(p->allowed_algorithms[0]);
        p->allowed_algorithm_count = take_string_array(
            obj, "AllowedAlgorithms", p->allowed_algorithms, cap);

        // If operator passed an empty array, fall back to RS256 to avoid
        // the impossible state of "no algorithms allowed".
        if (p->allowed_algorithm_count == 0) {
            p->allowed_algorithms[0] = strdup("RS256");
            p->allowed_algorithm_count = 1;
        }
    }

    // --- AccountLinking ---
    json_t* linking = json_object_get(obj, "AccountLinking");
    if (linking && json_is_object(linking)) {
        char* strat_str = take_string_or_null(linking, "Strategy");
        if (strat_str) {
            p->account_linking.strategy = oidc_rp_parse_link_strategy(strat_str);
            free(strat_str);
        }
        p->account_linking.require_email_verified = take_bool_or_default(
            linking, "RequireEmailVerified", p->account_linking.require_email_verified);

        json_t* prov = json_object_get(linking, "ProvisionDefaults");
        if (prov && json_is_object(prov)) {
            p->account_linking.provision_defaults.enabled = take_bool_or_default(
                prov, "Enabled", p->account_linking.provision_defaults.enabled);
            p->account_linking.provision_defaults.authorized = take_bool_or_default(
                prov, "Authorized", p->account_linking.provision_defaults.authorized);

            // Free any pre-existing role names before reading new ones.
            for (size_t i = 0; i < p->account_linking.provision_defaults.default_role_count; i++) {
                free(p->account_linking.provision_defaults.default_role_names[i]);
                p->account_linking.provision_defaults.default_role_names[i] = NULL;
            }
            p->account_linking.provision_defaults.default_role_count = 0;
            p->account_linking.provision_defaults.default_role_count = take_string_array(
                prov, "DefaultRoleNames",
                p->account_linking.provision_defaults.default_role_names,
                OIDC_RP_MAX_DEFAULT_ROLES);
        }

        // Free any pre-existing domains before reading.
        for (size_t i = 0; i < p->account_linking.allowed_email_domain_count; i++) {
            free(p->account_linking.allowed_email_domains[i]);
            p->account_linking.allowed_email_domains[i] = NULL;
        }
        p->account_linking.allowed_email_domain_count = 0;
        p->account_linking.allowed_email_domain_count = take_string_array(
            linking, "AllowedEmailDomains",
            p->account_linking.allowed_email_domains,
            OIDC_RP_MAX_EMAIL_DOMAINS);
    }

    // --- RoleMapping ---
    json_t* roles = json_object_get(obj, "RoleMapping");
    if (roles && json_is_object(roles)) {
        char* src_str = take_string_or_null(roles, "Source");
        if (src_str) {
            p->role_mapping.source = oidc_rp_parse_role_source(src_str);
            free(src_str);
        }
        if ((v = take_string_or_null(roles, "IdpRoleClaim"))) {
            free(p->role_mapping.idp_role_claim);
            p->role_mapping.idp_role_claim = v;
        }
        // IdpRolePrefix is allowed to be the empty string explicitly.
        json_t* prefix_node = json_object_get(roles, "IdpRolePrefix");
        if (prefix_node && json_is_string(prefix_node)) {
            const char* raw = json_string_value(prefix_node);
            free(p->role_mapping.idp_role_prefix);
            char* resolved = process_env_variable_string(raw);
            p->role_mapping.idp_role_prefix = resolved ? resolved : strdup(raw ? raw : "");
        }
    }
}

bool load_oidc_rp_config(json_t* root, AppConfig* config) {
    if (!config) {
        log_this(SR_AUTH, "OIDC_RP: cannot load into NULL config", LOG_LEVEL_ERROR, 0);
        return false;
    }

    OIDCRelyingPartyConfig* rp = &config->oidc_rp;

    // Defaults: feature off, no providers, no default database.
    // (initialize_config_defaults_oidc_rp also handles this; we re-apply
    // here so load_oidc_rp_config remains correct even if invoked
    // independently.)
    if (rp->provider_count == 0 && !rp->database) {
        memset(rp, 0, sizeof(*rp));
        rp->enabled = false;
        rp->database = NULL;
    }

    // Missing root or missing OIDC_RP section is not an error — just keep
    // the disabled defaults.
    if (!root || !json_is_object(root)) {
        return true;
    }

    json_t* section = json_object_get(root, "OIDC_RP");
    if (!section || !json_is_object(section)) {
        return true;
    }

    // Tell the section header logger about us, like the other loaders do.
    PROCESS_SECTION(root, "OIDC_RP");

    rp->enabled = take_bool_or_default(section, "Enabled", false);

    char* db = take_string_or_null(section, "Database");
    if (db) {
        free(rp->database);
        rp->database = db;
    }

    json_t* providers = json_object_get(section, "Providers");
    if (providers && json_is_array(providers)) {
        size_t n = json_array_size(providers);
        if (n > OIDC_RP_MAX_PROVIDERS) {
            log_this(SR_AUTH,
                "OIDC_RP: %zu providers configured, truncating to %d",
                LOG_LEVEL_ALERT, 2, n, OIDC_RP_MAX_PROVIDERS);
            n = OIDC_RP_MAX_PROVIDERS;
        }

        // Free any previously-loaded providers (rare: e.g. config reload).
        for (size_t i = 0; i < rp->provider_count; i++) {
            provider_cleanup(&rp->providers[i]);
        }
        rp->provider_count = 0;

        for (size_t i = 0; i < n; i++) {
            json_t* pobj = json_array_get(providers, i);
            if (!pobj || !json_is_object(pobj)) continue;
            memset(&rp->providers[rp->provider_count], 0,
                   sizeof(rp->providers[0]));
            provider_load_from_json(&rp->providers[rp->provider_count], pobj);
            rp->provider_count++;
        }
    }

    return true;
}

void cleanup_oidc_rp_config(OIDCRelyingPartyConfig* config) {
    if (!config) return;

    for (size_t i = 0; i < config->provider_count; i++) {
        provider_cleanup(&config->providers[i]);
    }
    config->provider_count = 0;

    free(config->database);
    config->database = NULL;

    config->enabled = false;
    memset(config, 0, sizeof(*config));
}

void dump_oidc_rp_config(const OIDCRelyingPartyConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL OIDC_RP config");
        return;
    }

    DUMP_BOOL("Enabled", config->enabled);
    DUMP_STRING("Database", config->database);
    DUMP_SIZE("Provider Count", config->provider_count);

    for (size_t i = 0; i < config->provider_count; i++) {
        const OIDCRPProviderConfig* p = &config->providers[i];

        DUMP_TEXT("――", "Provider");
        DUMP_STRING("――――Name", p->name);
        DUMP_STRING("――――Label", p->label);
        DUMP_STRING("――――Issuer", p->issuer);
        DUMP_STRING("――――Client ID", p->client_id);
        DUMP_SECRET("――――Client Secret", p->client_secret);
        DUMP_STRING("――――Redirect URI", p->redirect_uri);
        DUMP_STRING("――――Scopes", p->scopes);
        DUMP_SECRET("――――System API Key", p->system_api_key);
        DUMP_BOOL("――――Verify SSL", p->verify_ssl);
        DUMP_STRING("――――Auth Method", oidc_rp_auth_method_name(p->auth_method));
        DUMP_INT("――――Discovery Cache Seconds", p->discovery_cache_seconds);
        DUMP_INT("――――JWKS Cache Seconds", p->jwks_cache_seconds);
        DUMP_INT("――――State TTL Seconds", p->state_ttl_seconds);
        DUMP_INT("――――Handoff TTL Seconds", p->handoff_ttl_seconds);
        DUMP_BOOL("――――Bind Handoff To IP", p->bind_handoff_to_ip);

        DUMP_TEXT("――――", "Allowed Algorithms");
        for (size_t j = 0; j < p->allowed_algorithm_count; j++) {
            DUMP_STRING("――――――Algorithm", p->allowed_algorithms[j]);
        }

        DUMP_TEXT("――――", "Account Linking");
        DUMP_STRING("――――――Strategy", oidc_rp_link_strategy_name(p->account_linking.strategy));
        DUMP_BOOL("――――――Require Email Verified", p->account_linking.require_email_verified);
        DUMP_BOOL("――――――Provisioning Enabled", p->account_linking.provision_defaults.enabled);
        DUMP_BOOL("――――――Provisioning Authorized", p->account_linking.provision_defaults.authorized);
        for (size_t j = 0; j < p->account_linking.provision_defaults.default_role_count; j++) {
            DUMP_STRING("――――――Default Role", p->account_linking.provision_defaults.default_role_names[j]);
        }
        for (size_t j = 0; j < p->account_linking.allowed_email_domain_count; j++) {
            DUMP_STRING("――――――Allowed Email Domain", p->account_linking.allowed_email_domains[j]);
        }

        DUMP_TEXT("――――", "Role Mapping");
        DUMP_STRING("――――――Source", oidc_rp_role_source_name(p->role_mapping.source));
        DUMP_STRING("――――――IdP Role Claim", p->role_mapping.idp_role_claim);
        DUMP_STRING("――――――IdP Role Prefix", p->role_mapping.idp_role_prefix);
    }
}
