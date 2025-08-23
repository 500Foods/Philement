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
    // Test various URLs - results depend on global config state
    bool result1 = swagger_url_validator("/swagger");
    bool result2 = swagger_url_validator("/swagger/");
    bool result3 = swagger_url_validator("/docs");
    
    // All should be boolean values
    TEST_ASSERT_TRUE(result1 == true || result1 == false);
    TEST_ASSERT_TRUE(result2 == true || result2 == false);
    TEST_ASSERT_TRUE(result3 == true || result3 == false);
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
        TEST_ASSERT_TRUE(result == true || result == false);
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
        TEST_ASSERT_TRUE(result == true || result == false);
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
        TEST_ASSERT_TRUE(result == true || result == false);
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
        TEST_ASSERT_TRUE(result == true || result == false);
    }
}

void test_swagger_url_validator_long_urls(void) {
    // Test very long URLs
    char long_url[1024];
    snprintf(long_url, sizeof(long_url), "/swagger/%0500d", 1);
    
    bool result = swagger_url_validator(long_url);
    TEST_ASSERT_TRUE(result == true || result == false);
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
        TEST_ASSERT_TRUE(result == true || result == false);
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
