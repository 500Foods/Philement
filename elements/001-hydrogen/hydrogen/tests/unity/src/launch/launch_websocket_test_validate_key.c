/*
 * Unity Test File: WebSocket Key Validation Tests
 * Tests validate_key with fail-closed rules (TERMINAL_FIX_PLAN Phase 2):
 *   - Non-NULL, non-empty, >= 32 printable ASCII chars (no spaces/control)
 *   - Rejects unresolved ${env.*} references
 *   - Rejects known default/fallback literals
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>

// Forward declarations for functions being tested
bool validate_key(const char* key);

// Forward declarations for test functions
void test_validate_key_valid_cases(void);
void test_validate_key_invalid_too_short(void);
void test_validate_key_invalid_control_chars(void);
void test_validate_key_null_and_empty(void);
void test_validate_key_rejects_env_reference(void);
void test_validate_key_rejects_known_defaults(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_validate_key_valid_cases(void) {
    // Valid keys: 32+ printable ASCII chars, no spaces/control, not a known default
    TEST_ASSERT_TRUE(validate_key("01234567890123456789012345678901"));  // 32 chars
    TEST_ASSERT_TRUE(validate_key("abcdefghijklmnopqrstuvwxyz012345"));  // 32 chars
    TEST_ASSERT_TRUE(validate_key("MySuperSecretWebSocketKey1234567890abcde"));
    TEST_ASSERT_TRUE(validate_key("!@#$%^&*()_+-=[]{}|;:,.<>?/`~!@#$"));
}

void test_validate_key_invalid_too_short(void) {
    // Rejected: shorter than 32 characters
    TEST_ASSERT_FALSE(validate_key("short"));
    TEST_ASSERT_FALSE(validate_key("validkey123"));
    TEST_ASSERT_FALSE(validate_key("abcdefghijklmnop"));       // 16 chars
    TEST_ASSERT_FALSE(validate_key("1234567890123456789012345678901"));  // 31 chars
}

void test_validate_key_invalid_control_chars(void) {
    // Rejected: spaces, tabs, newlines, control chars
    TEST_ASSERT_FALSE(validate_key("key\twithtab"));
    TEST_ASSERT_FALSE(validate_key("key\nwithnewline"));
    TEST_ASSERT_FALSE(validate_key("key with space"));
    TEST_ASSERT_FALSE(validate_key("key\x01withcontrol"));
}

void test_validate_key_null_and_empty(void) {
    // Rejected: NULL and empty strings
    TEST_ASSERT_FALSE(validate_key(NULL));
    TEST_ASSERT_FALSE(validate_key(""));
}

void test_validate_key_rejects_env_reference(void) {
    // Rejected: unresolved ${env.*} reference (17 chars, all printable — passes
    // the old length-only check, but must fail fail-closed)
    TEST_ASSERT_FALSE(validate_key("${env.WEBSOCKET_KEY}"));
    TEST_ASSERT_FALSE(validate_key("${env.WEBSOCKET_KEY}shortenvref"));
}

void test_validate_key_rejects_known_defaults(void) {
    // Rejected: known default/fallback literals
    TEST_ASSERT_FALSE(validate_key("default_key"));
    TEST_ASSERT_FALSE(validate_key("default_websocket_key"));
    TEST_ASSERT_FALSE(validate_key("ABCDEFGHIJKLMNOP"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_key_valid_cases);
    RUN_TEST(test_validate_key_invalid_too_short);
    RUN_TEST(test_validate_key_invalid_control_chars);
    RUN_TEST(test_validate_key_null_and_empty);
    RUN_TEST(test_validate_key_rejects_env_reference);
    RUN_TEST(test_validate_key_rejects_known_defaults);

    return UNITY_END();
}
