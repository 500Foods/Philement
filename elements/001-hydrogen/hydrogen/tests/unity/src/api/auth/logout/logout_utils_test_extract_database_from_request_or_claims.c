/*
 * Unity Unit Tests for logout_utils.c - extract_database_from_request_or_claims
 *
 * Tests the extract_database_from_request_or_claims function
 *
 * CHANGELOG:
 * 2026-01-16: Initial version - Tests for extract_database_from_request_or_claims
 *
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/logout/logout_utils.h>

// ============================================================================
// Function Prototypes
// ============================================================================

// Test functions
void test_extract_database_from_request_or_claims_request_body_database(void);
void test_extract_database_from_request_or_claims_jwt_claims_database(void);
void test_extract_database_from_request_or_claims_no_database(void);
void test_extract_database_from_request_or_claims_null_buffer(void);
void test_extract_database_from_request_or_claims_null_claims(void);
void test_extract_database_from_request_or_claims_invalid_json(void);
void test_extract_database_from_request_or_claims_request_out_parameter(void);

// ============================================================================
// Test Setup/Teardown
// ============================================================================

void setUp(void) {
    // No setup needed for these tests
}

void tearDown(void) {
    // No teardown needed for these tests
}

// ============================================================================
// Test Functions
// ============================================================================

// Test: Database from request body takes precedence
void test_extract_database_from_request_or_claims_request_body_database(void) {
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';
    char data1[] = "{\"database\":\"requestdb\"}";
    buffer.data = data1;
    buffer.size = strlen(buffer.data);
    
    jwt_claims_t claims = {0};
    char database0[] = "tokendb";
    claims.database = database0;
    
    json_t* request_out = NULL;
    const char* database = extract_database_from_request_or_claims(
        &buffer, &claims, &request_out
    );
    
    TEST_ASSERT_NOT_NULL(database);
    TEST_ASSERT_EQUAL_STRING("requestdb", database);
    TEST_ASSERT_NOT_NULL(request_out);
    
    if (request_out) {
        json_decref(request_out);
    }
}

// Test: Database from JWT claims when request body doesn't specify
void test_extract_database_from_request_or_claims_jwt_claims_database(void) {
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';
    char data0[] = "{\"other_field\":\"value\"}";
    buffer.data = data0;
    buffer.size = strlen(buffer.data);
    
    jwt_claims_t claims = {0};
    char database1[] = "tokendb";
    claims.database = database1;
    
    json_t* request_out = NULL;
    const char* database = extract_database_from_request_or_claims(
        &buffer, &claims, &request_out
    );
    
    TEST_ASSERT_NOT_NULL(database);
    TEST_ASSERT_EQUAL_STRING("tokendb", database);
    TEST_ASSERT_NOT_NULL(request_out);
    
    if (request_out) {
        json_decref(request_out);
    }
}

// Test: No database specified anywhere
void test_extract_database_from_request_or_claims_no_database(void) {
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';
    char data2[] = "{\"other_field\":\"value\"}";
    buffer.data = data2;
    buffer.size = strlen(buffer.data);
    
    jwt_claims_t claims = {0};
    claims.database = NULL;
    
    json_t* request_out = NULL;
    const char* database = extract_database_from_request_or_claims(
        &buffer, &claims, &request_out
    );
    
    TEST_ASSERT_NULL(database);
    TEST_ASSERT_NOT_NULL(request_out);
    
    if (request_out) {
        json_decref(request_out);
    }
}

// Test: NULL buffer
void test_extract_database_from_request_or_claims_null_buffer(void) {
    jwt_claims_t claims = {0};
    char database3[] = "tokendb";
    claims.database = database3;
    
    json_t* request_out = NULL;
    const char* database = extract_database_from_request_or_claims(
        NULL, &claims, &request_out
    );
    
    TEST_ASSERT_NOT_NULL(database);
    TEST_ASSERT_EQUAL_STRING("tokendb", database);
    TEST_ASSERT_NULL(request_out);
}

// Test: NULL claims
void test_extract_database_from_request_or_claims_null_claims(void) {
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';
    char data3[] = "{\"other_field\":\"value\"}";
    buffer.data = data3;
    buffer.size = strlen(buffer.data);
    
    json_t* request_out = NULL;
    const char* database = extract_database_from_request_or_claims(
        &buffer, NULL, &request_out
    );
    
    TEST_ASSERT_NULL(database);
    TEST_ASSERT_NOT_NULL(request_out);
    
    if (request_out) {
        json_decref(request_out);
    }
}

// Test: Invalid JSON in request body
void test_extract_database_from_request_or_claims_invalid_json(void) {
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';
    char data4[] = "invalid json";
    buffer.data = data4;
    buffer.size = strlen(buffer.data);
    
    jwt_claims_t claims = {0};
    char database4[] = "tokendb";
    claims.database = database4;
    
    json_t* request_out = NULL;
    const char* database = extract_database_from_request_or_claims(
        &buffer, &claims, &request_out
    );
    
    // Should fall back to JWT claims database
    TEST_ASSERT_NOT_NULL(database);
    TEST_ASSERT_EQUAL_STRING("tokendb", database);
    TEST_ASSERT_NULL(request_out);
}

// Test: request_out parameter handling
void test_extract_database_from_request_or_claims_request_out_parameter(void) {
    ApiPostBuffer buffer = {0};
    buffer.http_method = 'P';
    char data6[] = "{\"database\":\"requestdb\"}";
    buffer.data = data6;
    buffer.size = strlen(buffer.data);
    
    jwt_claims_t claims = {0};
    char database6[] = "tokendb";
    claims.database = database6;
    
    // Test with request_out parameter
    json_t* request_out = NULL;
    const char* database = extract_database_from_request_or_claims(
        &buffer, &claims, &request_out
    );
    
    TEST_ASSERT_NOT_NULL(database);
    TEST_ASSERT_EQUAL_STRING("requestdb", database);
    TEST_ASSERT_NOT_NULL(request_out);
    
    if (request_out) {
        json_decref(request_out);
    }
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_extract_database_from_request_or_claims_request_body_database);
    RUN_TEST(test_extract_database_from_request_or_claims_jwt_claims_database);
    RUN_TEST(test_extract_database_from_request_or_claims_no_database);
    RUN_TEST(test_extract_database_from_request_or_claims_null_buffer);
    RUN_TEST(test_extract_database_from_request_or_claims_null_claims);
    RUN_TEST(test_extract_database_from_request_or_claims_invalid_json);
    RUN_TEST(test_extract_database_from_request_or_claims_request_out_parameter);
    
    return UNITY_END();
}