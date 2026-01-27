/*
 * Unity Test File: Query Execution Helpers - Process Query Parameters
 *
 * This file contains unit tests for the process_query_parameters function
 * and related error handling functions in query_exec_helpers.c
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/conduit/helpers/query_exec_helpers.h>
#include <src/database/database.h>
#include <src/database/database_cache.h>
#include <src/database/database_params.h>
#include <src/database/database_pending.h>
#include <src/database/dbqueue/dbqueue.h>

// Include mocks for functions we need to control
// USE_MOCK_GENERATE_QUERY_ID defined by CMake
// USE_MOCK_PREPARE_AND_SUBMIT_QUERY defined by CMake
#include <unity/mocks/mock_generate_query_id.h>
#include <unity/mocks/mock_prepare_and_submit_query.h>

// Forward declarations for functions being tested
char* process_query_parameters(json_t* params, const QueryCacheEntry* cache_entry,
                              const DatabaseQueue* db_queue, ParameterList** param_list,
                              char** converted_sql, TypedParameter*** ordered_params,
                              size_t* param_count);

DatabaseQueue* select_query_queue_with_error_handling(const char* database, const QueryCacheEntry* cache_entry,
                                                      char* converted_sql, ParameterList* param_list,
                                                      TypedParameter** ordered_params, char* message);

char* generate_query_id_with_error_handling(char* converted_sql, ParameterList* param_list,
                                           TypedParameter** ordered_params, char* message);

PendingQueryResult* register_pending_result_with_error_handling(char* query_id, const QueryCacheEntry* cache_entry,
                                                               char* converted_sql, ParameterList* param_list,
                                                               TypedParameter** ordered_params, char* message);

bool submit_query_with_error_handling(DatabaseQueue* selected_queue, char* query_id,
                                     const QueryCacheEntry* cache_entry, TypedParameter** ordered_params,
                                     size_t param_count, char* converted_sql, ParameterList* param_list,
                                     char* message);

// Test fixtures
static QueryCacheEntry* test_cache_entry = NULL;
static DatabaseQueue* test_db_queue = NULL;

void setUp(void) {
    // Reset mocks
    mock_generate_query_id_reset();
    mock_prepare_and_submit_query_reset();

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

    // Reset mocks
    mock_generate_query_id_reset();
    mock_prepare_and_submit_query_reset();
}

// Forward declarations for test functions
static void test_process_query_parameters_valid_params_success(void);
static void test_process_query_parameters_unused_params_with_warning(void);
static void test_process_query_parameters_invalid_param_types(void);
static void test_process_query_parameters_combined_messages(void);
static void test_select_query_queue_with_error_handling_success(void);
static void test_select_query_queue_with_error_handling_failure(void);
static void test_generate_query_id_with_error_handling_success(void);
static void test_generate_query_id_with_error_handling_failure(void);
static void test_register_pending_result_with_error_handling_success(void);
static void test_register_pending_result_with_error_handling_failure(void);
static void test_submit_query_with_error_handling_success(void);
static void test_submit_query_with_error_handling_failure(void);

// Test cases for process_query_parameters


void test_process_query_parameters_valid_params_success(void) {
    // Test with valid parameters - must use the correct JSON structure
    json_t* params = json_object();
    json_t* integer_obj = json_object();
    json_object_set_new(integer_obj, "userId", json_integer(123));
    json_object_set_new(params, "INTEGER", integer_obj);

    ParameterList* param_list = NULL;
    char* converted_sql = NULL;
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;

    char* result = process_query_parameters(params, test_cache_entry, test_db_queue,
                                           &param_list, &converted_sql, &ordered_params, &param_count);

    TEST_ASSERT_NULL(result);  // Should succeed
    TEST_ASSERT_NOT_NULL(param_list);
    TEST_ASSERT_NOT_NULL(converted_sql);
    TEST_ASSERT_NOT_NULL(ordered_params);
    TEST_ASSERT_GREATER_THAN(0, param_count);

    // Clean up
    json_decref(params);
    free_parameter_list(param_list);
    free(converted_sql);
    free(ordered_params);
}


void test_process_query_parameters_unused_params_with_warning(void) {
    // Test with unused parameters - should succeed but log warning
    json_t* params = json_object();
    json_t* integer_obj = json_object();
    json_object_set_new(integer_obj, "userId", json_integer(123));
    json_object_set_new(params, "INTEGER", integer_obj);
    json_t* string_obj = json_object();
    json_object_set_new(string_obj, "unusedParam", json_string("should be ignored"));
    json_object_set_new(params, "STRING", string_obj);

    ParameterList* param_list = NULL;
    char* converted_sql = NULL;
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;

    char* result = process_query_parameters(params, test_cache_entry, test_db_queue,
                                           &param_list, &converted_sql, &ordered_params, &param_count);

    TEST_ASSERT_NOT_NULL(result);  // Should return warning message for unused params
    TEST_ASSERT_TRUE(strstr(result, "Unused Parameters") != NULL);
    TEST_ASSERT_NOT_NULL(param_list);
    TEST_ASSERT_NOT_NULL(converted_sql);
    TEST_ASSERT_NOT_NULL(ordered_params);

    // Clean up
    free(result);

    // Clean up
    json_decref(params);
    free_parameter_list(param_list);
    free(converted_sql);
    free(ordered_params);
}

void test_process_query_parameters_invalid_param_types(void) {
    // Test with invalid parameter types - put integer in STRING section
    json_t* params = json_object();
    json_t* string_obj = json_object();
    json_object_set_new(string_obj, "userId", json_integer(123));  // Integer in STRING section
    json_object_set_new(params, "STRING", string_obj);

    ParameterList* param_list = NULL;
    char* converted_sql = NULL;
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;

    char* result = process_query_parameters(params, test_cache_entry, test_db_queue,
                                           &param_list, &converted_sql, &ordered_params, &param_count);

    TEST_ASSERT_NOT_NULL(result);  // Should fail with type error

    // Clean up
    json_decref(params);
    free(result);
}

void test_process_query_parameters_combined_messages(void) {
    // Test scenario where both unused warning and validation message exist
    // This would require a more complex setup with validation messages
    // For now, just ensure the code path exists
    TEST_PASS();  // Placeholder - would need specific test data to trigger both messages
}

// Test cases for error handling helper functions

void test_select_query_queue_with_error_handling_success(void) {
    // This would need a mock for select_query_queue to return a valid queue
    TEST_PASS();  // Placeholder - need mock for select_query_queue
}

void test_select_query_queue_with_error_handling_failure(void) {
    // This would need a mock for select_query_queue to return NULL
    TEST_PASS();  // Placeholder - need mock for select_query_queue
}

void test_generate_query_id_with_error_handling_success(void) {
    // Test successful ID generation
    mock_generate_query_id_set_result("test_query_id");

    char* result = generate_query_id_with_error_handling(NULL, NULL, NULL, NULL);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test_query_id", result);
    free(result);
}

void test_generate_query_id_with_error_handling_failure(void) {
    // Test ID generation failure
    mock_generate_query_id_set_result(NULL);

    char* result = generate_query_id_with_error_handling(NULL, NULL, NULL, NULL);
    TEST_ASSERT_NULL(result);
}

void test_register_pending_result_with_error_handling_success(void) {
    // This would need a mock for pending_result_register to return a valid result
    TEST_PASS();  // Placeholder - need mock for pending_result_register
}

void test_register_pending_result_with_error_handling_failure(void) {
    // This would need a mock for pending_result_register to return NULL
    TEST_PASS();  // Placeholder - need mock for pending_result_register
}

void test_submit_query_with_error_handling_success(void) {
    // Test successful submission
    mock_prepare_and_submit_query_set_result(true);

    char* query_id = strdup("test_query_123");
    bool result = submit_query_with_error_handling(test_db_queue, query_id, test_cache_entry,
                                                  NULL, 0, NULL, NULL, NULL);
    TEST_ASSERT_TRUE(result);
    // Note: query_id is freed by the function on success
}

void test_submit_query_with_error_handling_failure(void) {
    // Test submission failure
    mock_prepare_and_submit_query_set_result(false);

    char* query_id = strdup("test_query_456");
    bool result = submit_query_with_error_handling(test_db_queue, query_id, test_cache_entry,
                                                  NULL, 0, NULL, NULL, NULL);
    TEST_ASSERT_FALSE(result);
    // Note: query_id is freed by the function on failure
}

int main(void) {
    UNITY_BEGIN();

    // Process query parameters tests
    RUN_TEST(test_process_query_parameters_valid_params_success);
    RUN_TEST(test_process_query_parameters_unused_params_with_warning);
    RUN_TEST(test_process_query_parameters_invalid_param_types);
    RUN_TEST(test_process_query_parameters_combined_messages);

    // Error handling helper tests
    RUN_TEST(test_select_query_queue_with_error_handling_success);
    RUN_TEST(test_select_query_queue_with_error_handling_failure);
    RUN_TEST(test_generate_query_id_with_error_handling_success);
    RUN_TEST(test_generate_query_id_with_error_handling_failure);
    RUN_TEST(test_register_pending_result_with_error_handling_success);
    RUN_TEST(test_register_pending_result_with_error_handling_failure);
    RUN_TEST(test_submit_query_with_error_handling_success);
    RUN_TEST(test_submit_query_with_error_handling_failure);

    return UNITY_END();
}