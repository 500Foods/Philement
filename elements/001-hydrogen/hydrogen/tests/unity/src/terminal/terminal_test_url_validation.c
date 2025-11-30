/*
 * Unity Test File: Terminal URL Validation Tests
 * Tests terminal.c functions for URL validation and request handling
 * Focuses on functions that don't require complex subsystem initialization
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the terminal module
#include <src/terminal/terminal.h>

// Test fixtures
static TerminalConfig test_config;

void setUp(void) {
    // Initialize test config
    memset(&test_config, 0, sizeof(TerminalConfig));
    test_config.enabled = true;
    test_config.web_path = (char*)"/terminal";
    test_config.index_page = (char*)"terminal.html";
}

void tearDown(void) {
    // No cleanup needed
}

// Function prototypes for test functions
void test_is_terminal_request_null_parameters(void);
void test_is_terminal_request_disabled_config(void);
void test_is_terminal_request_null_web_path(void);
void test_is_terminal_request_null_url(void);
void test_is_terminal_request_exact_match(void);
void test_is_terminal_request_with_slash(void);
void test_is_terminal_request_subdirectory(void);
void test_is_terminal_request_no_match(void);
void test_is_terminal_request_partial_match(void);
void test_terminal_url_validator_disabled(void);

int main(void) {
    UNITY_BEGIN();

    // is_terminal_request tests
    RUN_TEST(test_is_terminal_request_null_parameters);
    RUN_TEST(test_is_terminal_request_disabled_config);
    RUN_TEST(test_is_terminal_request_null_web_path);
    RUN_TEST(test_is_terminal_request_null_url);
    RUN_TEST(test_is_terminal_request_exact_match);
    RUN_TEST(test_is_terminal_request_with_slash);
    RUN_TEST(test_is_terminal_request_subdirectory);
    RUN_TEST(test_is_terminal_request_no_match);
    RUN_TEST(test_is_terminal_request_partial_match);

    // terminal_url_validator tests
    RUN_TEST(test_terminal_url_validator_disabled);

    return UNITY_END();
}

/*
 * TEST SUITE: is_terminal_request
 */

void test_is_terminal_request_null_parameters(void) {
    // Test NULL config
    bool result = is_terminal_request("/terminal", NULL);
    TEST_ASSERT_FALSE(result);

    // Test NULL URL
    result = is_terminal_request(NULL, &test_config);
    TEST_ASSERT_FALSE(result);

    // Test both NULL
    result = is_terminal_request(NULL, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_is_terminal_request_disabled_config(void) {
    TerminalConfig disabled_config = {0}; // disabled = false by default
    disabled_config.web_path = (char*)"/terminal";

    bool result = is_terminal_request("/terminal", &disabled_config);
    TEST_ASSERT_FALSE(result);
}

void test_is_terminal_request_null_web_path(void) {
    TerminalConfig config_no_path = {0};
    config_no_path.enabled = true;
    // web_path remains NULL

    bool result = is_terminal_request("/terminal", &config_no_path);
    TEST_ASSERT_FALSE(result);
}

void test_is_terminal_request_null_url(void) {
    // Already tested in null parameters test
    TEST_PASS();
}

void test_is_terminal_request_exact_match(void) {
    bool result = is_terminal_request("/terminal", &test_config);
    TEST_ASSERT_TRUE(result);
}

void test_is_terminal_request_with_slash(void) {
    bool result = is_terminal_request("/terminal/", &test_config);
    TEST_ASSERT_TRUE(result);
}

void test_is_terminal_request_subdirectory(void) {
    bool result = is_terminal_request("/terminal/terminal.html", &test_config);
    TEST_ASSERT_TRUE(result);

    result = is_terminal_request("/terminal/js/app.js", &test_config);
    TEST_ASSERT_TRUE(result);
}

void test_is_terminal_request_no_match(void) {
    bool result = is_terminal_request("/other", &test_config);
    TEST_ASSERT_FALSE(result);

    result = is_terminal_request("/terminal-other", &test_config);
    TEST_ASSERT_FALSE(result);

    result = is_terminal_request("/", &test_config);
    TEST_ASSERT_FALSE(result);
}

void test_is_terminal_request_partial_match(void) {
    // Test that partial matches don't work
    bool result = is_terminal_request("/term", &test_config);
    TEST_ASSERT_FALSE(result);

    result = is_terminal_request("/terminal-other", &test_config);
    TEST_ASSERT_FALSE(result);
}

/*
 * TEST SUITE: terminal_url_validator
 */

void test_terminal_url_validator_disabled(void) {
    // Test that validator returns false when global config is NULL (default state)
    // This tests the case where terminal support is not initialized/disabled
    bool result = terminal_url_validator("/terminal");
    TEST_ASSERT_FALSE(result);

    result = terminal_url_validator("/terminal/somefile.html");
    TEST_ASSERT_FALSE(result);

    result = terminal_url_validator("/");
    TEST_ASSERT_FALSE(result);
}
