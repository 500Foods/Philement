/*
 * Unity Test File: Conduit Queries Execute Single Query
 * This file contains unit tests for the execute_single_query function
 * in src/api/conduit/queries/queries.c
 *
 * CHANGELOG:
 * 2026-01-15: Initial creation of unit tests for execute_single_query
 * 2026-08-03: Implemented all 8 tests using available mock infrastructure
 *
 * TEST_VERSION: 1.1.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for testing
#define USE_MOCK_LAUNCH
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_launch.h>
#include <unity/mocks/mock_system.h>

// Enable mock database queue functions for database lookup mocking
#include <unity/mocks/mock_dbqueue.h>
// Enable mock for generate_query_id and prepare_and_submit_query
#include <unity/mocks/mock_generate_query_id.h>
#include <unity/mocks/mock_prepare_and_submit_query.h>

// Include source header
#include <src/api/conduit/queries/queries.h>

extern DatabaseQueueManager* global_queue_manager;

// Static mock objects for database queue and cache setup
static QueryCacheEntry mock_cache_entry;
static QueryTableCache mock_query_cache;
static DatabaseQueue mock_db_queue;
static DatabaseQueue* mock_db_queue_ptr;
static DatabaseQueueManager mock_queue_manager;

// Saved state for global_queue_manager restoration
static DatabaseQueueManager* saved_global_queue_manager;

void test_execute_single_query_null_database(void);
void test_execute_single_query_null_query_obj(void);
void test_execute_single_query_missing_query_ref(void);
void test_execute_single_query_invalid_query_ref_type(void);
void test_execute_single_query_database_not_found(void);
void test_execute_single_query_query_not_found(void);
void test_execute_single_query_parameter_type_validation_failure(void);
void test_execute_single_query_missing_parameters(void);
void test_execute_single_query_parameter_processing_failure(void);
void test_execute_single_query_unused_parameters(void);
void test_execute_single_query_queue_selection_failure(void);
void test_execute_single_query_query_id_generation_failure(void);
void test_execute_single_query_pending_registration_failure(void);
void test_execute_single_query_query_submission_failure(void);

void setUp(void) {
    mock_launch_reset_all();
    mock_system_reset_all();
    mock_dbqueue_reset_all();
    mock_generate_query_id_reset();
    mock_prepare_and_submit_query_reset();

    saved_global_queue_manager = global_queue_manager;
    global_queue_manager = NULL;
}

void tearDown(void) {
    global_queue_manager = saved_global_queue_manager;
    mock_launch_reset_all();
    mock_system_reset_all();
    mock_dbqueue_reset_all();
    mock_generate_query_id_reset();
    mock_prepare_and_submit_query_reset();
}

static void setup_mock_db_with_cache(void) {
    memset(&mock_cache_entry, 0, sizeof(mock_cache_entry));
    mock_cache_entry.query_ref = 123;
    mock_cache_entry.query_type = 10;
    mock_cache_entry.sql_template = (char*)"SELECT 1";
    mock_cache_entry.queue_type = (char*)"fast";
    mock_cache_entry.timeout_seconds = 30;

    memset(&mock_query_cache, 0, sizeof(mock_query_cache));

    memset(&mock_db_queue, 0, sizeof(mock_db_queue));
    mock_db_queue.database_name = (char*)"testdb";
    mock_db_queue.queue_type = (char*)"fast";
    mock_db_queue.engine_type = DB_ENGINE_POSTGRESQL;
    mock_db_queue.query_cache = &mock_query_cache;

    mock_db_queue_ptr = &mock_db_queue;

    memset(&mock_queue_manager, 0, sizeof(mock_queue_manager));
    mock_queue_manager.database_count = 1;
    mock_queue_manager.max_databases = 1;
    mock_queue_manager.databases = &mock_db_queue_ptr;

    mock_dbqueue_set_get_database_result(&mock_db_queue);
    mock_dbqueue_set_query_cache_lookup_by_ref_and_type_result(&mock_cache_entry);
}

static void setup_mock_db_with_global_queue(void) {
    setup_mock_db_with_cache();
    global_queue_manager = &mock_queue_manager;
}

static json_t* create_query_obj_with_params(int query_ref, json_t* params) {
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_integer(query_ref));
    if (params) {
        json_object_set_new(query_obj, "params", params);
    }
    return query_obj;
}

// Test execute_single_query with NULL database
void test_execute_single_query_null_database(void) {
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_integer(123));

    json_t *result = execute_single_query(NULL, query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Invalid query object", json_string_value(json_object_get(result, "error")));

    json_decref(query_obj);
    json_decref(result);
}

// Test execute_single_query with NULL query_obj
void test_execute_single_query_null_query_obj(void) {
    json_t *result = execute_single_query("test_db", NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Invalid query object", json_string_value(json_object_get(result, "error")));

    json_decref(result);
}

// Test execute_single_query with missing query_ref
void test_execute_single_query_missing_query_ref(void) {
    json_t *query_obj = json_object();

    json_t *result = execute_single_query("test_db", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Missing required field: query_ref", json_string_value(json_object_get(result, "error")));

    json_decref(query_obj);
    json_decref(result);
}

// Test execute_single_query with invalid query_ref type
void test_execute_single_query_invalid_query_ref_type(void) {
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_string("not_a_number"));

    json_t *result = execute_single_query("test_db", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Missing required field: query_ref", json_string_value(json_object_get(result, "error")));

    json_decref(query_obj);
    json_decref(result);
}

// Test execute_single_query with database not found
void test_execute_single_query_database_not_found(void) {
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_integer(123));

    json_t *result = execute_single_query("nonexistent_db", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Database not available", json_string_value(json_object_get(result, "error")));

    json_decref(query_obj);
    json_decref(result);
}

// Test execute_single_query with query not found
void test_execute_single_query_query_not_found(void) {
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_integer(99999));

    json_t *result = execute_single_query("testdb", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Database not available", json_string_value(json_object_get(result, "error")));

    json_decref(query_obj);
    json_decref(result);
}

// Test parameter type validation failure
// Parameters under type keys must match their expected JSON type.
// A string value under "INTEGER" is a type mismatch.
void test_execute_single_query_parameter_type_validation_failure(void) {
    setup_mock_db_with_cache();

    mock_cache_entry.sql_template = (char*)"SELECT :INTEGER:param1";

    json_t *params = json_object();
    json_t *integer_params = json_object();
    json_object_set_new(integer_params, "param1", json_string("not_an_integer"));
    json_object_set_new(params, "INTEGER", integer_params);

    json_t *query_obj = create_query_obj_with_params(123, params);

    json_t *result = execute_single_query("testdb", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Parameter processing failed", json_string_value(json_object_get(result, "error")));
    json_t *message = json_object_get(result, "message");
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_NOT_NULL(strstr(json_string_value(message), "param1"));
    TEST_ASSERT_NOT_NULL(strstr(json_string_value(message), "string"));

    json_decref(query_obj);
    json_decref(result);
}

// Test missing parameters
// SQL template requires :INTEGER:param1 but params object is empty.
void test_execute_single_query_missing_parameters(void) {
    setup_mock_db_with_cache();

    mock_cache_entry.sql_template = (char*)"SELECT :INTEGER:param1";

    json_t *params = json_object();

    json_t *query_obj = create_query_obj_with_params(123, params);

    json_t *result = execute_single_query("testdb", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Parameter processing failed", json_string_value(json_object_get(result, "error")));
    json_t *message = json_object_get(result, "message");
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_NOT_NULL(strstr(json_string_value(message), "param1"));

    json_decref(query_obj);
    json_decref(result);
}

// Test parameter processing failure
// Inject a calloc failure at the calloc call inside process_parameters
// by setting mock_calloc_should_fail to the call index. With a simple
// "SELECT 1" template and NULL params, the calloc call sequence is:
//   #1 calloc in process_query_parameters (temp_param_list)
//   #2 calloc in process_parameters (param_list) — this one fails
void test_execute_single_query_parameter_processing_failure(void) {
    setup_mock_db_with_cache();

    mock_cache_entry.sql_template = (char*)"SELECT 1";
    mock_cache_entry.queue_type = (char*)"fast";

    json_t *query_obj = create_query_obj_with_params(123, NULL);

    mock_system_set_calloc_failure(2);

    json_t *result = execute_single_query("testdb", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Parameter processing failed", json_string_value(json_object_get(result, "error")));
    json_t *message = json_object_get(result, "message");
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_EQUAL_STRING("Parameter processing failed", json_string_value(message));

    json_decref(query_obj);
    json_decref(result);
}

// Test unused parameters
// SQL template "SELECT 1" has no parameters, but params object
// provides an unused parameter. check_unused_parameters_simple detects this.
void test_execute_single_query_unused_parameters(void) {
    setup_mock_db_with_cache();

    mock_cache_entry.sql_template = (char*)"SELECT 1";

    json_t *params = json_object();
    json_t *integer_params = json_object();
    json_object_set_new(integer_params, "unused_param", json_integer(42));
    json_object_set_new(params, "INTEGER", integer_params);

    json_t *query_obj = create_query_obj_with_params(123, params);

    json_t *result = execute_single_query("testdb", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Parameter processing failed", json_string_value(json_object_get(result, "error")));
    json_t *message = json_object_get(result, "message");
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_NOT_NULL(strstr(json_string_value(message), "unused_param"));

    json_decref(query_obj);
    json_decref(result);
}

// Test queue selection failure
// All preceding steps succeed, but select_query_queue returns NULL
// because global_queue_manager is NULL (select_optimal_queue returns NULL).
void test_execute_single_query_queue_selection_failure(void) {
    setup_mock_db_with_cache();

    mock_cache_entry.sql_template = (char*)"SELECT 1";
    mock_cache_entry.queue_type = (char*)"fast";
    // global_queue_manager is NOT set (remains NULL), so select_query_queue fails

    json_t *query_obj = create_query_obj_with_params(123, NULL);

    json_t *result = execute_single_query("testdb", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("No suitable queue available", json_string_value(json_object_get(result, "error")));

    json_decref(query_obj);
    json_decref(result);
}

// Test query ID generation failure
// All preceding steps succeed including queue selection, but
// generate_query_id returns NULL (mocked).
void test_execute_single_query_query_id_generation_failure(void) {
    setup_mock_db_with_global_queue();

    mock_cache_entry.sql_template = (char*)"SELECT 1";
    mock_cache_entry.queue_type = (char*)"fast";

    mock_generate_query_id_set_result(NULL);

    json_t *query_obj = create_query_obj_with_params(123, NULL);

    json_t *result = execute_single_query("testdb", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Failed to generate query ID", json_string_value(json_object_get(result, "error")));

    json_decref(query_obj);
    json_decref(result);
}

// Test pending registration failure
// All preceding steps succeed including queue selection and query ID
// generation, but pending_result_register returns NULL.
// This is achieved by injecting a calloc failure at the calloc call
// inside pending_result_manager_create (call #5 in the sequence:
//   #1 calloc in process_query_parameters
//   #2 calloc in process_parameters
//   #3 strdup in convert_named_to_positional
//   #4 strdup in mock_generate_query_id
//   #5 calloc in pending_result_manager_create  ← fails here
void test_execute_single_query_pending_registration_failure(void) {
    setup_mock_db_with_global_queue();

    mock_cache_entry.sql_template = (char*)"SELECT 1";
    mock_cache_entry.queue_type = (char*)"fast";

    mock_generate_query_id_set_result("test_query_id");

    mock_system_set_calloc_failure(5);

    json_t *query_obj = create_query_obj_with_params(123, NULL);

    json_t *result = execute_single_query("testdb", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Failed to register pending result", json_string_value(json_object_get(result, "error")));

    json_decref(query_obj);
    json_decref(result);
}

// Test query submission failure
// All preceding steps succeed, but prepare_and_submit_query returns false (mocked).
void test_execute_single_query_query_submission_failure(void) {
    setup_mock_db_with_global_queue();

    mock_cache_entry.sql_template = (char*)"SELECT 1";
    mock_cache_entry.queue_type = (char*)"fast";

    mock_generate_query_id_set_result("test_query_id");

    mock_prepare_and_submit_query_set_result(false);

    json_t *query_obj = create_query_obj_with_params(123, NULL);

    json_t *result = execute_single_query("testdb", query_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(result, "success")));
    TEST_ASSERT_EQUAL_STRING("Failed to submit query", json_string_value(json_object_get(result, "error")));

    json_decref(query_obj);
    json_decref(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_execute_single_query_null_database);
    RUN_TEST(test_execute_single_query_null_query_obj);
    RUN_TEST(test_execute_single_query_missing_query_ref);
    RUN_TEST(test_execute_single_query_invalid_query_ref_type);
    RUN_TEST(test_execute_single_query_database_not_found);
    RUN_TEST(test_execute_single_query_query_not_found);
    RUN_TEST(test_execute_single_query_parameter_type_validation_failure);
    RUN_TEST(test_execute_single_query_missing_parameters);
    RUN_TEST(test_execute_single_query_parameter_processing_failure);
    RUN_TEST(test_execute_single_query_unused_parameters);
    RUN_TEST(test_execute_single_query_queue_selection_failure);
    RUN_TEST(test_execute_single_query_query_id_generation_failure);
    RUN_TEST(test_execute_single_query_pending_registration_failure);
    RUN_TEST(test_execute_single_query_query_submission_failure);

    return UNITY_END();
}
