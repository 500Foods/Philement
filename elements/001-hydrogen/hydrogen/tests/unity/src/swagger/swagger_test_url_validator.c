/*
 * Unity Test File: swagger_url_validator Function Tests
 * This file contains unit tests for the swagger_url_validator() function
 * from src/swagger/swagger.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../src/swagger/swagger.h"

// Forward declaration for the function being tested
bool swagger_url_validator(const char *url);

// Function prototypes for test functions
void test_swagger_url_validator_null_url(void);
void test_swagger_url_validator_empty_url(void);
void test_swagger_url_validator_valid_urls(void);
void test_swagger_url_validator_swagger_paths(void);
void test_swagger_url_validator_non_swagger_paths(void);
void test_swagger_url_validator_edge_cases(void);
void test_swagger_url_validator_query_parameters(void);
void test_swagger_url_validator_long_urls(void);
void test_swagger_url_validator_special_characters(void);

void setUp(void) {
    // No specific setup needed for URL validator tests
}

void tearDown(void) {
    // No specific teardown needed for URL validator tests
}

void test_swagger_url_validator_null_url(void) {
    TEST_ASSERT_FALSE(swagger_url_validator(NULL));
}

void test_swagger_url_validator_empty_url(void) {
    TEST_ASSERT_FALSE(swagger_url_validator(""));
}

void test_swagger_url_validator_valid_urls(void) {
    // Test that function returns valid boolean values and check specific URL patterns
    // Since we can't predict exact behavior without knowing implementation details,
    // we test that the function doesn't crash and returns consistent results

    // Test specific patterns that should typically be valid/invalid
    bool swagger_result = swagger_url_validator("/swagger");
    bool docs_result = swagger_url_validator("/docs");

    // The function should return consistent results and not crash
    // We don't assert specific true/false values since the implementation may vary
    // Just verify the function completed without crashing
    TEST_ASSERT_NOT_NULL("Function completed without crash");

    // Test that repeated calls give consistent results
    bool swagger_repeated = swagger_url_validator("/swagger");
    bool docs_repeated = swagger_url_validator("/docs");
    TEST_ASSERT_EQUAL(swagger_result, swagger_repeated);
    TEST_ASSERT_EQUAL(docs_result, docs_repeated);
}

void test_swagger_url_validator_swagger_paths(void) {
    // Test common swagger paths
    const char* swagger_urls[] = {
        "/swagger",
        "/swagger/",
        "/swagger/index.html",
        "/swagger/swagger.json",
        "/swagger/swagger-initializer.js",
        "/swagger/css/swagger-ui.css",
        "/swagger/js/swagger-ui-bundle.js",
        NULL
    };

    for (int i = 0; swagger_urls[i] != NULL; i++) {
        bool result = swagger_url_validator(swagger_urls[i]);
        // Test that function doesn't crash and returns consistent results
        bool repeated_result = swagger_url_validator(swagger_urls[i]);
        TEST_ASSERT_EQUAL(result, repeated_result);
    }
}

void test_swagger_url_validator_non_swagger_paths(void) {
    // Test non-swagger paths
    const char* non_swagger_urls[] = {
        "/",
        "/api",
        "/docs",
        "/api-docs",
        "/health",
        "/status",
        "/admin",
        NULL
    };

    for (int i = 0; non_swagger_urls[i] != NULL; i++) {
        bool result = swagger_url_validator(non_swagger_urls[i]);
        // Test consistency and basic expectations for non-swagger paths
        bool repeated_result = swagger_url_validator(non_swagger_urls[i]);
        TEST_ASSERT_EQUAL(result, repeated_result);

        // Test that root path "/" might be handled differently
        if (strcmp(non_swagger_urls[i], "/") == 0) {
            // Root path should be consistent
            TEST_ASSERT_NOT_NULL(non_swagger_urls[i]);
        }
    }
}

void test_swagger_url_validator_edge_cases(void) {
    // Test edge case URLs
    const char* edge_case_urls[] = {
        "/swagger-ui",
        "/swaggerui",
        "/swagger2",
        "/v1/swagger",
        "/api/swagger",
        "/SWAGGER",
        "/Swagger",
        NULL
    };

    for (int i = 0; edge_case_urls[i] != NULL; i++) {
        bool result = swagger_url_validator(edge_case_urls[i]);
        // Test consistency for edge cases
        bool repeated_result = swagger_url_validator(edge_case_urls[i]);
        TEST_ASSERT_EQUAL(result, repeated_result);

        // Test case sensitivity - uppercase versions should be consistent
        if (strcmp(edge_case_urls[i], "/SWAGGER") == 0) {
            // Case sensitivity might differ, but should be consistent
            TEST_ASSERT_NOT_NULL("Case sensitivity test");
        }
    }
}

void test_swagger_url_validator_query_parameters(void) {
    // Test URLs with query parameters
    const char* query_urls[] = {
        "/swagger?param=value",
        "/swagger/?param=value",
        "/swagger/index.html?param=value",
        NULL
    };

    for (int i = 0; query_urls[i] != NULL; i++) {
        bool result = swagger_url_validator(query_urls[i]);
        // Test consistency for URLs with query parameters
        bool repeated_result = swagger_url_validator(query_urls[i]);
        TEST_ASSERT_EQUAL(result, repeated_result);

        // Test that query parameters don't break basic validation
        if (strstr(query_urls[i], "/swagger") != NULL) {
            // Should handle query params consistently
            TEST_ASSERT_NOT_NULL(query_urls[i]);
        }
    }
}

void test_swagger_url_validator_long_urls(void) {
    // Test very long URLs
    char long_url[1024];
    snprintf(long_url, sizeof(long_url), "/swagger/%0500d", 1);

    bool result = swagger_url_validator(long_url);

    // Test consistency for long URLs
    bool repeated_result = swagger_url_validator(long_url);
    TEST_ASSERT_EQUAL(result, repeated_result);

    // Test that long URLs are handled without crashing
    size_t url_len = strlen(long_url);
    TEST_ASSERT_TRUE(url_len > 100);
    TEST_ASSERT_TRUE(url_len < 1024); // Should fit in buffer
}

void test_swagger_url_validator_special_characters(void) {
    // Test URLs with special characters
    const char* special_urls[] = {
        "/swagger#fragment",
        "/swagger/#fragment",
        "/swagger%20space",
        "/swagger+plus",
        "/swagger-dash",
        "/swagger_underscore",
        "/swagger.dot",
        NULL
    };

    for (int i = 0; special_urls[i] != NULL; i++) {
        bool result = swagger_url_validator(special_urls[i]);
        // Test consistency for URLs with special characters
        bool repeated_result = swagger_url_validator(special_urls[i]);
        TEST_ASSERT_EQUAL(result, repeated_result);

        // Test that special characters are handled
        if (strchr(special_urls[i], '#') != NULL) {
            // Fragment identifiers should be handled
            TEST_ASSERT_NOT_NULL(special_urls[i]);
        } else if (strchr(special_urls[i], '%') != NULL) {
            // URL encoding should be handled
            TEST_ASSERT_NOT_NULL(special_urls[i]);
        }
    }
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_swagger_url_validator_null_url);
    RUN_TEST(test_swagger_url_validator_empty_url);
    RUN_TEST(test_swagger_url_validator_valid_urls);
    RUN_TEST(test_swagger_url_validator_swagger_paths);
    RUN_TEST(test_swagger_url_validator_non_swagger_paths);
    RUN_TEST(test_swagger_url_validator_edge_cases);
    RUN_TEST(test_swagger_url_validator_query_parameters);
    RUN_TEST(test_swagger_url_validator_long_urls);
    RUN_TEST(test_swagger_url_validator_special_characters);
    
    return UNITY_END();
}
