/*
 * Unity Test File: Auth Query Cleanup Resources
 * This file contains unit tests for the cleanup_auth_query_resources function
 * in src/api/conduit/auth_query/auth_query.c
 *
 * Tests resource cleanup functionality for authenticated query processing.
 *
 * CHANGELOG:
 * 2026-02-18: Initial creation of unit tests for cleanup_auth_query_resources
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for external dependencies
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Include source header
#include <src/api/conduit/auth_query/auth_query.h>
#include <src/database/database_params.h>

// Forward declarations for test functions
void test_cleanup_auth_query_resources_all_null(void);
void test_cleanup_auth_query_resources_with_request_json(void);
void test_cleanup_auth_query_resources_with_jwt_result(void);
void test_cleanup_auth_query_resources_with_query_id(void);
void test_cleanup_auth_query_resources_with_param_list(void);
void test_cleanup_auth_query_resources_with_converted_sql(void);
void test_cleanup_auth_query_resources_with_ordered_params(void);
void test_cleanup_auth_query_resources_with_message(void);
void test_cleanup_auth_query_resources_with_all_resources(void);
void test_cleanup_auth_query_resources_partial_resources(void);

// Test fixtures
void setUp(void) {
    // Reset all mocks before each test
    mock_mhd_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_mhd_reset_all();
}

// Test cleanup with all NULL parameters
void test_cleanup_auth_query_resources_all_null(void) {
    // Should not crash when all parameters are NULL
    cleanup_auth_query_resources(NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL);
    TEST_PASS();
}

// Test cleanup with request_json
void test_cleanup_auth_query_resources_with_request_json(void) {
    json_t *request_json = json_object();
    TEST_ASSERT_NOT_NULL(request_json);
    json_object_set_new(request_json, "query_ref", json_integer(123));
    
    // Should clean up without crashing
    cleanup_auth_query_resources(request_json, NULL, NULL, NULL, NULL, NULL, 0, NULL);
    TEST_PASS();
}

// Test cleanup with jwt_result
void test_cleanup_auth_query_resources_with_jwt_result(void) {
    jwt_validation_result_t *jwt_result = calloc(1, sizeof(jwt_validation_result_t));
    TEST_ASSERT_NOT_NULL(jwt_result);
    jwt_result->valid = true;
    jwt_result->claims = calloc(1, sizeof(jwt_claims_t));
    jwt_result->claims->database = strdup("testdb");
    jwt_result->claims->username = strdup("testuser");
    jwt_result->claims->user_id = 123;
    
    // Should clean up without crashing
    cleanup_auth_query_resources(NULL, jwt_result, NULL, NULL, NULL, NULL, 0, NULL);
    TEST_PASS();
}

// Test cleanup with query_id
void test_cleanup_auth_query_resources_with_query_id(void) {
    char *query_id = strdup("test_query_12345");
    TEST_ASSERT_NOT_NULL(query_id);
    
    // Should clean up without crashing
    cleanup_auth_query_resources(NULL, NULL, query_id, NULL, NULL, NULL, 0, NULL);
    TEST_PASS();
}

// Test cleanup with param_list
void test_cleanup_auth_query_resources_with_param_list(void) {
    ParameterList *param_list = calloc(1, sizeof(ParameterList));
    TEST_ASSERT_NOT_NULL(param_list);
    param_list->params = calloc(2, sizeof(TypedParameter*));
    param_list->params[0] = calloc(1, sizeof(TypedParameter));
    param_list->params[0]->name = strdup("param1");
    param_list->params[0]->type = PARAM_TYPE_INTEGER;
    param_list->params[0]->value.int_value = 42;
    param_list->params[1] = calloc(1, sizeof(TypedParameter));
    param_list->params[1]->name = strdup("param2");
    param_list->params[1]->type = PARAM_TYPE_STRING;
    param_list->params[1]->value.string_value = strdup("test_value");
    param_list->count = 2;
    
    // Should clean up without crashing
    cleanup_auth_query_resources(NULL, NULL, NULL, param_list, NULL, NULL, 0, NULL);
    TEST_PASS();
}

// Test cleanup with converted_sql
void test_cleanup_auth_query_resources_with_converted_sql(void) {
    char *converted_sql = strdup("SELECT * FROM test WHERE id = $1");
    TEST_ASSERT_NOT_NULL(converted_sql);
    
    // Should clean up without crashing
    cleanup_auth_query_resources(NULL, NULL, NULL, NULL, converted_sql, NULL, 0, NULL);
    TEST_PASS();
}

// Test cleanup with ordered_params
void test_cleanup_auth_query_resources_with_ordered_params(void) {
    TypedParameter **ordered_params = calloc(3, sizeof(TypedParameter*));
    TEST_ASSERT_NOT_NULL(ordered_params);
    
    ordered_params[0] = calloc(1, sizeof(TypedParameter));
    ordered_params[0]->name = strdup("param1");
    ordered_params[0]->type = PARAM_TYPE_INTEGER;
    ordered_params[0]->value.int_value = 100;
    
    ordered_params[1] = NULL;  // Test NULL in middle
    
    ordered_params[2] = calloc(1, sizeof(TypedParameter));
    ordered_params[2]->name = strdup("param3");
    ordered_params[2]->type = PARAM_TYPE_STRING;
    ordered_params[2]->value.string_value = strdup("value3");
    
    // Should clean up without crashing
    cleanup_auth_query_resources(NULL, NULL, NULL, NULL, NULL, ordered_params, 3, NULL);
    TEST_PASS();
}

// Test cleanup with message
void test_cleanup_auth_query_resources_with_message(void) {
    char *message = strdup("Test message for cleanup");
    TEST_ASSERT_NOT_NULL(message);
    
    // Should clean up without crashing
    cleanup_auth_query_resources(NULL, NULL, NULL, NULL, NULL, NULL, 0, message);
    TEST_PASS();
}

// Test cleanup with all resources
void test_cleanup_auth_query_resources_with_all_resources(void) {
    // Create request_json
    json_t *request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(123));
    
    // Create jwt_result
    jwt_validation_result_t *jwt_result = calloc(1, sizeof(jwt_validation_result_t));
    jwt_result->valid = true;
    jwt_result->claims = calloc(1, sizeof(jwt_claims_t));
    jwt_result->claims->database = strdup("testdb");
    jwt_result->claims->username = strdup("testuser");
    
    // Create other resources
    char *query_id = strdup("query_123");
    char *converted_sql = strdup("SELECT * FROM test");
    char *message = strdup("Test message");
    
    // Create param_list
    ParameterList *param_list = calloc(1, sizeof(ParameterList));
    param_list->params = calloc(1, sizeof(TypedParameter*));
    param_list->params[0] = calloc(1, sizeof(TypedParameter));
    param_list->params[0]->name = strdup("p1");
    param_list->params[0]->type = PARAM_TYPE_INTEGER;
    param_list->count = 1;
    
    // Create ordered_params
    TypedParameter **ordered_params = calloc(2, sizeof(TypedParameter*));
    ordered_params[0] = calloc(1, sizeof(TypedParameter));
    ordered_params[0]->name = strdup("op1");
    ordered_params[0]->type = PARAM_TYPE_STRING;
    ordered_params[0]->value.string_value = strdup("val1");
    
    // Should clean up all resources without crashing
    cleanup_auth_query_resources(request_json, jwt_result, query_id, param_list, 
                                  converted_sql, ordered_params, 2, message);
    TEST_PASS();
}

// Test cleanup with partial resources (some NULL, some valid)
void test_cleanup_auth_query_resources_partial_resources(void) {
    // Create only some resources
    json_t *request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(456));
    
    char *message = strdup("Partial cleanup test");
    
    // Leave other parameters NULL
    // Should clean up without crashing
    cleanup_auth_query_resources(request_json, NULL, NULL, NULL, 
                                  NULL, NULL, 0, message);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_cleanup_auth_query_resources_all_null);
    RUN_TEST(test_cleanup_auth_query_resources_with_request_json);
    RUN_TEST(test_cleanup_auth_query_resources_with_jwt_result);
    RUN_TEST(test_cleanup_auth_query_resources_with_query_id);
    RUN_TEST(test_cleanup_auth_query_resources_with_param_list);
    RUN_TEST(test_cleanup_auth_query_resources_with_converted_sql);
    RUN_TEST(test_cleanup_auth_query_resources_with_ordered_params);
    RUN_TEST(test_cleanup_auth_query_resources_with_message);
    RUN_TEST(test_cleanup_auth_query_resources_with_all_resources);
    RUN_TEST(test_cleanup_auth_query_resources_partial_resources);

    return UNITY_END();
}
