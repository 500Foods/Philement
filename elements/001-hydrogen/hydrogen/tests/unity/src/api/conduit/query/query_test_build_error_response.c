/*
 * Unity Test File: build_error_response
 * This file contains unit tests for build_error_response function in query.c
 */

// Standard includes
#include <stdlib.h>
#include <string.h>

// Third-party includes
#include <jansson.h>

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/database/database_pending.h>
#include <src/database/database_cache.h>
#include <src/database/database_types.h>

// Include source header
#include <src/api/conduit/query/query.h>

// No mocks needed - use real implementations

// Helper functions to create dummy structures
static QueryCacheEntry* create_dummy_cache_entry(int timeout_seconds) {
    QueryCacheEntry* entry = calloc(1, sizeof(QueryCacheEntry));
    if (!entry) return NULL;
    entry->timeout_seconds = timeout_seconds;
    return entry;
}

static PendingQueryResult* create_dummy_pending(bool timed_out) {
    PendingQueryResult* pending = calloc(1, sizeof(PendingQueryResult));
    if (!pending) return NULL;
    pending->timed_out = timed_out;
    return pending;
}

static QueryResult* create_dummy_query_result(bool success, const char* error_msg) {
    QueryResult* result = calloc(1, sizeof(QueryResult));
    if (!result) return NULL;
    result->success = success;
    if (error_msg) {
        result->error_message = strdup(error_msg);
    }
    return result;
}

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed for global state
}

// Test timeout case
static void test_build_error_response_timeout(void) {
    int query_ref = 1;
    const char* database = "testdb";
    QueryCacheEntry* cache_entry = create_dummy_cache_entry(30);
    TEST_ASSERT_NOT_NULL(cache_entry);

    PendingQueryResult* pending = create_dummy_pending(true);
    TEST_ASSERT_NOT_NULL(pending);

    QueryResult* result = create_dummy_query_result(true, NULL);
    TEST_ASSERT_NOT_NULL(result);

    json_t* response = build_error_response(query_ref, database, cache_entry, pending, result, NULL);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    json_t* success = json_object_get(response, "success");
    json_t* error = json_object_get(response, "error");
    json_t* timeout_seconds = json_object_get(response, "timeout_seconds");
    json_t* query_ref_json = json_object_get(response, "query_ref");
    json_t* db_json = json_object_get(response, "database");

    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_FALSE(json_is_true(success));
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Query execution timeout", json_string_value(error));
    TEST_ASSERT_NOT_NULL(timeout_seconds);
    TEST_ASSERT_EQUAL_INT(30, json_integer_value(timeout_seconds));
    TEST_ASSERT_EQUAL_INT(1, json_integer_value(query_ref_json));
    TEST_ASSERT_EQUAL_STRING("testdb", json_string_value(db_json));

    free(cache_entry);
    free(pending);
    free(result);
    json_decref(response);
}

// Test database error case
static void test_build_error_response_database_error(void) {
    int query_ref = 1;
    const char* database = "testdb";
    QueryCacheEntry* cache_entry = create_dummy_cache_entry(30);
    TEST_ASSERT_NOT_NULL(cache_entry);

    PendingQueryResult* pending = create_dummy_pending(false);
    TEST_ASSERT_NOT_NULL(pending);

    QueryResult* result = create_dummy_query_result(false, "Database connection failed");
    TEST_ASSERT_NOT_NULL(result);

    json_t* response = build_error_response(query_ref, database, cache_entry, pending, result, NULL);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    json_t* success = json_object_get(response, "success");
    json_t* error = json_object_get(response, "error");
    json_t* message = json_object_get(response, "message");
    json_t* query_ref_json = json_object_get(response, "query_ref");
    json_t* db_json = json_object_get(response, "database");

    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_FALSE(json_is_true(success));
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Database error", json_string_value(error));
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_EQUAL_STRING("Database connection failed", json_string_value(message));
    TEST_ASSERT_EQUAL_INT(1, json_integer_value(query_ref_json));
    TEST_ASSERT_EQUAL_STRING("testdb", json_string_value(db_json));

    free(cache_entry);
    free(pending);
    free(result->error_message);
    free(result);
    json_decref(response);
}

// Test general failure case
static void test_build_error_response_general_failure(void) {
    int query_ref = 1;
    const char* database = "testdb";
    QueryCacheEntry* cache_entry = create_dummy_cache_entry(30);
    TEST_ASSERT_NOT_NULL(cache_entry);

    PendingQueryResult* pending = create_dummy_pending(false);
    TEST_ASSERT_NOT_NULL(pending);

    QueryResult* result = create_dummy_query_result(false, NULL);
    TEST_ASSERT_NOT_NULL(result);

    json_t* response = build_error_response(query_ref, database, cache_entry, pending, result, NULL);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    json_t* success = json_object_get(response, "success");
    json_t* error = json_object_get(response, "error");
    json_t* query_ref_json = json_object_get(response, "query_ref");
    json_t* db_json = json_object_get(response, "database");

    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_FALSE(json_is_true(success));
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Query execution failed", json_string_value(error));
    TEST_ASSERT_EQUAL_INT(1, json_integer_value(query_ref_json));
    TEST_ASSERT_EQUAL_STRING("testdb", json_string_value(db_json));

    free(cache_entry);
    free(pending);
    free(result);
    json_decref(response);
}

// Test with NULL result
static void test_build_error_response_null_result(void) {
    int query_ref = 1;
    const char* database = "testdb";
    QueryCacheEntry* cache_entry = create_dummy_cache_entry(30);
    TEST_ASSERT_NOT_NULL(cache_entry);

    PendingQueryResult* pending = create_dummy_pending(false);
    TEST_ASSERT_NOT_NULL(pending);

    const QueryResult* result = NULL;

    json_t* response = build_error_response(query_ref, database, cache_entry, pending, result, NULL);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    json_t* success = json_object_get(response, "success");
    json_t* error = json_object_get(response, "error");
    json_t* query_ref_json = json_object_get(response, "query_ref");
    json_t* db_json = json_object_get(response, "database");

    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_FALSE(json_is_true(success));
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL_STRING("Query execution failed", json_string_value(error));
    TEST_ASSERT_EQUAL_INT(1, json_integer_value(query_ref_json));
    TEST_ASSERT_EQUAL_STRING("testdb", json_string_value(db_json));

    free(cache_entry);
    free(pending);
    json_decref(response);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_build_error_response_timeout);
    RUN_TEST(test_build_error_response_database_error);
    RUN_TEST(test_build_error_response_general_failure);
    RUN_TEST(test_build_error_response_null_result);

    return UNITY_END();
}