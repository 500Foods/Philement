/*
 * Unity Test File: OIDC Relying Party Configuration Tests
 *
 * Covers load_oidc_rp_config, cleanup_oidc_rp_config, dump_oidc_rp_config,
 * and the small enum parser/name helpers from src/config/config_oidc_rp.c.
 *
 * Hard rules verified by these tests:
 *   - Missing OIDC_RP section -> defaults (Enabled=false, no providers).
 *   - All sensitive fields flow through ${env.X} substitution.
 *   - Provider truncation when more than OIDC_RP_MAX_PROVIDERS are listed.
 *   - cleanup is leak-free under ASAN (verified by test_11_leaks).
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_oidc_rp.h>
#include <src/config/config.h>

// Forward declarations of functions under test.
bool load_oidc_rp_config(json_t* root, AppConfig* config);
void cleanup_oidc_rp_config(OIDCRelyingPartyConfig* config);
void dump_oidc_rp_config(const OIDCRelyingPartyConfig* config);
OIDCRPLinkStrategy oidc_rp_parse_link_strategy(const char* str);
OIDCRPRoleSource oidc_rp_parse_role_source(const char* str);
OIDCRPAuthMethod oidc_rp_parse_auth_method(const char* str);
const char* oidc_rp_link_strategy_name(OIDCRPLinkStrategy strategy);
const char* oidc_rp_role_source_name(OIDCRPRoleSource source);
const char* oidc_rp_auth_method_name(OIDCRPAuthMethod method);

// Forward declarations of test functions.
void test_oidc_rp_null_root_uses_defaults(void);
void test_oidc_rp_missing_section_uses_defaults(void);
void test_oidc_rp_null_config_returns_false(void);
void test_oidc_rp_disabled_with_no_providers(void);
void test_oidc_rp_full_provider_loaded(void);
void test_oidc_rp_provider_defaults_when_partial(void);
void test_oidc_rp_truncates_too_many_providers(void);
void test_oidc_rp_skips_non_object_provider(void);
void test_oidc_rp_role_mapping_loaded(void);
void test_oidc_rp_account_linking_loaded(void);
void test_oidc_rp_env_var_substitution_for_secret(void);
void test_oidc_rp_empty_allowed_algorithms_falls_back_to_rs256(void);
void test_oidc_rp_cleanup_null_safe(void);
void test_oidc_rp_cleanup_empty_safe(void);
void test_oidc_rp_dump_null_safe(void);
void test_oidc_rp_dump_with_data(void);
void test_oidc_rp_parse_link_strategy_known(void);
void test_oidc_rp_parse_link_strategy_unknown_falls_back(void);
void test_oidc_rp_parse_role_source_known(void);
void test_oidc_rp_parse_role_source_unknown_falls_back(void);
void test_oidc_rp_link_strategy_name_known(void);
void test_oidc_rp_link_strategy_name_out_of_range(void);
void test_oidc_rp_role_source_name_known(void);
void test_oidc_rp_role_source_name_out_of_range(void);
void test_oidc_rp_auth_method_default_is_basic(void);
void test_oidc_rp_auth_method_loaded_from_json(void);
void test_oidc_rp_auth_method_unknown_falls_back_to_basic(void);
void test_oidc_rp_parse_auth_method_known(void);
void test_oidc_rp_parse_auth_method_unknown_falls_back(void);
void test_oidc_rp_auth_method_name_known(void);
void test_oidc_rp_auth_method_name_out_of_range(void);

void setUp(void) {
    // Some tests poke at HYDROGEN_OIDC_CLIENT_SECRET — make sure we start
    // from a known clean state.
    unsetenv("HYDROGEN_OIDC_CLIENT_SECRET_TEST");
}

void tearDown(void) {
    unsetenv("HYDROGEN_OIDC_CLIENT_SECRET_TEST");
}

// ---------------------------------------------------------------------------
// Top-level loader behaviour
// ---------------------------------------------------------------------------

void test_oidc_rp_null_root_uses_defaults(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));

    bool result = load_oidc_rp_config(NULL, &config);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.oidc_rp.enabled);
    TEST_ASSERT_EQUAL_size_t(0, config.oidc_rp.provider_count);
    TEST_ASSERT_NULL(config.oidc_rp.database);

    cleanup_oidc_rp_config(&config.oidc_rp);
}

void test_oidc_rp_missing_section_uses_defaults(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));
    json_t* root = json_object();

    bool result = load_oidc_rp_config(root, &config);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.oidc_rp.enabled);
    TEST_ASSERT_EQUAL_size_t(0, config.oidc_rp.provider_count);

    json_decref(root);
    cleanup_oidc_rp_config(&config.oidc_rp);
}

void test_oidc_rp_null_config_returns_false(void) {
    bool result = load_oidc_rp_config(NULL, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_oidc_rp_disabled_with_no_providers(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));
    json_t* root = json_object();
    json_t* section = json_object();
    json_object_set_new(section, "Enabled", json_false());
    json_object_set_new(section, "Database", json_string("Lithium"));
    json_object_set_new(root, "OIDC_RP", section);

    bool result = load_oidc_rp_config(root, &config);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.oidc_rp.enabled);
    TEST_ASSERT_EQUAL_STRING("Lithium", config.oidc_rp.database);
    TEST_ASSERT_EQUAL_size_t(0, config.oidc_rp.provider_count);

    json_decref(root);
    cleanup_oidc_rp_config(&config.oidc_rp);
}

// ---------------------------------------------------------------------------
// Provider loading
// ---------------------------------------------------------------------------

// Build a fully-populated provider JSON object for reuse across tests.
static json_t* make_full_provider_json(void) {
    json_t* p = json_object();
    json_object_set_new(p, "Name", json_string("500passwords"));
    json_object_set_new(p, "Label", json_string("500 Passwords"));
    json_object_set_new(p, "Issuer", json_string("https://www.500passwords.com/realms/philement"));
    json_object_set_new(p, "ClientId", json_string("lithium-web"));
    json_object_set_new(p, "ClientSecret", json_string("super-secret"));
    json_object_set_new(p, "RedirectUri", json_string("https://hydrogen.example.com/api/auth/oidc/callback"));
    json_object_set_new(p, "Scopes", json_string("openid profile email roles"));
    json_object_set_new(p, "SystemApiKey", json_string("sys-api-key"));
    json_object_set_new(p, "VerifySsl", json_true());
    json_object_set_new(p, "AuthMethod", json_string("client_secret_post"));
    json_object_set_new(p, "DiscoveryCacheSeconds", json_integer(7200));
    json_object_set_new(p, "JwksCacheSeconds", json_integer(7200));
    json_object_set_new(p, "StateTtlSeconds", json_integer(900));
    json_object_set_new(p, "HandoffTtlSeconds", json_integer(120));
    json_object_set_new(p, "BindHandoffToIp", json_false());

    json_t* algos = json_array();
    json_array_append_new(algos, json_string("RS256"));
    json_array_append_new(algos, json_string("RS512"));
    json_object_set_new(p, "AllowedAlgorithms", algos);

    // AccountLinking
    json_t* linking = json_object();
    json_object_set_new(linking, "Strategy", json_string("match_email_then_provision"));
    json_object_set_new(linking, "RequireEmailVerified", json_true());

    json_t* prov = json_object();
    json_object_set_new(prov, "Enabled", json_true());
    json_object_set_new(prov, "Authorized", json_true());
    json_t* roles = json_array();
    json_array_append_new(roles, json_string("user"));
    json_array_append_new(roles, json_string("viewer"));
    json_object_set_new(prov, "DefaultRoleNames", roles);
    json_object_set_new(linking, "ProvisionDefaults", prov);

    json_t* domains = json_array();
    json_array_append_new(domains, json_string("philement.com"));
    json_array_append_new(domains, json_string("500passwords.com"));
    json_object_set_new(linking, "AllowedEmailDomains", domains);

    json_object_set_new(p, "AccountLinking", linking);

    // RoleMapping
    json_t* rm = json_object();
    json_object_set_new(rm, "Source", json_string("merge"));
    json_object_set_new(rm, "IdpRoleClaim", json_string("realm_access.roles"));
    json_object_set_new(rm, "IdpRolePrefix", json_string("kc:"));
    json_object_set_new(p, "RoleMapping", rm);

    return p;
}

void test_oidc_rp_full_provider_loaded(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));

    json_t* root = json_object();
    json_t* section = json_object();
    json_object_set_new(section, "Enabled", json_true());
    json_object_set_new(section, "Database", json_string("Lithium"));

    json_t* providers = json_array();
    json_array_append_new(providers, make_full_provider_json());
    json_object_set_new(section, "Providers", providers);

    json_object_set_new(root, "OIDC_RP", section);

    bool result = load_oidc_rp_config(root, &config);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.oidc_rp.enabled);
    TEST_ASSERT_EQUAL_STRING("Lithium", config.oidc_rp.database);
    TEST_ASSERT_EQUAL_size_t(1, config.oidc_rp.provider_count);

    OIDCRPProviderConfig* p = &config.oidc_rp.providers[0];
    TEST_ASSERT_EQUAL_STRING("500passwords", p->name);
    TEST_ASSERT_EQUAL_STRING("500 Passwords", p->label);
    TEST_ASSERT_EQUAL_STRING("https://www.500passwords.com/realms/philement", p->issuer);
    TEST_ASSERT_EQUAL_STRING("lithium-web", p->client_id);
    TEST_ASSERT_EQUAL_STRING("super-secret", p->client_secret);
    TEST_ASSERT_EQUAL_STRING("https://hydrogen.example.com/api/auth/oidc/callback", p->redirect_uri);
    TEST_ASSERT_EQUAL_STRING("openid profile email roles", p->scopes);
    TEST_ASSERT_EQUAL_STRING("sys-api-key", p->system_api_key);
    TEST_ASSERT_TRUE(p->verify_ssl);
    TEST_ASSERT_EQUAL_INT(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_POST, p->auth_method);
    TEST_ASSERT_EQUAL_INT(7200, p->discovery_cache_seconds);
    TEST_ASSERT_EQUAL_INT(7200, p->jwks_cache_seconds);
    TEST_ASSERT_EQUAL_INT(900, p->state_ttl_seconds);
    TEST_ASSERT_EQUAL_INT(120, p->handoff_ttl_seconds);
    TEST_ASSERT_FALSE(p->bind_handoff_to_ip);

    TEST_ASSERT_EQUAL_size_t(2, p->allowed_algorithm_count);
    TEST_ASSERT_EQUAL_STRING("RS256", p->allowed_algorithms[0]);
    TEST_ASSERT_EQUAL_STRING("RS512", p->allowed_algorithms[1]);

    TEST_ASSERT_EQUAL_INT(OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION,
                          p->account_linking.strategy);
    TEST_ASSERT_TRUE(p->account_linking.require_email_verified);
    TEST_ASSERT_TRUE(p->account_linking.provision_defaults.enabled);
    TEST_ASSERT_TRUE(p->account_linking.provision_defaults.authorized);
    TEST_ASSERT_EQUAL_size_t(2, p->account_linking.provision_defaults.default_role_count);
    TEST_ASSERT_EQUAL_STRING("user", p->account_linking.provision_defaults.default_role_names[0]);
    TEST_ASSERT_EQUAL_STRING("viewer", p->account_linking.provision_defaults.default_role_names[1]);
    TEST_ASSERT_EQUAL_size_t(2, p->account_linking.allowed_email_domain_count);
    TEST_ASSERT_EQUAL_STRING("philement.com", p->account_linking.allowed_email_domains[0]);
    TEST_ASSERT_EQUAL_STRING("500passwords.com", p->account_linking.allowed_email_domains[1]);

    TEST_ASSERT_EQUAL_INT(OIDC_RP_ROLE_SRC_MERGE, p->role_mapping.source);
    TEST_ASSERT_EQUAL_STRING("realm_access.roles", p->role_mapping.idp_role_claim);
    TEST_ASSERT_EQUAL_STRING("kc:", p->role_mapping.idp_role_prefix);

    json_decref(root);
    cleanup_oidc_rp_config(&config.oidc_rp);
}

void test_oidc_rp_provider_defaults_when_partial(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));

    json_t* root = json_object();
    json_t* section = json_object();
    json_object_set_new(section, "Enabled", json_true());

    // Provider with only Name + ClientId — every other field should be the
    // default established in provider_apply_defaults.
    json_t* providers = json_array();
    json_t* p = json_object();
    json_object_set_new(p, "Name", json_string("minimal"));
    json_object_set_new(p, "ClientId", json_string("c1"));
    json_array_append_new(providers, p);
    json_object_set_new(section, "Providers", providers);
    json_object_set_new(root, "OIDC_RP", section);

    bool result = load_oidc_rp_config(root, &config);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_size_t(1, config.oidc_rp.provider_count);

    OIDCRPProviderConfig* prov = &config.oidc_rp.providers[0];
    TEST_ASSERT_EQUAL_STRING("minimal", prov->name);
    TEST_ASSERT_EQUAL_STRING("c1", prov->client_id);
    TEST_ASSERT_NULL(prov->client_secret);
    TEST_ASSERT_EQUAL_STRING("openid profile email", prov->scopes);
    TEST_ASSERT_TRUE(prov->verify_ssl);
    TEST_ASSERT_EQUAL_INT(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC, prov->auth_method);
    TEST_ASSERT_EQUAL_INT(3600, prov->discovery_cache_seconds);
    TEST_ASSERT_EQUAL_INT(3600, prov->jwks_cache_seconds);
    TEST_ASSERT_EQUAL_INT(600, prov->state_ttl_seconds);
    TEST_ASSERT_EQUAL_INT(60, prov->handoff_ttl_seconds);
    TEST_ASSERT_TRUE(prov->bind_handoff_to_ip);
    TEST_ASSERT_EQUAL_size_t(1, prov->allowed_algorithm_count);
    TEST_ASSERT_EQUAL_STRING("RS256", prov->allowed_algorithms[0]);
    TEST_ASSERT_EQUAL_INT(OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION,
                          prov->account_linking.strategy);
    TEST_ASSERT_TRUE(prov->account_linking.require_email_verified);
    TEST_ASSERT_EQUAL_INT(OIDC_RP_ROLE_SRC_DATABASE, prov->role_mapping.source);
    TEST_ASSERT_EQUAL_STRING("realm_access.roles", prov->role_mapping.idp_role_claim);
    TEST_ASSERT_EQUAL_STRING("", prov->role_mapping.idp_role_prefix);

    json_decref(root);
    cleanup_oidc_rp_config(&config.oidc_rp);
}

void test_oidc_rp_truncates_too_many_providers(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));

    json_t* root = json_object();
    json_t* section = json_object();
    json_object_set_new(section, "Enabled", json_true());

    json_t* providers = json_array();
    for (int i = 0; i < OIDC_RP_MAX_PROVIDERS + 3; i++) {
        json_t* p = json_object();
        char name[32];
        snprintf(name, sizeof(name), "p%d", i);
        json_object_set_new(p, "Name", json_string(name));
        json_array_append_new(providers, p);
    }
    json_object_set_new(section, "Providers", providers);
    json_object_set_new(root, "OIDC_RP", section);

    bool result = load_oidc_rp_config(root, &config);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_size_t((size_t)OIDC_RP_MAX_PROVIDERS,
                             config.oidc_rp.provider_count);

    json_decref(root);
    cleanup_oidc_rp_config(&config.oidc_rp);
}

void test_oidc_rp_skips_non_object_provider(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));

    json_t* root = json_object();
    json_t* section = json_object();
    json_object_set_new(section, "Enabled", json_true());

    json_t* providers = json_array();
    json_array_append_new(providers, json_string("garbage"));
    json_array_append_new(providers, make_full_provider_json());
    json_array_append_new(providers, json_integer(42));
    json_object_set_new(section, "Providers", providers);
    json_object_set_new(root, "OIDC_RP", section);

    bool result = load_oidc_rp_config(root, &config);
    TEST_ASSERT_TRUE(result);
    // Only the one valid object should remain.
    TEST_ASSERT_EQUAL_size_t(1, config.oidc_rp.provider_count);
    TEST_ASSERT_EQUAL_STRING("500passwords", config.oidc_rp.providers[0].name);

    json_decref(root);
    cleanup_oidc_rp_config(&config.oidc_rp);
}

void test_oidc_rp_role_mapping_loaded(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));

    json_t* root = json_object();
    json_t* section = json_object();
    json_object_set_new(section, "Enabled", json_true());
    json_t* providers = json_array();
    json_t* p = json_object();
    json_object_set_new(p, "Name", json_string("p"));

    json_t* rm = json_object();
    json_object_set_new(rm, "Source", json_string("idp_realm_roles"));
    json_object_set_new(rm, "IdpRoleClaim", json_string("custom.claim"));
    json_object_set_new(rm, "IdpRolePrefix", json_string(""));
    json_object_set_new(p, "RoleMapping", rm);

    json_array_append_new(providers, p);
    json_object_set_new(section, "Providers", providers);
    json_object_set_new(root, "OIDC_RP", section);

    bool result = load_oidc_rp_config(root, &config);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(OIDC_RP_ROLE_SRC_IDP_REALM_ROLES,
                          config.oidc_rp.providers[0].role_mapping.source);
    TEST_ASSERT_EQUAL_STRING("custom.claim",
                             config.oidc_rp.providers[0].role_mapping.idp_role_claim);
    TEST_ASSERT_EQUAL_STRING("",
                             config.oidc_rp.providers[0].role_mapping.idp_role_prefix);

    json_decref(root);
    cleanup_oidc_rp_config(&config.oidc_rp);
}

void test_oidc_rp_account_linking_loaded(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));

    json_t* root = json_object();
    json_t* section = json_object();
    json_object_set_new(section, "Enabled", json_true());
    json_t* providers = json_array();
    json_t* p = json_object();
    json_object_set_new(p, "Name", json_string("p"));

    json_t* linking = json_object();
    json_object_set_new(linking, "Strategy", json_string("match_sub_only"));
    json_object_set_new(linking, "RequireEmailVerified", json_false());
    json_object_set_new(p, "AccountLinking", linking);
    json_array_append_new(providers, p);
    json_object_set_new(section, "Providers", providers);
    json_object_set_new(root, "OIDC_RP", section);

    bool result = load_oidc_rp_config(root, &config);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(OIDC_RP_LINK_MATCH_SUB_ONLY,
                          config.oidc_rp.providers[0].account_linking.strategy);
    TEST_ASSERT_FALSE(config.oidc_rp.providers[0].account_linking.require_email_verified);

    json_decref(root);
    cleanup_oidc_rp_config(&config.oidc_rp);
}

void test_oidc_rp_env_var_substitution_for_secret(void) {
    setenv("HYDROGEN_OIDC_CLIENT_SECRET_TEST", "actual-secret-value", 1);

    AppConfig config;
    memset(&config, 0, sizeof(config));

    json_t* root = json_object();
    json_t* section = json_object();
    json_object_set_new(section, "Enabled", json_true());

    json_t* providers = json_array();
    json_t* p = json_object();
    json_object_set_new(p, "Name", json_string("envtest"));
    json_object_set_new(p, "ClientSecret", json_string("${env.HYDROGEN_OIDC_CLIENT_SECRET_TEST}"));
    json_array_append_new(providers, p);
    json_object_set_new(section, "Providers", providers);
    json_object_set_new(root, "OIDC_RP", section);

    bool result = load_oidc_rp_config(root, &config);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("actual-secret-value",
                             config.oidc_rp.providers[0].client_secret);

    json_decref(root);
    cleanup_oidc_rp_config(&config.oidc_rp);
}

void test_oidc_rp_empty_allowed_algorithms_falls_back_to_rs256(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));

    json_t* root = json_object();
    json_t* section = json_object();
    json_object_set_new(section, "Enabled", json_true());

    json_t* providers = json_array();
    json_t* p = json_object();
    json_object_set_new(p, "Name", json_string("p"));
    // Empty AllowedAlgorithms array — operator mistake; loader must
    // refuse to leave us with zero allowed algorithms.
    json_object_set_new(p, "AllowedAlgorithms", json_array());
    json_array_append_new(providers, p);
    json_object_set_new(section, "Providers", providers);
    json_object_set_new(root, "OIDC_RP", section);

    bool result = load_oidc_rp_config(root, &config);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_size_t(1, config.oidc_rp.providers[0].allowed_algorithm_count);
    TEST_ASSERT_EQUAL_STRING("RS256",
                             config.oidc_rp.providers[0].allowed_algorithms[0]);

    json_decref(root);
    cleanup_oidc_rp_config(&config.oidc_rp);
}

// ---------------------------------------------------------------------------
// Cleanup + dump
// ---------------------------------------------------------------------------

void test_oidc_rp_cleanup_null_safe(void) {
    cleanup_oidc_rp_config(NULL);  // must not crash
}

void test_oidc_rp_cleanup_empty_safe(void) {
    OIDCRelyingPartyConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cleanup_oidc_rp_config(&cfg);  // must not crash
    TEST_ASSERT_FALSE(cfg.enabled);
    TEST_ASSERT_EQUAL_size_t(0, cfg.provider_count);
}

void test_oidc_rp_dump_null_safe(void) {
    dump_oidc_rp_config(NULL);  // must not crash
}

void test_oidc_rp_dump_with_data(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));

    json_t* root = json_object();
    json_t* section = json_object();
    json_object_set_new(section, "Enabled", json_true());
    json_t* providers = json_array();
    json_array_append_new(providers, make_full_provider_json());
    json_object_set_new(section, "Providers", providers);
    json_object_set_new(root, "OIDC_RP", section);

    TEST_ASSERT_TRUE(load_oidc_rp_config(root, &config));
    dump_oidc_rp_config(&config.oidc_rp);  // must not crash

    json_decref(root);
    cleanup_oidc_rp_config(&config.oidc_rp);
}

// ---------------------------------------------------------------------------
// Enum helpers
// ---------------------------------------------------------------------------

void test_oidc_rp_parse_link_strategy_known(void) {
    TEST_ASSERT_EQUAL_INT(OIDC_RP_LINK_MATCH_SUB_ONLY,
                          oidc_rp_parse_link_strategy("match_sub_only"));
    TEST_ASSERT_EQUAL_INT(OIDC_RP_LINK_MATCH_EMAIL_ONLY,
                          oidc_rp_parse_link_strategy("match_email_only"));
    TEST_ASSERT_EQUAL_INT(OIDC_RP_LINK_PROVISION_ONLY,
                          oidc_rp_parse_link_strategy("provision_only"));
    TEST_ASSERT_EQUAL_INT(OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION,
                          oidc_rp_parse_link_strategy("match_email_then_provision"));
}

void test_oidc_rp_parse_link_strategy_unknown_falls_back(void) {
    TEST_ASSERT_EQUAL_INT(OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION,
                          oidc_rp_parse_link_strategy("garbage"));
    TEST_ASSERT_EQUAL_INT(OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION,
                          oidc_rp_parse_link_strategy(NULL));
}

void test_oidc_rp_parse_role_source_known(void) {
    TEST_ASSERT_EQUAL_INT(OIDC_RP_ROLE_SRC_DATABASE,
                          oidc_rp_parse_role_source("database"));
    TEST_ASSERT_EQUAL_INT(OIDC_RP_ROLE_SRC_IDP_REALM_ROLES,
                          oidc_rp_parse_role_source("idp_realm_roles"));
    TEST_ASSERT_EQUAL_INT(OIDC_RP_ROLE_SRC_IDP_CLIENT_ROLES,
                          oidc_rp_parse_role_source("idp_client_roles"));
    TEST_ASSERT_EQUAL_INT(OIDC_RP_ROLE_SRC_MERGE,
                          oidc_rp_parse_role_source("merge"));
}

void test_oidc_rp_parse_role_source_unknown_falls_back(void) {
    TEST_ASSERT_EQUAL_INT(OIDC_RP_ROLE_SRC_DATABASE,
                          oidc_rp_parse_role_source("garbage"));
    TEST_ASSERT_EQUAL_INT(OIDC_RP_ROLE_SRC_DATABASE,
                          oidc_rp_parse_role_source(NULL));
}

void test_oidc_rp_link_strategy_name_known(void) {
    TEST_ASSERT_EQUAL_STRING("match_email_then_provision",
                             oidc_rp_link_strategy_name(OIDC_RP_LINK_MATCH_EMAIL_THEN_PROVISION));
    TEST_ASSERT_EQUAL_STRING("match_sub_only",
                             oidc_rp_link_strategy_name(OIDC_RP_LINK_MATCH_SUB_ONLY));
    TEST_ASSERT_EQUAL_STRING("match_email_only",
                             oidc_rp_link_strategy_name(OIDC_RP_LINK_MATCH_EMAIL_ONLY));
    TEST_ASSERT_EQUAL_STRING("provision_only",
                             oidc_rp_link_strategy_name(OIDC_RP_LINK_PROVISION_ONLY));
}

void test_oidc_rp_link_strategy_name_out_of_range(void) {
    TEST_ASSERT_EQUAL_STRING("unknown", oidc_rp_link_strategy_name((OIDCRPLinkStrategy)999));
}

void test_oidc_rp_role_source_name_known(void) {
    TEST_ASSERT_EQUAL_STRING("database",
                             oidc_rp_role_source_name(OIDC_RP_ROLE_SRC_DATABASE));
    TEST_ASSERT_EQUAL_STRING("merge",
                             oidc_rp_role_source_name(OIDC_RP_ROLE_SRC_MERGE));
}

void test_oidc_rp_role_source_name_out_of_range(void) {
    TEST_ASSERT_EQUAL_STRING("unknown", oidc_rp_role_source_name((OIDCRPRoleSource)999));
}

// ---------------------------------------------------------------------------
// AuthMethod (Phase 11)
// ---------------------------------------------------------------------------

void test_oidc_rp_auth_method_default_is_basic(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));

    json_t* root = json_object();
    json_t* section = json_object();
    json_object_set_new(section, "Enabled", json_true());

    json_t* providers = json_array();
    json_t* p = json_object();
    json_object_set_new(p, "Name", json_string("p"));
    // Deliberately omit AuthMethod to verify default.
    json_array_append_new(providers, p);
    json_object_set_new(section, "Providers", providers);
    json_object_set_new(root, "OIDC_RP", section);

    TEST_ASSERT_TRUE(load_oidc_rp_config(root, &config));
    TEST_ASSERT_EQUAL_INT(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC,
                          config.oidc_rp.providers[0].auth_method);

    json_decref(root);
    cleanup_oidc_rp_config(&config.oidc_rp);
}

void test_oidc_rp_auth_method_loaded_from_json(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));

    json_t* root = json_object();
    json_t* section = json_object();
    json_object_set_new(section, "Enabled", json_true());

    json_t* providers = json_array();
    json_t* p = json_object();
    json_object_set_new(p, "Name", json_string("p"));
    json_object_set_new(p, "AuthMethod", json_string("client_secret_post"));
    json_array_append_new(providers, p);
    json_object_set_new(section, "Providers", providers);
    json_object_set_new(root, "OIDC_RP", section);

    TEST_ASSERT_TRUE(load_oidc_rp_config(root, &config));
    TEST_ASSERT_EQUAL_INT(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_POST,
                          config.oidc_rp.providers[0].auth_method);

    json_decref(root);
    cleanup_oidc_rp_config(&config.oidc_rp);
}

void test_oidc_rp_auth_method_unknown_falls_back_to_basic(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));

    json_t* root = json_object();
    json_t* section = json_object();
    json_object_set_new(section, "Enabled", json_true());

    json_t* providers = json_array();
    json_t* p = json_object();
    json_object_set_new(p, "Name", json_string("p"));
    json_object_set_new(p, "AuthMethod", json_string("private_key_jwt_unsupported"));
    json_array_append_new(providers, p);
    json_object_set_new(section, "Providers", providers);
    json_object_set_new(root, "OIDC_RP", section);

    TEST_ASSERT_TRUE(load_oidc_rp_config(root, &config));
    TEST_ASSERT_EQUAL_INT(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC,
                          config.oidc_rp.providers[0].auth_method);

    json_decref(root);
    cleanup_oidc_rp_config(&config.oidc_rp);
}

void test_oidc_rp_parse_auth_method_known(void) {
    TEST_ASSERT_EQUAL_INT(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC,
                          oidc_rp_parse_auth_method("client_secret_basic"));
    TEST_ASSERT_EQUAL_INT(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_POST,
                          oidc_rp_parse_auth_method("client_secret_post"));
}

void test_oidc_rp_parse_auth_method_unknown_falls_back(void) {
    TEST_ASSERT_EQUAL_INT(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC,
                          oidc_rp_parse_auth_method("garbage"));
    TEST_ASSERT_EQUAL_INT(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC,
                          oidc_rp_parse_auth_method(NULL));
}

void test_oidc_rp_auth_method_name_known(void) {
    TEST_ASSERT_EQUAL_STRING("client_secret_basic",
                             oidc_rp_auth_method_name(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC));
    TEST_ASSERT_EQUAL_STRING("client_secret_post",
                             oidc_rp_auth_method_name(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_POST));
}

void test_oidc_rp_auth_method_name_out_of_range(void) {
    TEST_ASSERT_EQUAL_STRING("unknown", oidc_rp_auth_method_name((OIDCRPAuthMethod)999));
}

// ---------------------------------------------------------------------------
// Test runner
// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_oidc_rp_null_root_uses_defaults);
    RUN_TEST(test_oidc_rp_missing_section_uses_defaults);
    RUN_TEST(test_oidc_rp_null_config_returns_false);
    RUN_TEST(test_oidc_rp_disabled_with_no_providers);

    RUN_TEST(test_oidc_rp_full_provider_loaded);
    RUN_TEST(test_oidc_rp_provider_defaults_when_partial);
    RUN_TEST(test_oidc_rp_truncates_too_many_providers);
    RUN_TEST(test_oidc_rp_skips_non_object_provider);
    RUN_TEST(test_oidc_rp_role_mapping_loaded);
    RUN_TEST(test_oidc_rp_account_linking_loaded);
    RUN_TEST(test_oidc_rp_env_var_substitution_for_secret);
    RUN_TEST(test_oidc_rp_empty_allowed_algorithms_falls_back_to_rs256);

    RUN_TEST(test_oidc_rp_cleanup_null_safe);
    RUN_TEST(test_oidc_rp_cleanup_empty_safe);
    RUN_TEST(test_oidc_rp_dump_null_safe);
    RUN_TEST(test_oidc_rp_dump_with_data);

    RUN_TEST(test_oidc_rp_parse_link_strategy_known);
    RUN_TEST(test_oidc_rp_parse_link_strategy_unknown_falls_back);
    RUN_TEST(test_oidc_rp_parse_role_source_known);
    RUN_TEST(test_oidc_rp_parse_role_source_unknown_falls_back);
    RUN_TEST(test_oidc_rp_link_strategy_name_known);
    RUN_TEST(test_oidc_rp_link_strategy_name_out_of_range);
    RUN_TEST(test_oidc_rp_role_source_name_known);
    RUN_TEST(test_oidc_rp_role_source_name_out_of_range);

    RUN_TEST(test_oidc_rp_auth_method_default_is_basic);
    RUN_TEST(test_oidc_rp_auth_method_loaded_from_json);
    RUN_TEST(test_oidc_rp_auth_method_unknown_falls_back_to_basic);
    RUN_TEST(test_oidc_rp_parse_auth_method_known);
    RUN_TEST(test_oidc_rp_parse_auth_method_unknown_falls_back);
    RUN_TEST(test_oidc_rp_auth_method_name_known);
    RUN_TEST(test_oidc_rp_auth_method_name_out_of_range);

    return UNITY_END();
}
