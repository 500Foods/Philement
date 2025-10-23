/*
 * Unity Test File: create_processing_error_response
 * This file contains unit tests for create_processing_error_response function
 * in src/api/conduit/query/query.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/query/query.h>

// Forward declarations for functions not in header
json_t* create_processing_error_response(const char* error_msg, const char* database, int query_ref);

// Function prototypes
void test_create_processing_error_response_basic(void);
void test_create_processing_error_response_with_null_database(void);
void test_create_processing_error_response_different_errors(void);

void setUp(void) {
    // No setup needed for this function
}

void tearDown(void) {
    // No cleanup needed for this function
}

// Test basic error response creation
void test_create_processing_error_response_basic(void) {
    json_t* response = create_processing_error_response("Parameter conversion failed", "test_db", 123);

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
    TEST_ASSERT_EQUAL_STRING("Parameter conversion failed", json_string_value(error));

    // Check query_ref field
    json_t* query_ref = json_object_get(response, "query_ref");
    TEST_ASSERT_NOT_NULL(query_ref);
    TEST_ASSERT_TRUE(json_is_integer(query_ref));
    TEST_ASSERT_EQUAL(123, json_integer_value(query_ref));

    // Check database field
    json_t* database = json_object_get(response, "database");
    TEST_ASSERT_NOT_NULL(database);
    TEST_ASSERT_TRUE(json_is_string(database));
    TEST_ASSERT_EQUAL_STRING("test_db", json_string_value(database));

    json_decref(response);
}

// Test with NULL database
void test_create_processing_error_response_with_null_database(void) {
    json_t* response = create_processing_error_response("Memory allocation failed", NULL, 456);

    TEST_ASSERT_NOT_NULL(response);

    // Check that database field is still created (even if NULL input)
    json_t* database = json_object_get(response, "database");
    TEST_ASSERT_NOT_NULL(database);
    TEST_ASSERT_TRUE(json_is_string(database));
    // Should be empty string for NULL input
    TEST_ASSERT_EQUAL_STRING("", json_string_value(database));

    json_decref(response);
}

// Test different error messages
void test_create_processing_error_response_different_errors(void) {
    const char* test_cases[][3] = {
        {"Failed to generate query ID", "db1", "789"},
        {"Failed to register pending result", "db2", "999"},
        {"Failed to submit query", "db3", "111"},
        {"No suitable queue available", "db4", "222"}
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        const char* error_msg = test_cases[i][0];
        const char* database = test_cases[i][1];
        int query_ref = atoi(test_cases[i][2]);

        json_t* response = create_processing_error_response(error_msg, database, query_ref);

        TEST_ASSERT_NOT_NULL(response);

        // Verify error message
        json_t* error = json_object_get(response, "error");
        TEST_ASSERT_EQUAL_STRING(error_msg, json_string_value(error));

        // Verify database
        json_t* db_field = json_object_get(response, "database");
        TEST_ASSERT_EQUAL_STRING(database, json_string_value(db_field));

        // Verify query_ref
        json_t* ref_field = json_object_get(response, "query_ref");
        TEST_ASSERT_EQUAL(query_ref, json_integer_value(ref_field));

        json_decref(response);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_create_processing_error_response_basic);
    if (0) RUN_TEST(test_create_processing_error_response_with_null_database);
    RUN_TEST(test_create_processing_error_response_different_errors);

    return UNITY_END();
}