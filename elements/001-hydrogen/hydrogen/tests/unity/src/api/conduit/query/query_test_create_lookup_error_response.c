/*
 * Unity Test File: Test create_lookup_error_response function
 * This file contains unit tests for src/api/conduit/query/query.c create_lookup_error_response function
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Forward declaration for the function being tested
json_t* create_lookup_error_response(const char* error_msg, const char* database, int query_ref, bool include_query_ref);

void setUp(void) {
    // No specific setup
}

void tearDown(void) {
    // No specific teardown
}

// Test basic error response without database or query_ref
// Test function prototypes
void test_create_lookup_error_response_basic(void);
void test_create_lookup_error_response_with_database_no_ref(void);
void test_create_lookup_error_response_with_ref_no_database(void);
void test_create_lookup_error_response_full(void);
void test_create_lookup_error_response_basic(void) {
    json_t* response = create_lookup_error_response("Test error", NULL, 0, false);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_TRUE(json_is_false(success));

    json_t* error = json_object_get(response, "error");
    TEST_ASSERT_EQUAL_STRING("Test error", json_string_value(error));

    // No database or query_ref
    TEST_ASSERT_NULL(json_object_get(response, "database"));
    TEST_ASSERT_NULL(json_object_get(response, "query_ref"));

    json_decref(response);
}

// Test with database but no query_ref
void test_create_lookup_error_response_with_database_no_ref(void) {
    json_t* response = create_lookup_error_response("Database error", "test_db", 0, false);

    TEST_ASSERT_NOT_NULL(response);

    json_t* db = json_object_get(response, "database");
    TEST_ASSERT_EQUAL_STRING("test_db", json_string_value(db));

    TEST_ASSERT_NULL(json_object_get(response, "query_ref"));

    json_decref(response);
}

// Test with query_ref but no database
void test_create_lookup_error_response_with_ref_no_database(void) {
    json_t* response = create_lookup_error_response("Query error", NULL, 123, true);

    TEST_ASSERT_NOT_NULL(response);

    json_t* ref = json_object_get(response, "query_ref");
    TEST_ASSERT_EQUAL_INT(123, json_integer_value(ref));

    TEST_ASSERT_NULL(json_object_get(response, "database"));

    json_decref(response);
}

// Test with both database and query_ref
void test_create_lookup_error_response_full(void) {
    json_t* response = create_lookup_error_response("Full error", "test_db", 456, true);

    TEST_ASSERT_NOT_NULL(response);

    json_t* db = json_object_get(response, "database");
    TEST_ASSERT_EQUAL_STRING("test_db", json_string_value(db));

    json_t* ref = json_object_get(response, "query_ref");
    TEST_ASSERT_EQUAL_INT(456, json_integer_value(ref));

    json_decref(response);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_create_lookup_error_response_basic);
    RUN_TEST(test_create_lookup_error_response_with_database_no_ref);
    RUN_TEST(test_create_lookup_error_response_with_ref_no_database);
    RUN_TEST(test_create_lookup_error_response_full);

    return UNITY_END();
}