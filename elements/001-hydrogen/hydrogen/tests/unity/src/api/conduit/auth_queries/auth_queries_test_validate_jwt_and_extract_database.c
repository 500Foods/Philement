/*
 * Unity Test File: Auth Queries Validate JWT and Extract Database
 * This file contains unit tests for the validate_jwt_and_extract_database function
 * in src/api/conduit/auth_queries/auth_queries.c
 *
 * Tests JWT validation and database extraction from Authorization header.
 *
 * CHANGELOG:
 * 2026-02-19: Initial creation of unit tests for validate_jwt_and_extract_database
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for external dependencies
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_system.h>

// Include source headers
#include <src/api/conduit/auth_queries/auth_queries.h>

// Mock app_config for testing
extern AppConfig *app_config;

// Function prototypes for test functions
void test_validate_jwt_and_extract_database_null_params(void);
void test_validate_jwt_and_extract_database_null_database_ptr(void);
void test_validate_jwt_and_extract_database_missing_auth_header(void);
void test_validate_jwt_and_extract_database_invalid_auth_format(void);
void test_validate_jwt_and_extract_database_invalid_jwt(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();

    // Initialize mock app config
    app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);

    app_config->databases.connection_count = 1;
    DatabaseConnection *conn = &app_config->databases.connections[0];
    conn->enabled = true;
    conn->connection_name = strdup("testdb");
    conn->max_queries_per_request = 5;
}

void tearDown(void) {
    if (app_config) {
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            free(app_config->databases.connections[i].connection_name);
        }
        free(app_config);
        app_config = NULL;
    }

    mock_mhd_reset_all();
    mock_system_reset_all();
}

// Test validate_jwt_and_extract_database with NULL parameters
void test_validate_jwt_and_extract_database_null_params(void) {
    char *database = NULL;
    
    enum MHD_Result result = validate_jwt_and_extract_database(NULL, &database);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test validate_jwt_and_extract_database with NULL database pointer
void test_validate_jwt_and_extract_database_null_database_ptr(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    
    enum MHD_Result result = validate_jwt_and_extract_database(mock_connection, NULL);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test validate_jwt_and_extract_database with missing Authorization header
void test_validate_jwt_and_extract_database_missing_auth_header(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    char *database = NULL;
    
    // Mock MHD_lookup_connection_value to return NULL (no Authorization header)
    mock_mhd_set_lookup_result(NULL);
    
    enum MHD_Result result = validate_jwt_and_extract_database(mock_connection, &database);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test validate_jwt_and_extract_database with invalid Authorization header format
void test_validate_jwt_and_extract_database_invalid_auth_format(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    char *database = NULL;
    
    // Mock MHD_lookup_connection_value to return invalid format (no Bearer prefix)
    mock_mhd_set_lookup_result("InvalidToken");
    
    enum MHD_Result result = validate_jwt_and_extract_database(mock_connection, &database);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test validate_jwt_and_extract_database with invalid JWT token
void test_validate_jwt_and_extract_database_invalid_jwt(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    char *database = NULL;
    
    // Mock MHD_lookup_connection_value to return invalid JWT
    mock_mhd_set_lookup_result("Bearer invalid_token");
    
    enum MHD_Result result = validate_jwt_and_extract_database(mock_connection, &database);
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_validate_jwt_and_extract_database_null_params);
    RUN_TEST(test_validate_jwt_and_extract_database_null_database_ptr);
    RUN_TEST(test_validate_jwt_and_extract_database_missing_auth_header);
    RUN_TEST(test_validate_jwt_and_extract_database_invalid_auth_format);
    RUN_TEST(test_validate_jwt_and_extract_database_invalid_jwt);
    
    return UNITY_END();
}
