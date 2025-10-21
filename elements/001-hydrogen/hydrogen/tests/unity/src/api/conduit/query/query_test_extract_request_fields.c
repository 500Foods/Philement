/*
 * Unity Test: extract_request_fields function
 * Tests extraction and validation of required request fields
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include the source file to test static functions
#include "../../../../../src/api/conduit/query/query.c"

// Test function prototypes
void test_extract_request_fields_valid(void);
void test_extract_request_fields_missing_query_ref(void);
void test_extract_request_fields_invalid_query_ref_type(void);
void test_extract_request_fields_missing_database(void);
void test_extract_request_fields_invalid_database_type(void);
void test_extract_request_fields_null_params(void);
void test_extract_request_fields_empty_params(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

// Test valid request with all required fields
void test_extract_request_fields_valid(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(123));
    json_object_set_new(request_json, "database", json_string("test_db"));
    json_object_set_new(request_json, "params", json_object()); // Optional

    int query_ref;
    const char* database;
    json_t* params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &params);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(123, query_ref);
    TEST_ASSERT_EQUAL_STRING("test_db", database);
    TEST_ASSERT_NOT_NULL(params);
    TEST_ASSERT_TRUE(json_is_object(params));

    json_decref(request_json);
}

// Test missing query_ref field
void test_extract_request_fields_missing_query_ref(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "database", json_string("test_db"));

    int query_ref;
    const char* database;
    json_t* params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &params);

    TEST_ASSERT_FALSE(result);

    json_decref(request_json);
}

// Test invalid query_ref type (string instead of integer)
void test_extract_request_fields_invalid_query_ref_type(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_string("123"));
    json_object_set_new(request_json, "database", json_string("test_db"));

    int query_ref;
    const char* database;
    json_t* params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &params);

    TEST_ASSERT_FALSE(result);

    json_decref(request_json);
}

// Test missing database field
void test_extract_request_fields_missing_database(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(123));

    int query_ref;
    const char* database;
    json_t* params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &params);

    TEST_ASSERT_FALSE(result);

    json_decref(request_json);
}

// Test invalid database type (integer instead of string)
void test_extract_request_fields_invalid_database_type(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(123));
    json_object_set_new(request_json, "database", json_integer(456));

    int query_ref;
    const char* database;
    json_t* params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &params);

    TEST_ASSERT_FALSE(result);

    json_decref(request_json);
}

// Test with NULL params (should be valid)
void test_extract_request_fields_null_params(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(123));
    json_object_set_new(request_json, "database", json_string("test_db"));
    // No params field

    int query_ref;
    const char* database;
    json_t* params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &params);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(123, query_ref);
    TEST_ASSERT_EQUAL_STRING("test_db", database);
    TEST_ASSERT_NULL(params); // Should be NULL when not present

    json_decref(request_json);
}

// Test with empty params object
void test_extract_request_fields_empty_params(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(123));
    json_object_set_new(request_json, "database", json_string("test_db"));
    json_object_set_new(request_json, "params", json_object());

    int query_ref;
    const char* database;
    json_t* params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &params);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(123, query_ref);
    TEST_ASSERT_EQUAL_STRING("test_db", database);
    TEST_ASSERT_NOT_NULL(params);
    TEST_ASSERT_TRUE(json_is_object(params));
    TEST_ASSERT_EQUAL(0, json_object_size(params));

    json_decref(request_json);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_extract_request_fields_valid);
    RUN_TEST(test_extract_request_fields_missing_query_ref);
    RUN_TEST(test_extract_request_fields_invalid_query_ref_type);
    RUN_TEST(test_extract_request_fields_missing_database);
    RUN_TEST(test_extract_request_fields_invalid_database_type);
    RUN_TEST(test_extract_request_fields_null_params);
    RUN_TEST(test_extract_request_fields_empty_params);

    return UNITY_END();
}