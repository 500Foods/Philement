/*
 * Unity Test File: create_lookup_error_response
 * This file contains unit tests for create_lookup_error_response function
 * in src/api/conduit/query/query.c
 */

// Project includes
#include <src/hydrogen.h>
#include "unity.h"

// Include source header
#include <src/api/conduit/query/query.h>

// Forward declarations for functions not in header
json_t* create_lookup_error_response(const char* error_msg, const char* database, int query_ref, bool include_query_ref, const char* message);

// Function prototypes
void test_create_lookup_error_response_with_database(void);
void test_create_lookup_error_response_without_database(void);
void test_create_lookup_error_response_include_query_ref(void);
void test_create_lookup_error_response_exclude_query_ref(void);

void setUp(void) {
    // No setup needed for this function
}

void tearDown(void) {
    // No cleanup needed for this function
}

// Test error response creation with database included
void test_create_lookup_error_response_with_database(void) {
    json_t* response = create_lookup_error_response("Database not available", "test_db", 123, true, "Database is not available");

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    // Check success field
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_TRUE(json_is_false(success));

    // Check error field
    json_t* error = json_object_get(response, "error");
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_TRUE(json_is_string(error));
    TEST_ASSERT_EQUAL_STRING("Database not available", json_string_value(error));

    // Check database field
    json_t* database = json_object_get(response, "database");
    TEST_ASSERT_NOT_NULL(database);
    TEST_ASSERT_TRUE(json_is_string(database));
    TEST_ASSERT_EQUAL_STRING("test_db", json_string_value(database));

    // Check query_ref field (should be included)
    json_t* query_ref = json_object_get(response, "query_ref");
    TEST_ASSERT_NOT_NULL(query_ref);
    TEST_ASSERT_TRUE(json_is_integer(query_ref));
    TEST_ASSERT_EQUAL(123, json_integer_value(query_ref));

    // Check message field
    json_t* message = json_object_get(response, "message");
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_TRUE(json_is_string(message));
    TEST_ASSERT_EQUAL_STRING("Database is not available", json_string_value(message));

    json_decref(response);
}

// Test error response creation without database
void test_create_lookup_error_response_without_database(void) {
    json_t* response = create_lookup_error_response("Query not found", NULL, 456, false, NULL);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    // Check success field
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_TRUE(json_is_false(success));

    // Check error field
    json_t* error = json_object_get(response, "error");
    TEST_ASSERT_TRUE(json_is_string(error));
    TEST_ASSERT_EQUAL_STRING("Query not found", json_string_value(error));

    // Check database field (should not be present)
    json_t* database = json_object_get(response, "database");
    TEST_ASSERT_NULL(database);

    // Check query_ref field (should not be included)
    json_t* query_ref = json_object_get(response, "query_ref");
    TEST_ASSERT_NULL(query_ref);

    // Check message field (should not be present)
    json_t* message = json_object_get(response, "message");
    TEST_ASSERT_NULL(message);

    json_decref(response);
}

// Test with query_ref included
void test_create_lookup_error_response_include_query_ref(void) {
    json_t* response = create_lookup_error_response("Test error", "test_db", 789, true, NULL);

    // Check query_ref field
    json_t* query_ref = json_object_get(response, "query_ref");
    TEST_ASSERT_NOT_NULL(query_ref);
    TEST_ASSERT_TRUE(json_is_integer(query_ref));
    TEST_ASSERT_EQUAL(789, json_integer_value(query_ref));

    json_decref(response);
}

// Test with query_ref excluded
void test_create_lookup_error_response_exclude_query_ref(void) {
    json_t* response = create_lookup_error_response("Test error", "test_db", 999, false, NULL);

    // Check query_ref field (should not be present)
    json_t* query_ref = json_object_get(response, "query_ref");
    TEST_ASSERT_NULL(query_ref);

    json_decref(response);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_create_lookup_error_response_with_database);
    RUN_TEST(test_create_lookup_error_response_without_database);
    RUN_TEST(test_create_lookup_error_response_include_query_ref);
    RUN_TEST(test_create_lookup_error_response_exclude_query_ref);

    return UNITY_END();
}