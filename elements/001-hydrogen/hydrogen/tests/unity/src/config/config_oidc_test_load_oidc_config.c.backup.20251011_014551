/*
 * Unity Test File: OIDC Configuration Tests
 * This file contains unit tests for the load_oidc_config function
 * from src/config/config_oidc.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/config/config_oidc.h"
#include "../../../../src/config/config.h"

// Forward declarations for functions being tested
bool load_oidc_config(json_t* root, AppConfig* config);
void cleanup_oidc_config(OIDCConfig* config);
void dump_oidc_config(const OIDCConfig* config);

// Forward declarations for test functions
void test_load_oidc_config_null_root(void);
void test_load_oidc_config_empty_json(void);
void test_load_oidc_config_basic_fields(void);
void test_load_oidc_config_endpoints(void);
void test_load_oidc_config_keys(void);
void test_load_oidc_config_tokens(void);
void test_cleanup_oidc_config_null_pointer(void);
void test_cleanup_oidc_config_empty_config(void);
void test_cleanup_oidc_config_with_data(void);
void test_dump_oidc_config_null_pointer(void);
void test_dump_oidc_config_basic(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_oidc_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_oidc_config(NULL, &config);

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.oidc.enabled);  // Default is enabled
    TEST_ASSERT_EQUAL(8443, config.oidc.port);  // Default port
    TEST_ASSERT_EQUAL_STRING("client_secret_basic", config.oidc.auth_method);  // Default auth method
    TEST_ASSERT_EQUAL_STRING("openid profile email", config.oidc.scope);  // Default scope

    cleanup_oidc_config(&config.oidc);
}

void test_load_oidc_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_oidc_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.oidc.enabled);  // Default is enabled
    TEST_ASSERT_EQUAL(8443, config.oidc.port);  // Default port
    TEST_ASSERT_TRUE(config.oidc.verify_ssl);  // Default is true
    TEST_ASSERT_EQUAL(3600, config.oidc.tokens.access_token_lifetime);  // Default lifetime
    TEST_ASSERT_EQUAL_STRING("RS256", config.oidc.tokens.signing_alg);  // Default algorithm

    json_decref(root);
    cleanup_oidc_config(&config.oidc);
}

// ===== BASIC FIELD TESTS =====

void test_load_oidc_config_basic_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* oidc_section = json_object();

    // Set up basic OIDC configuration
    json_object_set(oidc_section, "Enabled", json_false());
    json_object_set(oidc_section, "Issuer", json_string("https://auth.example.com"));
    json_object_set(oidc_section, "ClientId", json_string("test-client"));
    json_object_set(oidc_section, "ClientSecret", json_string("secret-key"));
    json_object_set(oidc_section, "RedirectUri", json_string("https://app.example.com/callback"));
    json_object_set(oidc_section, "Port", json_integer(8080));
    json_object_set(oidc_section, "AuthMethod", json_string("client_secret_post"));
    json_object_set(oidc_section, "Scope", json_string("openid email"));
    json_object_set(oidc_section, "VerifySSL", json_false());

    json_object_set(root, "OIDC", oidc_section);

    bool result = load_oidc_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.oidc.enabled);
    TEST_ASSERT_EQUAL_STRING("https://auth.example.com", config.oidc.issuer);
    TEST_ASSERT_EQUAL_STRING("test-client", config.oidc.client_id);
    TEST_ASSERT_EQUAL_STRING("secret-key", config.oidc.client_secret);
    TEST_ASSERT_EQUAL_STRING("https://app.example.com/callback", config.oidc.redirect_uri);
    TEST_ASSERT_EQUAL(8080, config.oidc.port);
    TEST_ASSERT_EQUAL_STRING("client_secret_post", config.oidc.auth_method);
    TEST_ASSERT_EQUAL_STRING("openid email", config.oidc.scope);
    TEST_ASSERT_FALSE(config.oidc.verify_ssl);

    json_decref(root);
    cleanup_oidc_config(&config.oidc);
}

// ===== ENDPOINTS TESTS =====

void test_load_oidc_config_endpoints(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* oidc_section = json_object();
    json_t* endpoints_section = json_object();

    // Set up endpoints configuration
    json_object_set(endpoints_section, "Authorization", json_string("/auth"));
    json_object_set(endpoints_section, "Token", json_string("/token"));
    json_object_set(endpoints_section, "UserInfo", json_string("/userinfo"));
    json_object_set(endpoints_section, "JWKS", json_string("/jwks"));
    json_object_set(endpoints_section, "EndSession", json_string("/logout"));
    json_object_set(endpoints_section, "Introspection", json_string("/introspect"));
    json_object_set(endpoints_section, "Revocation", json_string("/revoke"));
    json_object_set(endpoints_section, "Registration", json_string("/register"));

    json_object_set(oidc_section, "Endpoints", endpoints_section);
    json_object_set(root, "OIDC", oidc_section);

    bool result = load_oidc_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/auth", config.oidc.endpoints.authorization);
    TEST_ASSERT_EQUAL_STRING("/token", config.oidc.endpoints.token);
    TEST_ASSERT_EQUAL_STRING("/userinfo", config.oidc.endpoints.userinfo);
    TEST_ASSERT_EQUAL_STRING("/jwks", config.oidc.endpoints.jwks);
    TEST_ASSERT_EQUAL_STRING("/logout", config.oidc.endpoints.end_session);
    TEST_ASSERT_EQUAL_STRING("/introspect", config.oidc.endpoints.introspection);
    TEST_ASSERT_EQUAL_STRING("/revoke", config.oidc.endpoints.revocation);
    TEST_ASSERT_EQUAL_STRING("/register", config.oidc.endpoints.registration);

    json_decref(root);
    cleanup_oidc_config(&config.oidc);
}

// ===== KEYS TESTS =====

void test_load_oidc_config_keys(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* oidc_section = json_object();
    json_t* keys_section = json_object();

    // Set up keys configuration
    json_object_set(keys_section, "SigningKey", json_string("signing-key-data"));
    json_object_set(keys_section, "EncryptionKey", json_string("encryption-key-data"));
    json_object_set(keys_section, "JWKSUri", json_string("https://example.com/jwks"));
    json_object_set(keys_section, "StoragePath", json_string("/var/keys"));
    json_object_set(keys_section, "EncryptionEnabled", json_false());
    json_object_set(keys_section, "RotationIntervalDays", json_integer(60));

    json_object_set(oidc_section, "Keys", keys_section);
    json_object_set(root, "OIDC", oidc_section);

    bool result = load_oidc_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("signing-key-data", config.oidc.keys.signing_key);
    TEST_ASSERT_EQUAL_STRING("encryption-key-data", config.oidc.keys.encryption_key);
    TEST_ASSERT_EQUAL_STRING("https://example.com/jwks", config.oidc.keys.jwks_uri);
    TEST_ASSERT_EQUAL_STRING("/var/keys", config.oidc.keys.storage_path);
    TEST_ASSERT_FALSE(config.oidc.keys.encryption_enabled);
    TEST_ASSERT_EQUAL(60, config.oidc.keys.rotation_interval_days);

    json_decref(root);
    cleanup_oidc_config(&config.oidc);
}

// ===== TOKENS TESTS =====

void test_load_oidc_config_tokens(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* oidc_section = json_object();
    json_t* tokens_section = json_object();

    // Set up tokens configuration
    json_object_set(tokens_section, "AccessTokenLifetime", json_integer(1800));
    json_object_set(tokens_section, "RefreshTokenLifetime", json_integer(604800));
    json_object_set(tokens_section, "IdTokenLifetime", json_integer(1800));
    json_object_set(tokens_section, "SigningAlg", json_string("ES256"));
    json_object_set(tokens_section, "EncryptionAlg", json_string("A128CBC-HS256"));

    json_object_set(oidc_section, "Tokens", tokens_section);
    json_object_set(root, "OIDC", oidc_section);

    bool result = load_oidc_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1800, config.oidc.tokens.access_token_lifetime);
    TEST_ASSERT_EQUAL(604800, config.oidc.tokens.refresh_token_lifetime);
    TEST_ASSERT_EQUAL(1800, config.oidc.tokens.id_token_lifetime);
    TEST_ASSERT_EQUAL_STRING("ES256", config.oidc.tokens.signing_alg);
    TEST_ASSERT_EQUAL_STRING("A128CBC-HS256", config.oidc.tokens.encryption_alg);

    json_decref(root);
    cleanup_oidc_config(&config.oidc);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_oidc_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_oidc_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_oidc_config_empty_config(void) {
    OIDCConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_oidc_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enabled);
    TEST_ASSERT_NULL(config.issuer);
    TEST_ASSERT_NULL(config.client_id);
    TEST_ASSERT_NULL(config.endpoints.authorization);
    TEST_ASSERT_NULL(config.keys.signing_key);
    TEST_ASSERT_NULL(config.tokens.signing_alg);
}

void test_cleanup_oidc_config_with_data(void) {
    OIDCConfig config = {0};

    // Initialize with some test data
    config.enabled = true;
    config.issuer = strdup("https://auth.example.com");
    config.client_id = strdup("test-client");
    config.client_secret = strdup("secret");
    config.redirect_uri = strdup("https://app.example.com/callback");
    config.auth_method = strdup("client_secret_basic");
    config.scope = strdup("openid profile email");

    // Initialize endpoints
    config.endpoints.authorization = strdup("/authorize");
    config.endpoints.token = strdup("/token");
    config.endpoints.userinfo = strdup("/userinfo");

    // Initialize keys
    config.keys.signing_key = strdup("signing-key");
    config.keys.encryption_key = strdup("encryption-key");
    config.keys.jwks_uri = strdup("https://example.com/jwks");
    config.keys.storage_path = strdup("/var/keys");

    // Initialize tokens
    config.tokens.signing_alg = strdup("RS256");
    config.tokens.encryption_alg = strdup("A256GCM");

    // Cleanup should free all allocated memory
    cleanup_oidc_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enabled);
    TEST_ASSERT_NULL(config.issuer);
    TEST_ASSERT_NULL(config.client_id);
    TEST_ASSERT_NULL(config.endpoints.authorization);
    TEST_ASSERT_NULL(config.keys.signing_key);
    TEST_ASSERT_NULL(config.tokens.signing_alg);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_oidc_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_oidc_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_oidc_config_basic(void) {
    OIDCConfig config = {0};

    // Initialize with test data
    config.enabled = true;
    config.issuer = strdup("https://auth.example.com");
    config.client_id = strdup("test-client");
    config.port = 8443;
    config.auth_method = strdup("client_secret_basic");
    config.scope = strdup("openid profile email");
    config.verify_ssl = true;

    // Initialize endpoints
    config.endpoints.authorization = strdup("/authorize");
    config.endpoints.token = strdup("/token");

    // Initialize keys
    config.keys.signing_key = strdup("signing-key");
    config.keys.storage_path = strdup("/var/keys");
    config.keys.encryption_enabled = true;
    config.keys.rotation_interval_days = 30;

    // Initialize tokens
    config.tokens.access_token_lifetime = 3600;
    config.tokens.signing_alg = strdup("RS256");

    // Dump should not crash and handle the data properly
    dump_oidc_config(&config);

    // Clean up
    cleanup_oidc_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_oidc_config_null_root);
    RUN_TEST(test_load_oidc_config_empty_json);

    // Basic field tests
    RUN_TEST(test_load_oidc_config_basic_fields);
    RUN_TEST(test_load_oidc_config_endpoints);
    RUN_TEST(test_load_oidc_config_keys);
    RUN_TEST(test_load_oidc_config_tokens);

    // Cleanup function tests
    RUN_TEST(test_cleanup_oidc_config_null_pointer);
    RUN_TEST(test_cleanup_oidc_config_empty_config);
    RUN_TEST(test_cleanup_oidc_config_with_data);

    // Dump function tests
    RUN_TEST(test_dump_oidc_config_null_pointer);
    RUN_TEST(test_dump_oidc_config_basic);

    return UNITY_END();
}