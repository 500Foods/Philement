/*
 * Unity Test File: parameter_processing_test_handle_parameter_processing.c
 * This file contains unit tests for the handle_parameter_processing function
 */

// Standard includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <jansson.h>

// Enable mock database queue functions for unit testing
#define USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>

// Include source header
#include <src/api/conduit/conduit_helpers.h>

// Forward declarations for functions being tested
enum MHD_Result handle_parameter_processing(struct MHD_Connection *connection, json_t* params_json,
                                              const DatabaseQueue* db_queue, const QueryCacheEntry* cache_entry,
                                              const char* database, int query_ref,
                                              ParameterList** param_list, char** converted_sql,
                                              TypedParameter*** ordered_params, size_t* param_count,
                                              char** message);

void setUp(void) {
    // Reset all mocks to default state
    mock_dbqueue_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_dbqueue_reset_all();
}

// Test NULL db_queue
static void test_handle_parameter_processing_null_db_queue(void) {
    struct MHD_Connection *connection = NULL; // Mock connection
    json_t* params = json_object();
    json_t* integer_obj = json_object();
    json_object_set_new(integer_obj, "userId", json_integer(123));
    json_object_set_new(params, "INTEGER", integer_obj);

    QueryCacheEntry cache_entry = {
        .sql_template = (char*)"SELECT * FROM table WHERE id = :userId"
    };

    ParameterList* param_list = NULL;
    char* converted_sql = NULL;
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;
    char* message = NULL;

    enum MHD_Result result = handle_parameter_processing(connection, params, NULL, &cache_entry,
                                                        "test_db", 123, &param_list, &converted_sql,
                                                        &ordered_params, &param_count, &message);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(converted_sql);

    json_decref(params);
}

// Test valid parameter processing
static void test_handle_parameter_processing_valid_params(void) {
    struct MHD_Connection *connection = NULL; // Mock connection
    json_t* params = json_object();
    json_t* integer_obj = json_object();
    json_object_set_new(integer_obj, "userId", json_integer(123));
    json_object_set_new(params, "INTEGER", integer_obj);

    // Mock database queue
    DatabaseQueue db_queue = {0};
    db_queue.engine_type = DB_ENGINE_POSTGRESQL;

    QueryCacheEntry cache_entry = {
        .sql_template = (char*)"SELECT * FROM table WHERE id = :userId"
    };

    ParameterList* param_list = NULL;
    char* converted_sql = NULL;
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;
    char* message = NULL;

    enum MHD_Result result = handle_parameter_processing(connection, params, &db_queue, &cache_entry,
                                                        "test_db", 123, &param_list, &converted_sql,
                                                        &ordered_params, &param_count, &message);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(converted_sql);
    TEST_ASSERT_NOT_NULL(param_list);
    TEST_ASSERT_NOT_NULL(ordered_params);
    TEST_ASSERT_GREATER_THAN(0, param_count);

    // Cleanup
    if (param_list) {
        if (param_list->params) {
            for (size_t i = 0; i < param_list->count; i++) {
                if (param_list->params[i]) {
                    free(param_list->params[i]->name);
                    free(param_list->params[i]);
                }
            }
            free(param_list->params);
        }
        free(param_list);
    }
    free(converted_sql);
    free(ordered_params);

    json_decref(params);
}

// Test invalid parameter types
static void test_handle_parameter_processing_invalid_types(void) {
    struct MHD_Connection *connection = NULL; // Mock connection
    json_t* params = json_object();
    json_t* integer_obj = json_object();
    json_object_set_new(integer_obj, "userId", json_string("invalid"));
    json_object_set_new(params, "INTEGER", integer_obj);

    DatabaseQueue db_queue = {0};
    db_queue.engine_type = DB_ENGINE_POSTGRESQL;

    QueryCacheEntry cache_entry = {
        .sql_template = (char*)"SELECT * FROM table WHERE id = :userId"
    };

    ParameterList* param_list = NULL;
    char* converted_sql = NULL;
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;
    char* message = NULL;

    enum MHD_Result result = handle_parameter_processing(connection, params, &db_queue, &cache_entry,
                                                        "test_db", 123, &param_list, &converted_sql,
                                                        &ordered_params, &param_count, &message);

    TEST_ASSERT_EQUAL(MHD_YES, result); // Returns MHD_YES but with error response sent
    TEST_ASSERT_NULL(converted_sql);

    json_decref(params);
}

// Test missing parameters
static void test_handle_parameter_processing_missing_params(void) {
    struct MHD_Connection *connection = NULL; // Mock connection
    json_t* params = json_object();
    json_t* integer_obj = json_object();
    json_object_set_new(integer_obj, "userId", json_integer(123));
    json_object_set_new(params, "INTEGER", integer_obj);

    DatabaseQueue db_queue = {0};
    db_queue.engine_type = DB_ENGINE_POSTGRESQL;

    QueryCacheEntry cache_entry = {
        .sql_template = (char*)"SELECT * FROM table WHERE id = :userId AND name = :userName"
    };

    ParameterList* param_list = NULL;
    char* converted_sql = NULL;
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;
    char* message = NULL;

    enum MHD_Result result = handle_parameter_processing(connection, params, &db_queue, &cache_entry,
                                                        "test_db", 123, &param_list, &converted_sql,
                                                        &ordered_params, &param_count, &message);

    TEST_ASSERT_EQUAL(MHD_YES, result); // Returns MHD_YES but with error response sent
    TEST_ASSERT_NULL(converted_sql);

    json_decref(params);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_parameter_processing_null_db_queue);
    RUN_TEST(test_handle_parameter_processing_valid_params);
    RUN_TEST(test_handle_parameter_processing_invalid_types);
    RUN_TEST(test_handle_parameter_processing_missing_params);

    return UNITY_END();
}