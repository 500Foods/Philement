/*
 * Unity Test File: Query Execution Helpers - Cleanup Functions
 *
 * This file contains unit tests for the cleanup functions in query_exec_helpers.c
 * including cleanup_query_execution_resources and cleanup_ordered_params.
 *
 * CHANGELOG:
 * 2026-02-18: Initial creation of unit tests for cleanup functions
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/conduit/helpers/query_exec_helpers.h>
#include <src/database/database.h>
#include <src/database/database_cache.h>
#include <src/database/database_params.h>

// Forward declarations for functions being tested
void cleanup_query_execution_resources(ParameterList* param_list, char* converted_sql,
                                       TypedParameter** ordered_params, size_t param_count,
                                       char* query_id, char* message);
void cleanup_ordered_params(TypedParameter** ordered_params, size_t param_count);

// Forward declarations for test functions
void test_cleanup_query_execution_resources_all_null(void);
void test_cleanup_query_execution_resources_with_param_list(void);
void test_cleanup_query_execution_resources_with_converted_sql(void);
void test_cleanup_query_execution_resources_with_query_id(void);
void test_cleanup_query_execution_resources_with_message(void);
void test_cleanup_query_execution_resources_with_ordered_params(void);
void test_cleanup_query_execution_resources_all_resources(void);
void test_cleanup_ordered_params_null(void);
void test_cleanup_ordered_params_empty_array(void);
void test_cleanup_ordered_params_valid_params(void);
void test_cleanup_ordered_params_all_null_entries(void);
void test_cleanup_ordered_params_single_param(void);
void test_cleanup_ordered_params_various_types(void);

// Test fixtures
static QueryCacheEntry* test_cache_entry = NULL;
static DatabaseQueue* test_db_queue = NULL;

void setUp(void) {
    // Create test fixtures
    test_cache_entry = calloc(1, sizeof(QueryCacheEntry));
    TEST_ASSERT_NOT_NULL(test_cache_entry);
    test_cache_entry->sql_template = strdup("SELECT * FROM test WHERE id = :userId");
    test_cache_entry->queue_type = strdup("fast");
    test_cache_entry->timeout_seconds = 30;

    test_db_queue = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(test_db_queue);
    test_db_queue->engine_type = DB_ENGINE_POSTGRESQL;
    test_db_queue->database_name = strdup("test_db");
}

void tearDown(void) {
    // Clean up test fixtures
    if (test_cache_entry) {
        free(test_cache_entry->sql_template);
        free(test_cache_entry->queue_type);
        free(test_cache_entry);
        test_cache_entry = NULL;
    }
    if (test_db_queue) {
        free(test_db_queue->database_name);
        free(test_db_queue);
        test_db_queue = NULL;
    }
}

// Test cleanup_query_execution_resources with all NULL parameters
void test_cleanup_query_execution_resources_all_null(void) {
    // Should not crash when all parameters are NULL
    cleanup_query_execution_resources(NULL, NULL, NULL, 0, NULL, NULL);
    TEST_PASS();
}

// Test cleanup_query_execution_resources with valid param_list
void test_cleanup_query_execution_resources_with_param_list(void) {
    ParameterList* param_list = calloc(1, sizeof(ParameterList));
    TEST_ASSERT_NOT_NULL(param_list);
    
    param_list->params = calloc(2, sizeof(TypedParameter*));
    TEST_ASSERT_NOT_NULL(param_list->params);
    
    param_list->params[0] = calloc(1, sizeof(TypedParameter));
    TEST_ASSERT_NOT_NULL(param_list->params[0]);
    param_list->params[0]->name = strdup("param1");
    param_list->params[0]->type = PARAM_TYPE_INTEGER;
    param_list->params[0]->value.int_value = 42;
    
    param_list->params[1] = calloc(1, sizeof(TypedParameter));
    TEST_ASSERT_NOT_NULL(param_list->params[1]);
    param_list->params[1]->name = strdup("param2");
    param_list->params[1]->type = PARAM_TYPE_STRING;
    param_list->params[1]->value.string_value = strdup("test_value");
    
    param_list->count = 2;
    
    // Should clean up without crashing
    cleanup_query_execution_resources(param_list, NULL, NULL, 0, NULL, NULL);
    TEST_PASS();
}

// Test cleanup_query_execution_resources with converted_sql
void test_cleanup_query_execution_resources_with_converted_sql(void) {
    char* converted_sql = strdup("SELECT * FROM test WHERE id = $1");
    TEST_ASSERT_NOT_NULL(converted_sql);
    
    // Should clean up without crashing
    cleanup_query_execution_resources(NULL, converted_sql, NULL, 0, NULL, NULL);
    TEST_PASS();
}

// Test cleanup_query_execution_resources with query_id
void test_cleanup_query_execution_resources_with_query_id(void) {
    char* query_id = strdup("test_query_12345");
    TEST_ASSERT_NOT_NULL(query_id);
    
    // Should clean up without crashing
    cleanup_query_execution_resources(NULL, NULL, NULL, 0, query_id, NULL);
    TEST_PASS();
}

// Test cleanup_query_execution_resources with message
void test_cleanup_query_execution_resources_with_message(void) {
    char* message = strdup("Test message for cleanup");
    TEST_ASSERT_NOT_NULL(message);
    
    // Should clean up without crashing
    cleanup_query_execution_resources(NULL, NULL, NULL, 0, NULL, message);
    TEST_PASS();
}

// Test cleanup_query_execution_resources with ordered_params
void test_cleanup_query_execution_resources_with_ordered_params(void) {
    TypedParameter** ordered_params = calloc(3, sizeof(TypedParameter*));
    TEST_ASSERT_NOT_NULL(ordered_params);
    
    ordered_params[0] = calloc(1, sizeof(TypedParameter));
    TEST_ASSERT_NOT_NULL(ordered_params[0]);
    ordered_params[0]->name = strdup("param1");
    ordered_params[0]->type = PARAM_TYPE_INTEGER;
    ordered_params[0]->value.int_value = 100;
    
    ordered_params[1] = NULL;  // Test with NULL in middle
    
    ordered_params[2] = calloc(1, sizeof(TypedParameter));
    TEST_ASSERT_NOT_NULL(ordered_params[2]);
    ordered_params[2]->name = strdup("param3");
    ordered_params[2]->type = PARAM_TYPE_STRING;
    ordered_params[2]->value.string_value = strdup("value3");
    
    // Should clean up without crashing
    cleanup_query_execution_resources(NULL, NULL, ordered_params, 3, NULL, NULL);
    TEST_PASS();
}

// Test cleanup_query_execution_resources with all resources
void test_cleanup_query_execution_resources_all_resources(void) {
    // Create param_list
    ParameterList* param_list = calloc(1, sizeof(ParameterList));
    TEST_ASSERT_NOT_NULL(param_list);
    param_list->params = calloc(1, sizeof(TypedParameter*));
    param_list->params[0] = calloc(1, sizeof(TypedParameter));
    param_list->params[0]->name = strdup("p1");
    param_list->params[0]->type = PARAM_TYPE_INTEGER;
    param_list->count = 1;
    
    // Create other resources
    char* converted_sql = strdup("SELECT * FROM test");
    char* query_id = strdup("query_123");
    char* message = strdup("Test message");
    
    TypedParameter** ordered_params = calloc(2, sizeof(TypedParameter*));
    ordered_params[0] = calloc(1, sizeof(TypedParameter));
    ordered_params[0]->name = strdup("op1");
    ordered_params[0]->type = PARAM_TYPE_STRING;
    ordered_params[0]->value.string_value = strdup("val1");
    
    // Should clean up all resources without crashing
    cleanup_query_execution_resources(param_list, converted_sql, ordered_params, 2, query_id, message);
    TEST_PASS();
}

// Test cleanup_ordered_params with NULL
void test_cleanup_ordered_params_null(void) {
    // Should not crash with NULL
    cleanup_ordered_params(NULL, 0);
    TEST_PASS();
}

// Test cleanup_ordered_params with empty array (use 1 element to avoid alloc-size error)
void test_cleanup_ordered_params_empty_array(void) {
    // Test with single NULL entry instead of 0-size allocation
    TypedParameter** ordered_params = calloc(1, sizeof(TypedParameter*));
    TEST_ASSERT_NOT_NULL(ordered_params);
    ordered_params[0] = NULL;
    
    // Should not crash
    cleanup_ordered_params(ordered_params, 1);
    TEST_PASS();
}

// Test cleanup_ordered_params with valid parameters
void test_cleanup_ordered_params_valid_params(void) {
    TypedParameter** ordered_params = calloc(3, sizeof(TypedParameter*));
    TEST_ASSERT_NOT_NULL(ordered_params);
    
    // First parameter
    ordered_params[0] = calloc(1, sizeof(TypedParameter));
    TEST_ASSERT_NOT_NULL(ordered_params[0]);
    ordered_params[0]->name = strdup("id");
    ordered_params[0]->type = PARAM_TYPE_INTEGER;
    ordered_params[0]->value.int_value = 42;
    
    // Second parameter with string value
    ordered_params[1] = calloc(1, sizeof(TypedParameter));
    TEST_ASSERT_NOT_NULL(ordered_params[1]);
    ordered_params[1]->name = strdup("name");
    ordered_params[1]->type = PARAM_TYPE_STRING;
    ordered_params[1]->value.string_value = strdup("John Doe");
    
    // Third parameter with NULL value
    ordered_params[2] = NULL;
    
    // Should clean up without crashing
    cleanup_ordered_params(ordered_params, 3);
    TEST_PASS();
}

// Test cleanup_ordered_params with all NULL entries
void test_cleanup_ordered_params_all_null_entries(void) {
    TypedParameter** ordered_params = calloc(5, sizeof(TypedParameter*));
    TEST_ASSERT_NOT_NULL(ordered_params);
    
    // All entries are NULL (calloc already zeros memory)
    
    // Should not crash with all NULL entries
    cleanup_ordered_params(ordered_params, 5);
    TEST_PASS();
}

// Test cleanup_ordered_params with single parameter
void test_cleanup_ordered_params_single_param(void) {
    TypedParameter** ordered_params = calloc(1, sizeof(TypedParameter*));
    TEST_ASSERT_NOT_NULL(ordered_params);
    
    ordered_params[0] = calloc(1, sizeof(TypedParameter));
    TEST_ASSERT_NOT_NULL(ordered_params[0]);
    ordered_params[0]->name = strdup("single_param");
    ordered_params[0]->type = PARAM_TYPE_INTEGER;
    ordered_params[0]->value.int_value = 999;
    
    // Should clean up without crashing
    cleanup_ordered_params(ordered_params, 1);
    TEST_PASS();
}

// Test cleanup_ordered_params with various parameter types
void test_cleanup_ordered_params_various_types(void) {
    TypedParameter** ordered_params = calloc(4, sizeof(TypedParameter*));
    TEST_ASSERT_NOT_NULL(ordered_params);
    
    // Integer parameter
    ordered_params[0] = calloc(1, sizeof(TypedParameter));
    ordered_params[0]->name = strdup("int_param");
    ordered_params[0]->type = PARAM_TYPE_INTEGER;
    ordered_params[0]->value.int_value = 100;
    
    // String parameter
    ordered_params[1] = calloc(1, sizeof(TypedParameter));
    ordered_params[1]->name = strdup("str_param");
    ordered_params[1]->type = PARAM_TYPE_STRING;
    ordered_params[1]->value.string_value = strdup("test string");
    
    // Boolean parameter
    ordered_params[2] = calloc(1, sizeof(TypedParameter));
    ordered_params[2]->name = strdup("bool_param");
    ordered_params[2]->type = PARAM_TYPE_BOOLEAN;
    ordered_params[2]->value.bool_value = true;
    
    // NULL parameter
    ordered_params[3] = NULL;
    
    // Should clean up without crashing
    cleanup_ordered_params(ordered_params, 4);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    // cleanup_query_execution_resources tests
    RUN_TEST(test_cleanup_query_execution_resources_all_null);
    RUN_TEST(test_cleanup_query_execution_resources_with_param_list);
    RUN_TEST(test_cleanup_query_execution_resources_with_converted_sql);
    RUN_TEST(test_cleanup_query_execution_resources_with_query_id);
    RUN_TEST(test_cleanup_query_execution_resources_with_message);
    RUN_TEST(test_cleanup_query_execution_resources_with_ordered_params);
    RUN_TEST(test_cleanup_query_execution_resources_all_resources);

    // cleanup_ordered_params tests
    RUN_TEST(test_cleanup_ordered_params_null);
    RUN_TEST(test_cleanup_ordered_params_empty_array);
    RUN_TEST(test_cleanup_ordered_params_valid_params);
    RUN_TEST(test_cleanup_ordered_params_all_null_entries);
    RUN_TEST(test_cleanup_ordered_params_single_param);
    RUN_TEST(test_cleanup_ordered_params_various_types);

    return UNITY_END();
}
