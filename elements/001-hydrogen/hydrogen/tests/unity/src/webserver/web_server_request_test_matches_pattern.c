/*
 * Unity Test File: Web Server Request - Matches Pattern Test
 * This file contains unit tests for matches_pattern() function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_request.h>

// Standard library includes
#include <string.h>

// Forward declaration for function being tested
bool matches_pattern(const char* path, const char* pattern);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test NULL parameters
static void test_matches_pattern_null_path(void) {
    bool result = matches_pattern(NULL, "*.js");
    TEST_ASSERT_FALSE(result);
}

static void test_matches_pattern_null_pattern(void) {
    bool result = matches_pattern("test.js", NULL);
    TEST_ASSERT_FALSE(result);
}

static void test_matches_pattern_both_null(void) {
    bool result = matches_pattern(NULL, NULL);
    TEST_ASSERT_FALSE(result);
}

// Test wildcard pattern
static void test_matches_pattern_wildcard_all(void) {
    // "*" pattern should match any path
    TEST_ASSERT_TRUE(matches_pattern("test.js", "*"));
    TEST_ASSERT_TRUE(matches_pattern("index.html", "*"));
    TEST_ASSERT_TRUE(matches_pattern("style.css", "*"));
    TEST_ASSERT_TRUE(matches_pattern("", "*"));
}

// Test file extension patterns
static void test_matches_pattern_js_extension(void) {
    // ".js" pattern should match files ending with .js
    TEST_ASSERT_TRUE(matches_pattern("test.js", ".js"));
    TEST_ASSERT_TRUE(matches_pattern("app.js", ".js"));
    TEST_ASSERT_TRUE(matches_pattern("module.min.js", ".js"));
}

static void test_matches_pattern_js_extension_negative(void) {
    // ".js" pattern should NOT match files not ending with .js
    TEST_ASSERT_FALSE(matches_pattern("test.json", ".js"));
    TEST_ASSERT_FALSE(matches_pattern("test.jsx", ".js"));
    TEST_ASSERT_FALSE(matches_pattern("test.js.map", ".js"));
}

static void test_matches_pattern_html_extension(void) {
    // ".html" pattern should match files ending with .html
    TEST_ASSERT_TRUE(matches_pattern("index.html", ".html"));
    TEST_ASSERT_TRUE(matches_pattern("page.html", ".html"));
}

static void test_matches_pattern_css_extension(void) {
    // ".css" pattern should match files ending with .css
    TEST_ASSERT_TRUE(matches_pattern("style.css", ".css"));
    TEST_ASSERT_TRUE(matches_pattern("theme.css", ".css"));
}

static void test_matches_pattern_wasm_extension(void) {
    // ".wasm" pattern should match files ending with .wasm
    TEST_ASSERT_TRUE(matches_pattern("module.wasm", ".wasm"));
    TEST_ASSERT_TRUE(matches_pattern("binary.wasm", ".wasm"));
}

// Test extension pattern with path shorter than pattern
static void test_matches_pattern_path_shorter_than_extension(void) {
    // Path shorter than pattern should return false
    TEST_ASSERT_FALSE(matches_pattern(".j", ".js"));
    TEST_ASSERT_FALSE(matches_pattern("j", ".js"));
    TEST_ASSERT_FALSE(matches_pattern("", ".js"));
}

// Test substring matching for non-extension patterns
static void test_matches_pattern_substring_matching(void) {
    // Non-extension patterns use substring matching
    TEST_ASSERT_TRUE(matches_pattern("test-file.js", "test"));
    TEST_ASSERT_TRUE(matches_pattern("my-module.js", "module"));
    TEST_ASSERT_TRUE(matches_pattern("config.json", "config"));
}

static void test_matches_pattern_substring_not_found(void) {
    // Substring not found should return false
    TEST_ASSERT_FALSE(matches_pattern("test.js", "module"));
    TEST_ASSERT_FALSE(matches_pattern("index.html", "admin"));
}

// Test edge cases for extension matching
static void test_matches_pattern_exact_length_match(void) {
    // Path length exactly equals pattern length
    TEST_ASSERT_TRUE(matches_pattern(".js", ".js"));
    TEST_ASSERT_TRUE(matches_pattern(".css", ".css"));
}

static void test_matches_pattern_case_sensitive(void) {
    // Pattern matching should be case sensitive
    TEST_ASSERT_FALSE(matches_pattern("test.JS", ".js"));
    TEST_ASSERT_FALSE(matches_pattern("test.Js", ".js"));
    TEST_ASSERT_TRUE(matches_pattern("test.js", ".js"));
}

// Test patterns without leading dot (substring matching)
static void test_matches_pattern_no_leading_dot(void) {
    // Patterns without leading dot use substring matching
    TEST_ASSERT_TRUE(matches_pattern("test.js", "js"));
    TEST_ASSERT_TRUE(matches_pattern("index.html", "html"));
    TEST_ASSERT_TRUE(matches_pattern("style.css", "css"));
}

// Test various substring patterns
static void test_matches_pattern_substring_variations(void) {
    // Various substring matching scenarios
    TEST_ASSERT_TRUE(matches_pattern("worker.js", "worker"));
    TEST_ASSERT_TRUE(matches_pattern("app-bundle.js", "bundle"));
    TEST_ASSERT_TRUE(matches_pattern("jquery.min.js", "jquery"));
    TEST_ASSERT_TRUE(matches_pattern("bootstrap.css", "bootstrap"));
}

// Test empty patterns
static void test_matches_pattern_empty_pattern(void) {
    // Empty pattern with non-extension format
    // Should use substring matching - empty string is in all strings
    TEST_ASSERT_TRUE(matches_pattern("test.js", ""));
}

static void test_matches_pattern_empty_path(void) {
    // Empty path with non-wildcard pattern
    TEST_ASSERT_FALSE(matches_pattern("", ".js"));
    TEST_ASSERT_TRUE(matches_pattern("", ""));  // Both empty
}

// Test single character patterns
static void test_matches_pattern_single_char_extension(void) {
    // Single character "extension" (2 chars including dot)
    TEST_ASSERT_TRUE(matches_pattern("file.c", ".c"));
    TEST_ASSERT_TRUE(matches_pattern("main.h", ".h"));
    TEST_ASSERT_FALSE(matches_pattern("test.js", ".c"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_matches_pattern_null_path);
    RUN_TEST(test_matches_pattern_null_pattern);
    RUN_TEST(test_matches_pattern_both_null);
    RUN_TEST(test_matches_pattern_wildcard_all);
    RUN_TEST(test_matches_pattern_js_extension);
    RUN_TEST(test_matches_pattern_js_extension_negative);
    RUN_TEST(test_matches_pattern_html_extension);
    RUN_TEST(test_matches_pattern_css_extension);
    RUN_TEST(test_matches_pattern_wasm_extension);
    RUN_TEST(test_matches_pattern_path_shorter_than_extension);
    RUN_TEST(test_matches_pattern_substring_matching);
    RUN_TEST(test_matches_pattern_substring_not_found);
    RUN_TEST(test_matches_pattern_exact_length_match);
    RUN_TEST(test_matches_pattern_case_sensitive);
    RUN_TEST(test_matches_pattern_no_leading_dot);
    RUN_TEST(test_matches_pattern_substring_variations);
    RUN_TEST(test_matches_pattern_empty_pattern);
    RUN_TEST(test_matches_pattern_empty_path);
    RUN_TEST(test_matches_pattern_single_char_extension);

    return UNITY_END();
}
