/*
 * Unity Test File: OIDC Validation Helper Functions Tests
 * This file tests the extracted validation helper functions for comprehensive coverage
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>

// Forward declarations for helper functions being tested
bool validate_oidc_issuer(const char* issuer, const char*** messages, size_t* count, size_t* capacity, bool* ready);
bool validate_oidc_redirect_uri(const char* redirect_uri, const char*** messages, size_t* count, size_t* capacity, bool* ready);
bool validate_oidc_port(int port, const char*** messages, size_t* count, size_t* capacity, bool* ready);
bool validate_oidc_token_lifetimes(const OIDCTokensConfig* tokens, const char*** messages, size_t* count, size_t* capacity, bool* ready);
bool validate_oidc_key_settings(const OIDCKeysConfig* keys, const char*** messages, size_t* count, size_t* capacity, bool* ready);

// Test helper to create message arrays
static void setup_message_array(const char*** messages, size_t* count, size_t* capacity) {
    *messages = NULL;
    *count = 0;
    *capacity = 0;
}

// Test helper to cleanup message arrays
static void cleanup_message_array(const char*** messages, size_t count) {
    if (*messages) {
        for (size_t i = 0; i < count; i++) {
            free((void*)(*messages)[i]);
        }
        free(*messages);
        *messages = NULL;
    }
}

// Forward declarations for all test functions
void test_validate_oidc_issuer_null(void);
void test_validate_oidc_issuer_empty(void);
void test_validate_oidc_issuer_invalid_scheme(void);
void test_validate_oidc_issuer_valid_http(void);
void test_validate_oidc_issuer_valid_https(void);
void test_validate_oidc_redirect_uri_null(void);
void test_validate_oidc_redirect_uri_empty(void);
void test_validate_oidc_redirect_uri_invalid_scheme(void);
void test_validate_oidc_redirect_uri_valid(void);
void test_validate_oidc_port_too_low(void);
void test_validate_oidc_port_too_high(void);
void test_validate_oidc_port_valid(void);
void test_validate_oidc_token_lifetimes_access_too_low(void);
void test_validate_oidc_token_lifetimes_refresh_too_high(void);
void test_validate_oidc_token_lifetimes_id_too_high(void);
void test_validate_oidc_token_lifetimes_valid(void);
void test_validate_oidc_key_settings_encryption_without_key(void);
void test_validate_oidc_key_settings_rotation_too_low(void);
void test_validate_oidc_key_settings_rotation_too_high(void);
void test_validate_oidc_key_settings_valid(void);

void setUp(void) {
    // Set up test fixtures
}

void tearDown(void) {
    // Clean up test fixtures
}

// Tests for validate_oidc_issuer
void test_validate_oidc_issuer_null(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_issuer(NULL, &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(ready);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    cleanup_message_array(&messages, count);
}

void test_validate_oidc_issuer_empty(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_issuer("", &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(ready);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    cleanup_message_array(&messages, count);
}

void test_validate_oidc_issuer_invalid_scheme(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_issuer("ftp://auth.example.com", &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(ready);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    cleanup_message_array(&messages, count);
}

void test_validate_oidc_issuer_valid_http(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_issuer("http://auth.example.com", &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(ready);
    
    cleanup_message_array(&messages, count);
}

void test_validate_oidc_issuer_valid_https(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_issuer("https://auth.example.com", &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(ready);
    
    cleanup_message_array(&messages, count);
}

// Tests for validate_oidc_redirect_uri
void test_validate_oidc_redirect_uri_null(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_redirect_uri(NULL, &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_TRUE(result);  // NULL is valid (optional field)
    TEST_ASSERT_TRUE(ready);
    
    cleanup_message_array(&messages, count);
}

void test_validate_oidc_redirect_uri_empty(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_redirect_uri("", &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(ready);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    cleanup_message_array(&messages, count);
}

void test_validate_oidc_redirect_uri_invalid_scheme(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_redirect_uri("ftp://localhost/callback", &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(ready);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    cleanup_message_array(&messages, count);
}

void test_validate_oidc_redirect_uri_valid(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_redirect_uri("http://localhost:8080/callback", &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(ready);
    
    cleanup_message_array(&messages, count);
}

// Tests for validate_oidc_port
void test_validate_oidc_port_too_low(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_port(1023, &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(ready);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    cleanup_message_array(&messages, count);
}

void test_validate_oidc_port_too_high(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_port(65536, &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(ready);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    cleanup_message_array(&messages, count);
}

void test_validate_oidc_port_valid(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_port(8080, &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(ready);
    
    cleanup_message_array(&messages, count);
}

// Tests for validate_oidc_token_lifetimes
void test_validate_oidc_token_lifetimes_access_too_low(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    OIDCTokensConfig tokens = {
        .access_token_lifetime = 299,  // Below MIN_TOKEN_LIFETIME (300)
        .refresh_token_lifetime = 86400,
        .id_token_lifetime = 3600
    };
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_token_lifetimes(&tokens, &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(ready);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    cleanup_message_array(&messages, count);
}

void test_validate_oidc_token_lifetimes_refresh_too_high(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    OIDCTokensConfig tokens = {
        .access_token_lifetime = 3600,
        .refresh_token_lifetime = 2592001,  // Above MAX_REFRESH_LIFETIME (2592000)
        .id_token_lifetime = 3600
    };
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_token_lifetimes(&tokens, &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(ready);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    cleanup_message_array(&messages, count);
}

void test_validate_oidc_token_lifetimes_id_too_high(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    OIDCTokensConfig tokens = {
        .access_token_lifetime = 3600,
        .refresh_token_lifetime = 86400,
        .id_token_lifetime = 86401  // Above MAX_TOKEN_LIFETIME (86400)
    };
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_token_lifetimes(&tokens, &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(ready);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    cleanup_message_array(&messages, count);
}

void test_validate_oidc_token_lifetimes_valid(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    OIDCTokensConfig tokens = {
        .access_token_lifetime = 3600,
        .refresh_token_lifetime = 86400,
        .id_token_lifetime = 3600
    };
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_token_lifetimes(&tokens, &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(ready);
    
    cleanup_message_array(&messages, count);
}

// Tests for validate_oidc_key_settings
void test_validate_oidc_key_settings_encryption_without_key(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    OIDCKeysConfig keys = {
        .encryption_enabled = true,
        .encryption_key = NULL,
        .rotation_interval_days = 90
    };
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_key_settings(&keys, &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(ready);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    cleanup_message_array(&messages, count);
}

void test_validate_oidc_key_settings_rotation_too_low(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    OIDCKeysConfig keys = {
        .encryption_enabled = false,
        .encryption_key = NULL,
        .rotation_interval_days = 0  // Below MIN_KEY_ROTATION_DAYS (1)
    };
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_key_settings(&keys, &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(ready);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    cleanup_message_array(&messages, count);
}

void test_validate_oidc_key_settings_rotation_too_high(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    OIDCKeysConfig keys = {
        .encryption_enabled = false,
        .encryption_key = NULL,
        .rotation_interval_days = 91  // Above MAX_KEY_ROTATION_DAYS (90)
    };
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_key_settings(&keys, &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(ready);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    cleanup_message_array(&messages, count);
}

void test_validate_oidc_key_settings_valid(void) {
    const char** messages;
    size_t count, capacity;
    bool ready = true;
    OIDCKeysConfig keys = {
        .encryption_enabled = false,
        .encryption_key = NULL,
        .rotation_interval_days = 30
    };
    
    setup_message_array(&messages, &count, &capacity);
    
    bool result = validate_oidc_key_settings(&keys, &messages, &count, &capacity, &ready);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(ready);
    
    cleanup_message_array(&messages, count);
}

int main(void) {
    UNITY_BEGIN();
    
    // Issuer validation tests
    RUN_TEST(test_validate_oidc_issuer_null);
    RUN_TEST(test_validate_oidc_issuer_empty);
    RUN_TEST(test_validate_oidc_issuer_invalid_scheme);
    RUN_TEST(test_validate_oidc_issuer_valid_http);
    RUN_TEST(test_validate_oidc_issuer_valid_https);
    
    // Redirect URI validation tests
    RUN_TEST(test_validate_oidc_redirect_uri_null);
    RUN_TEST(test_validate_oidc_redirect_uri_empty);
    RUN_TEST(test_validate_oidc_redirect_uri_invalid_scheme);
    RUN_TEST(test_validate_oidc_redirect_uri_valid);
    
    // Port validation tests
    RUN_TEST(test_validate_oidc_port_too_low);
    RUN_TEST(test_validate_oidc_port_too_high);
    RUN_TEST(test_validate_oidc_port_valid);
    
    // Token lifetimes validation tests
    RUN_TEST(test_validate_oidc_token_lifetimes_access_too_low);
    RUN_TEST(test_validate_oidc_token_lifetimes_refresh_too_high);
    RUN_TEST(test_validate_oidc_token_lifetimes_id_too_high);
    RUN_TEST(test_validate_oidc_token_lifetimes_valid);
    
    // Key settings validation tests
    RUN_TEST(test_validate_oidc_key_settings_encryption_without_key);
    RUN_TEST(test_validate_oidc_key_settings_rotation_too_low);
    RUN_TEST(test_validate_oidc_key_settings_rotation_too_high);
    RUN_TEST(test_validate_oidc_key_settings_valid);
    
    return UNITY_END();
}