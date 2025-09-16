/*
 * Unity Test File: is_terminal_request Function Tests
 * This file contains unit tests for the is_terminal_request() function
 * from src/terminal/terminal.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../src/terminal/terminal.h"

// Forward declaration for the function being tested
bool is_terminal_request(const char *url, const TerminalConfig *config);

// Function prototypes for test functions
void test_is_terminal_request_null_parameters(void);
void test_is_terminal_request_disabled_config(void);
void test_is_terminal_request_missing_web_path(void);
void test_is_terminal_request_exact_match(void);
void test_is_terminal_request_with_trailing_slash(void);
void test_is_terminal_request_subdirectory(void);
void test_is_terminal_request_no_match(void);
void test_is_terminal_request_edge_cases(void);

// Test fixtures
static TerminalConfig test_config;

void setUp(void) {
    // Initialize test config
    memset(&test_config, 0, sizeof(TerminalConfig));
    test_config.enabled = true;
    test_config.web_path = strdup("/terminal");
}

void tearDown(void) {
    // Clean up test config
    free(test_config.web_path);
    memset(&test_config, 0, sizeof(TerminalConfig));
}

void test_is_terminal_request_null_parameters(void) {
    // Test with null config
    TEST_ASSERT_FALSE(is_terminal_request("/terminal", NULL));

    // Test with null URL
    TEST_ASSERT_FALSE(is_terminal_request(NULL, &test_config));

    // Test with both null
    TEST_ASSERT_FALSE(is_terminal_request(NULL, NULL));
}

void test_is_terminal_request_disabled_config(void) {
    // Test with disabled config
    test_config.enabled = false;
    TEST_ASSERT_FALSE(is_terminal_request("/terminal", &test_config));

    // Reset
    test_config.enabled = true;
}

void test_is_terminal_request_missing_web_path(void) {
    // Test with null web_path
    free(test_config.web_path);
    test_config.web_path = NULL;
    TEST_ASSERT_FALSE(is_terminal_request("/terminal", &test_config));

    // Reset
    test_config.web_path = strdup("/terminal");
}

void test_is_terminal_request_exact_match(void) {
    // Test exact match
    TEST_ASSERT_TRUE(is_terminal_request("/terminal", &test_config));
}

void test_is_terminal_request_with_trailing_slash(void) {
    // Test URL with trailing slash
    TEST_ASSERT_TRUE(is_terminal_request("/terminal/", &test_config));
}

void test_is_terminal_request_subdirectory(void) {
    // Test subdirectory access
    TEST_ASSERT_TRUE(is_terminal_request("/terminal/index.html", &test_config));
    TEST_ASSERT_TRUE(is_terminal_request("/terminal/subdir/file.js", &test_config));
}

void test_is_terminal_request_no_match(void) {
    // Test URLs that should not match
    TEST_ASSERT_FALSE(is_terminal_request("/other", &test_config));
    TEST_ASSERT_FALSE(is_terminal_request("/terminal-other", &test_config));
    TEST_ASSERT_FALSE(is_terminal_request("/api/terminal", &test_config));
}

void test_is_terminal_request_edge_cases(void) {
    // Test empty string
    TEST_ASSERT_FALSE(is_terminal_request("", &test_config));

    // Test root path
    TEST_ASSERT_FALSE(is_terminal_request("/", &test_config));

    // Test longer prefix
    free(test_config.web_path);
    test_config.web_path = strdup("/terminal/sub");
    TEST_ASSERT_TRUE(is_terminal_request("/terminal/sub", &test_config));
    TEST_ASSERT_TRUE(is_terminal_request("/terminal/sub/", &test_config));
    TEST_ASSERT_TRUE(is_terminal_request("/terminal/sub/file", &test_config));
    TEST_ASSERT_FALSE(is_terminal_request("/terminal/other", &test_config));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_is_terminal_request_null_parameters);
    RUN_TEST(test_is_terminal_request_disabled_config);
    RUN_TEST(test_is_terminal_request_missing_web_path);
    RUN_TEST(test_is_terminal_request_exact_match);
    RUN_TEST(test_is_terminal_request_with_trailing_slash);
    RUN_TEST(test_is_terminal_request_subdirectory);
    RUN_TEST(test_is_terminal_request_no_match);
    RUN_TEST(test_is_terminal_request_edge_cases);

    return UNITY_END();
}