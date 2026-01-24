/*
 * Unity Test File: extract_request_fields
 * This file contains unit tests for extract_request_fields function
 * in src/api/conduit/helpers/request_parsing.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/conduit_helpers.h>

// Function prototypes
void test_extract_request_fields_valid(void);
void test_extract_request_fields_missing_query_ref(void);
void test_extract_request_fields_invalid_query_ref_type(void);
void test_extract_request_fields_missing_database(void);
void test_extract_request_fields_invalid_database_type(void);
void test_extract_request_fields_with_params(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

// Test valid extraction
void test_extract_request_fields_valid(void) {
    json_t *request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(123));
    json_object_set_new(request_json, "database", json_string("testdb"));

    int query_ref;
    const char *database;
    json_t *params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &params);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(123, query_ref);
    TEST_ASSERT_EQUAL_STRING("testdb", database);
    TEST_ASSERT_NULL(params); // No params in this test

    json_decref(request_json);
}

// Test missing query_ref
void test_extract_request_fields_missing_query_ref(void) {
    json_t *request_json = json_object();
    json_object_set_new(request_json, "database", json_string("testdb"));

    int query_ref;
    const char *database;
    json_t *params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &params);

    TEST_ASSERT_FALSE(result);

    json_decref(request_json);
}

// Test invalid query_ref type (string instead of integer)
void test_extract_request_fields_invalid_query_ref_type(void) {
    json_t *request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_string("123"));
    json_object_set_new(request_json, "database", json_string("testdb"));

    int query_ref;
    const char *database;
    json_t *params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &params);

    TEST_ASSERT_FALSE(result);

    json_decref(request_json);
}

// Test missing database
void test_extract_request_fields_missing_database(void) {
    json_t *request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(123));

    int query_ref;
    const char *database;
    json_t *params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &params);

    TEST_ASSERT_FALSE(result);

    json_decref(request_json);
}

// Test invalid database type (integer instead of string)
void test_extract_request_fields_invalid_database_type(void) {
    json_t *request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(123));
    json_object_set_new(request_json, "database", json_integer(456));

    int query_ref;
    const char *database;
    json_t *params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &params);

    TEST_ASSERT_FALSE(result);

    json_decref(request_json);
}

// Test with params field
void test_extract_request_fields_with_params(void) {
    json_t *request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(123));
    json_object_set_new(request_json, "database", json_string("testdb"));

    json_t *params_json = json_object();
    json_object_set_new(params_json, "key", json_string("value"));
    json_object_set_new(request_json, "params", params_json);

    int query_ref;
    const char *database;
    json_t *params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &params);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(123, query_ref);
    TEST_ASSERT_EQUAL_STRING("testdb", database);
    TEST_ASSERT_NOT_NULL(params);
    TEST_ASSERT_TRUE(json_is_object(params));

    json_t *key = json_object_get(params, "key");
    TEST_ASSERT_TRUE(json_is_string(key));
    TEST_ASSERT_EQUAL_STRING("value", json_string_value(key));

    json_decref(request_json);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_extract_request_fields_valid);
    RUN_TEST(test_extract_request_fields_missing_query_ref);
    RUN_TEST(test_extract_request_fields_invalid_query_ref_type);
    RUN_TEST(test_extract_request_fields_missing_database);
    RUN_TEST(test_extract_request_fields_invalid_database_type);
    RUN_TEST(test_extract_request_fields_with_params);

    return UNITY_END();
}