/*
 * Unity Unit Tests for renew_utils.c - extract_database_from_request_or_claims
 * 
 * Tests the database extraction functionality from request body or JWT claims
 * 
 * CHANGELOG:
 * 2026-01-16: Initial version - Tests for database extraction
 * 
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/renew/renew_utils.h>
#include <jansson.h>

// ============================================================================
// Function Prototypes
// ============================================================================

void test_extract_database_from_request_or_claims_null_request_null_claims(void);
void test_extract_database_from_request_or_claims_request_with_database(void);
void test_extract_database_from_request_or_claims_claims_with_database(void);
void test_extract_database_from_request_or_claims_request_priority(void);
void test_extract_database_from_request_or_claims_no_database(void);

// ============================================================================
// Test Functions
// ============================================================================

void test_extract_database_from_request_or_claims_null_request_null_claims(void) {
    char* database = extract_database_from_request_or_claims_renew(NULL, NULL);
    
    TEST_ASSERT_NULL(database);
}

void test_extract_database_from_request_or_claims_request_with_database(void) {
    json_t* request = json_object();
    json_object_set_new(request, "database", json_string("requestdb"));
    
    char* database = extract_database_from_request_or_claims_renew(request, NULL);
    
    TEST_ASSERT_NOT_NULL(database);
    TEST_ASSERT_EQUAL_STRING("requestdb", database);
    
    free(database);
    json_decref(request);
}

void test_extract_database_from_request_or_claims_claims_with_database(void) {
    jwt_claims_t claims = {0};
    claims.database = strdup("claimsdb");
    
    char* database = extract_database_from_request_or_claims_renew(NULL, &claims);
    
    TEST_ASSERT_NOT_NULL(database);
    TEST_ASSERT_EQUAL_STRING("claimsdb", database);
    
    free(database);
    free(claims.database);
}

void test_extract_database_from_request_or_claims_request_priority(void) {
    json_t* request = json_object();
    json_object_set_new(request, "database", json_string("requestdb"));
    
    jwt_claims_t claims = {0};
    claims.database = strdup("claimsdb");
    
    char* database = extract_database_from_request_or_claims_renew(request, &claims);
    
    TEST_ASSERT_NOT_NULL(database);
    TEST_ASSERT_EQUAL_STRING("requestdb", database);
    
    free(database);
    free(claims.database);
    json_decref(request);
}

void test_extract_database_from_request_or_claims_no_database(void) {
    json_t* request = json_object();
    jwt_claims_t claims = {0};
    
    char* database = extract_database_from_request_or_claims_renew(request, &claims);
    
    TEST_ASSERT_NULL(database);
    
    json_decref(request);
}

// ============================================================================
// Test Setup/Teardown
// ============================================================================

void setUp(void) {
    // Nothing to set up
}

void tearDown(void) {
    // Nothing to tear down
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_extract_database_from_request_or_claims_null_request_null_claims);
    RUN_TEST(test_extract_database_from_request_or_claims_request_with_database);
    RUN_TEST(test_extract_database_from_request_or_claims_claims_with_database);
    RUN_TEST(test_extract_database_from_request_or_claims_request_priority);
    RUN_TEST(test_extract_database_from_request_or_claims_no_database);
    
    return UNITY_END();
}