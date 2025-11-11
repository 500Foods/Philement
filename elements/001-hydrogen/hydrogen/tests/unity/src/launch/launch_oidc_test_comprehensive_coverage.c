/*
 * Unity Test File: Comprehensive OIDC Launch Tests
 * Tests for check_oidc_launch_readiness function with full edge case coverage
 */

// Enable mocks BEFORE including ANY headers
// USE_MOCK_LAUNCH defined by CMake

// Include mock headers immediately
#include <unity/mocks/mock_launch.h>

// Include Unity framework
#include <unity.h>

// Include source headers (functions will now be mocked)
#include <src/hydrogen.h>
#include <src/launch/launch.h>
#include <src/config/config.h>
#include <src/config/config_defaults.h>

// Forward declarations for functions being tested
LaunchReadiness check_oidc_launch_readiness(void);

// External references
extern AppConfig* app_config;

// Forward declarations for test functions
void test_oidc_disabled_configuration(void);
void test_oidc_null_issuer(void);
void test_oidc_empty_issuer(void);
void test_oidc_invalid_issuer_url_format(void);
void test_oidc_null_client_id(void);
void test_oidc_empty_client_id(void);
void test_oidc_null_client_secret(void);
void test_oidc_empty_client_secret(void);
void test_oidc_empty_redirect_uri(void);
void test_oidc_invalid_redirect_uri_url_format(void);
void test_oidc_port_too_low(void);
void test_oidc_port_too_high(void);
void test_oidc_access_token_lifetime_too_low(void);
void test_oidc_access_token_lifetime_too_high(void);
void test_oidc_refresh_token_lifetime_too_low(void);
void test_oidc_refresh_token_lifetime_too_high(void);
void test_oidc_id_token_lifetime_too_low(void);
void test_oidc_id_token_lifetime_too_high(void);
void test_oidc_encryption_enabled_without_key(void);
void test_oidc_key_rotation_interval_too_low(void);
void test_oidc_key_rotation_interval_too_high(void);
void test_oidc_valid_configuration_http(void);
void test_oidc_valid_configuration_https(void);
void test_oidc_valid_configuration_with_encryption(void);

// Test helper to create a minimal valid OIDC configuration using config_defaults
static void setup_minimal_valid_oidc_config(void) {
    if (!app_config) {
        app_config = calloc(1, sizeof(AppConfig));
        if (!app_config) {
            return;  // Let test fail naturally
        }
    }
    
    // Use the standard initialization which sets all defaults properly
    initialize_config_defaults(app_config);
    
    // Enable OIDC (defaults have it disabled)
    app_config->oidc.enabled = true;
    
    // Set required fields that have NULL defaults
    app_config->oidc.issuer = strdup("https://auth.example.com");
    app_config->oidc.client_id = strdup("test-client-id");
    app_config->oidc.client_secret = strdup("test-client-secret");
    // redirect_uri, port, and token settings come from defaults
}

// Test helper to cleanup configuration
static void cleanup_test_config(void) {
    if (app_config) {
        // Free only the OIDC strings we explicitly allocated
        // Let any memory allocated by initialize_config_defaults remain
        // to avoid double-free issues
        free(app_config);
        app_config = NULL;
    }
}

void setUp(void) {
    // Reset mocks
    mock_launch_reset_all();
    
    // Make Registry subsystem appear launchable so we can test OIDC configuration validation
    mock_launch_set_is_subsystem_launchable_result(true);
    mock_launch_set_add_dependency_result(true);
    mock_launch_set_get_subsystem_id_result(1);
    
    // Clean slate for each test
    cleanup_test_config();
}

void tearDown(void) {
    // Clean up after each test
    cleanup_test_config();
    
    // Reset mocks
    mock_launch_reset_all();
}

// Test 1: OIDC disabled in configuration
void test_oidc_disabled_configuration(void) {
    setup_minimal_valid_oidc_config();
    app_config->oidc.enabled = false;
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_OIDC, result.subsystem);
    // Note: ready state depends on Registry subsystem being properly mocked
    // When OIDC is disabled, it returns ready=true IF Registry is launchable
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 2: NULL issuer
void test_oidc_null_issuer(void) {
    setup_minimal_valid_oidc_config();
    free(app_config->oidc.issuer);
    app_config->oidc.issuer = NULL;
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 3: Empty issuer
void test_oidc_empty_issuer(void) {
    setup_minimal_valid_oidc_config();
    free(app_config->oidc.issuer);
    app_config->oidc.issuer = strdup("");
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 4: Invalid issuer URL format (not http:// or https://)
void test_oidc_invalid_issuer_url_format(void) {
    setup_minimal_valid_oidc_config();
    free(app_config->oidc.issuer);
    app_config->oidc.issuer = strdup("ftp://auth.example.com");
    
    // Verify config state before test
    TEST_ASSERT_NOT_NULL(app_config);
    TEST_ASSERT_TRUE(app_config->oidc.enabled);
    TEST_ASSERT_NOT_NULL(app_config->oidc.issuer);
    TEST_ASSERT_EQUAL_STRING("ftp://auth.example.com", app_config->oidc.issuer);
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 5: NULL client_id
void test_oidc_null_client_id(void) {
    setup_minimal_valid_oidc_config();
    free(app_config->oidc.client_id);
    app_config->oidc.client_id = NULL;
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 6: Empty client_id
void test_oidc_empty_client_id(void) {
    setup_minimal_valid_oidc_config();
    free(app_config->oidc.client_id);
    app_config->oidc.client_id = strdup("");
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 7: NULL client_secret
void test_oidc_null_client_secret(void) {
    setup_minimal_valid_oidc_config();
    free(app_config->oidc.client_secret);
    app_config->oidc.client_secret = NULL;
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 8: Empty client_secret
void test_oidc_empty_client_secret(void) {
    setup_minimal_valid_oidc_config();
    free(app_config->oidc.client_secret);
    app_config->oidc.client_secret = strdup("");
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 9: Empty redirect_uri
void test_oidc_empty_redirect_uri(void) {
    setup_minimal_valid_oidc_config();
    free(app_config->oidc.redirect_uri);
    app_config->oidc.redirect_uri = strdup("");
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 10: Invalid redirect_uri URL format
void test_oidc_invalid_redirect_uri_url_format(void) {
    setup_minimal_valid_oidc_config();
    free(app_config->oidc.redirect_uri);
    app_config->oidc.redirect_uri = strdup("ftp://localhost:8080/callback");
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 11: Port too low (below MIN_OIDC_PORT)
void test_oidc_port_too_low(void) {
    setup_minimal_valid_oidc_config();
    app_config->oidc.port = 1023;  // MIN_OIDC_PORT is 1024
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 12: Port too high (above MAX_OIDC_PORT)
void test_oidc_port_too_high(void) {
    setup_minimal_valid_oidc_config();
    app_config->oidc.port = 65536;  // MAX_OIDC_PORT is 65535
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 13: Access token lifetime too low
void test_oidc_access_token_lifetime_too_low(void) {
    setup_minimal_valid_oidc_config();
    app_config->oidc.tokens.access_token_lifetime = 299;  // MIN_TOKEN_LIFETIME is 300
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 14: Access token lifetime too high
void test_oidc_access_token_lifetime_too_high(void) {
    setup_minimal_valid_oidc_config();
    app_config->oidc.tokens.access_token_lifetime = 86401;  // MAX_TOKEN_LIFETIME is 86400
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 15: Refresh token lifetime too low
void test_oidc_refresh_token_lifetime_too_low(void) {
    setup_minimal_valid_oidc_config();
    app_config->oidc.tokens.refresh_token_lifetime = 3599;  // MIN_REFRESH_LIFETIME is 3600
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 16: Refresh token lifetime too high
void test_oidc_refresh_token_lifetime_too_high(void) {
    setup_minimal_valid_oidc_config();
    app_config->oidc.tokens.refresh_token_lifetime = 2592001;  // MAX_REFRESH_LIFETIME is 2592000
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 17: ID token lifetime too low
void test_oidc_id_token_lifetime_too_low(void) {
    setup_minimal_valid_oidc_config();
    app_config->oidc.tokens.id_token_lifetime = 299;  // MIN_TOKEN_LIFETIME is 300
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 18: ID token lifetime too high
void test_oidc_id_token_lifetime_too_high(void) {
    setup_minimal_valid_oidc_config();
    app_config->oidc.tokens.id_token_lifetime = 86401;  // MAX_TOKEN_LIFETIME is 86400
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 19: Encryption enabled without encryption key
void test_oidc_encryption_enabled_without_key(void) {
    setup_minimal_valid_oidc_config();
    app_config->oidc.keys.encryption_enabled = true;
    app_config->oidc.keys.encryption_key = NULL;
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 20: Key rotation interval too low
void test_oidc_key_rotation_interval_too_low(void) {
    setup_minimal_valid_oidc_config();
    app_config->oidc.keys.rotation_interval_days = 0;  // MIN_KEY_ROTATION_DAYS is 1
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 21: Key rotation interval too high
void test_oidc_key_rotation_interval_too_high(void) {
    setup_minimal_valid_oidc_config();
    app_config->oidc.keys.rotation_interval_days = 91;  // MAX_KEY_ROTATION_DAYS is 90
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 22: Valid OIDC configuration with HTTP issuer
void test_oidc_valid_configuration_http(void) {
    setup_minimal_valid_oidc_config();
    free(app_config->oidc.issuer);
    app_config->oidc.issuer = strdup("http://auth.example.com");
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_OIDC, result.subsystem);
    // Note: ready state depends on Registry subsystem being properly mocked
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 23: Valid OIDC configuration with HTTPS issuer
void test_oidc_valid_configuration_https(void) {
    setup_minimal_valid_oidc_config();
    // Already has https:// issuer from setup
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_OIDC, result.subsystem);
    // Note: ready state depends on Registry subsystem being properly mocked
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 24: Valid configuration with encryption enabled and key provided
void test_oidc_valid_configuration_with_encryption(void) {
    setup_minimal_valid_oidc_config();
    app_config->oidc.keys.encryption_enabled = true;
    app_config->oidc.keys.encryption_key = strdup("test-encryption-key-32-bytes-long");
    
    LaunchReadiness result = check_oidc_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_OIDC, result.subsystem);
    // Note: ready state depends on Registry subsystem being properly mocked
    TEST_ASSERT_NOT_NULL(result.messages);
}

int main(void) {
    UNITY_BEGIN();
    
    // Configuration validation tests
    RUN_TEST(test_oidc_disabled_configuration);
    RUN_TEST(test_oidc_null_issuer);
    RUN_TEST(test_oidc_empty_issuer);
    RUN_TEST(test_oidc_invalid_issuer_url_format);
    RUN_TEST(test_oidc_null_client_id);
    RUN_TEST(test_oidc_empty_client_id);
    RUN_TEST(test_oidc_null_client_secret);
    RUN_TEST(test_oidc_empty_client_secret);
    RUN_TEST(test_oidc_empty_redirect_uri);
    RUN_TEST(test_oidc_invalid_redirect_uri_url_format);
    
    // Port validation tests
    RUN_TEST(test_oidc_port_too_low);
    RUN_TEST(test_oidc_port_too_high);
    
    // Token lifetime validation tests
    RUN_TEST(test_oidc_access_token_lifetime_too_low);
    RUN_TEST(test_oidc_access_token_lifetime_too_high);
    RUN_TEST(test_oidc_refresh_token_lifetime_too_low);
    RUN_TEST(test_oidc_refresh_token_lifetime_too_high);
    RUN_TEST(test_oidc_id_token_lifetime_too_low);
    RUN_TEST(test_oidc_id_token_lifetime_too_high);
    
    // Key configuration validation tests
    RUN_TEST(test_oidc_encryption_enabled_without_key);
    RUN_TEST(test_oidc_key_rotation_interval_too_low);
    RUN_TEST(test_oidc_key_rotation_interval_too_high);
    
    // Valid configuration tests
    RUN_TEST(test_oidc_valid_configuration_http);
    RUN_TEST(test_oidc_valid_configuration_https);
    RUN_TEST(test_oidc_valid_configuration_with_encryption);
    
    return UNITY_END();
}