/*
 * Unity Test File: Test build_success_response function
 * This file contains unit tests for src/api/conduit/query/query.c build_success_response function
 */
                         
#include <src/hydrogen.h>
#include "unity.h"

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/database_cache.h>
// Forward declaration for the function being tested
json_t* build_success_response(int query_ref, const QueryCacheEntry* cache_entry,
                               const QueryResult* result, const DatabaseQueue* selected_queue);

// Mock structures for testing
typedef struct MockQueryCacheEntry {
    int query_ref;
    char* sql_template;
    char* description;
    char* queue_type;
    int timeout_seconds;
    time_t last_used;
    volatile int usage_count;
} MockQueryCacheEntry;

typedef struct MockQueryResult {
    bool success;
    char* data_json;
    size_t row_count;
    size_t column_count;
    char** column_names;
    char* error_message;
    time_t execution_time_ms;
    int affected_rows;
} MockQueryResult;

typedef struct MockDatabaseQueue {
    char* database_name;
    char* connection_string;
    DatabaseEngine engine_type;
    char* queue_type;
} MockDatabaseQueue;

void setUp(void) {
    // No specific setup
}

void tearDown(void) {
    // No specific teardown
}

// Test function prototypes
void test_build_success_response_basic_empty(void);
void test_build_success_response_with_data(void);
void test_build_success_response_null_description(void);
// Test basic success response with empty data
void test_build_success_response_basic_empty(void) {
    int query_ref = 123;
    char desc1[] = "Test query";
    char queue1[] = "fast";
    MockQueryCacheEntry cache_entry = {123, NULL, desc1, queue1, 30, 0, 0};
    MockQueryResult result = {true, NULL, 0, 0, NULL, NULL, 100, 0};
    char queue_type1[] = "fast";
    MockDatabaseQueue selected_queue = { .database_name = NULL, .connection_string = NULL, .engine_type = DB_ENGINE_POSTGRESQL, .queue_type = queue_type1 };

    json_t* response = build_success_response(query_ref, (const QueryCacheEntry*)&cache_entry, (const QueryResult*)&result, (const DatabaseQueue*)&selected_queue);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    // Verify success
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_TRUE(json_is_true(success));

    // Verify query_ref
    json_t* ref = json_object_get(response, "query_ref");
    TEST_ASSERT_EQUAL_INT(123, json_integer_value(ref));

    // Verify description
    json_t* desc = json_object_get(response, "description");
    TEST_ASSERT_EQUAL_STRING("Test query", json_string_value(desc));

    // Verify rows (empty array)
    json_t* rows = json_object_get(response, "rows");
    TEST_ASSERT_TRUE(json_is_array(rows));
    TEST_ASSERT_EQUAL_INT(0, json_array_size(rows));

    // Verify counts
    json_t* row_count = json_object_get(response, "row_count");
    TEST_ASSERT_EQUAL_INT(0, json_integer_value(row_count));

    json_t* col_count = json_object_get(response, "column_count");
    TEST_ASSERT_EQUAL_INT(0, json_integer_value(col_count));

    // Verify execution time
    json_t* exec_time = json_object_get(response, "execution_time_ms");
    TEST_ASSERT_EQUAL_INT(100, json_integer_value(exec_time));

    // Verify queue
    json_t* queue = json_object_get(response, "queue_used");
    TEST_ASSERT_EQUAL_STRING("fast", json_string_value(queue));

    json_decref(response);
}

// Test with data_json present
void test_build_success_response_with_data(void) {
    int query_ref = 456;
    char queue2[] = "medium";
    MockQueryCacheEntry cache_entry = {456, NULL, NULL, queue2, 30, 0, 0};
    char data_json[] = "[{\"id\":1,\"name\":\"test\"}]";
    MockQueryResult result = {true, data_json, 1, 2, NULL, NULL, 200, 1};
    char queue_type2[] = "medium";
    MockDatabaseQueue selected_queue = { .database_name = NULL, .connection_string = NULL, .engine_type = DB_ENGINE_POSTGRESQL, .queue_type = queue_type2 };

    json_t* response = build_success_response(query_ref, (const QueryCacheEntry*)&cache_entry, (const QueryResult*)&result, (const DatabaseQueue*)&selected_queue);

    TEST_ASSERT_NOT_NULL(response);

    // Verify rows parsed from data_json
    json_t* rows = json_object_get(response, "rows");
    TEST_ASSERT_TRUE(json_is_array(rows));
    TEST_ASSERT_EQUAL_INT(1, json_array_size(rows));

    // Verify row_count and column_count
    json_t* row_count = json_object_get(response, "row_count");
    TEST_ASSERT_EQUAL_INT(1, json_integer_value(row_count));

    json_t* col_count = json_object_get(response, "column_count");
    TEST_ASSERT_EQUAL_INT(2, json_integer_value(col_count));

    // Description should be empty string if NULL
    json_t* desc = json_object_get(response, "description");
    TEST_ASSERT_EQUAL_STRING("", json_string_value(desc));

    json_decref(response);
}

// Test NULL cache_entry description handling
void test_build_success_response_null_description(void) {
    int query_ref = 789;
    char queue3[] = "slow";
    MockQueryCacheEntry cache_entry = {789, NULL, NULL, queue3, 30, 0, 0};
    MockQueryResult result = {true, NULL, 0, 0, NULL, NULL, 50, 0};
    char queue_type3[] = "slow";
    MockDatabaseQueue selected_queue = { .database_name = NULL, .connection_string = NULL, .engine_type = DB_ENGINE_POSTGRESQL, .queue_type = queue_type3 };

    json_t* response = build_success_response(query_ref, (const QueryCacheEntry*)&cache_entry, (const QueryResult*)&result, (const DatabaseQueue*)&selected_queue);

    TEST_ASSERT_NOT_NULL(response);

    json_t* desc = json_object_get(response, "description");
    TEST_ASSERT_EQUAL_STRING("", json_string_value(desc));

    json_decref(response);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_build_success_response_basic_empty);
    RUN_TEST(test_build_success_response_with_data);
    RUN_TEST(test_build_success_response_null_description);

    return UNITY_END();
}