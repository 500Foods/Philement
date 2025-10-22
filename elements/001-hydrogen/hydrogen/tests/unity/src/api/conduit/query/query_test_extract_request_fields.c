/*
 * Unity Test File: extract_request_fields
 * This file contains unit tests for extract_request_fields function
 * in src/api/conduit/query/query.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>
#include <jansson.h>  // For JSON handling

// Include source header
#include <src/api/conduit/query/query.h>

// Test function declarations
static void test_extract_request_fields_valid_all(void);
static void test_extract_request_fields_valid_no_params(void);
static void test_extract_request_fields_missing_query_ref(void);
static void test_extract_request_fields_invalid_query_ref(void);
static void test_extract_request_fields_missing_database(void);
static void test_extract_request_fields_invalid_database(void);
static void test_extract_request_fields_null_json(void);

void setUp(void) {
    // No setup required
}

void tearDown(void) {
    // No teardown required
}

// Test valid extraction with all fields
static void test_extract_request_fields_valid_all(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(42));
    json_object_set_new(request_json, "database", json_string("testdb"));
    json_t* params = json_object();
    json_object_set_new(request_json, "params", params);

    int query_ref;
    const char* database;
    json_t* extracted_params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &extracted_params);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(42, query_ref);
    TEST_ASSERT_EQUAL_STRING("testdb", database);
    TEST_ASSERT_EQUAL(params, extracted_params);  // Same pointer

    json_decref(request_json);
}

// Test valid extraction without params
static void test_extract_request_fields_valid_no_params(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(42));
    json_object_set_new(request_json, "database", json_string("testdb"));

    int query_ref;
    const char* database;
    json_t* extracted_params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &extracted_params);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(42, query_ref);
    TEST_ASSERT_EQUAL_STRING("testdb", database);
    TEST_ASSERT_NULL(extracted_params);

    json_decref(request_json);
}

// Test missing query_ref
static void test_extract_request_fields_missing_query_ref(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "database", json_string("testdb"));

    int query_ref;
    const char* database;
    json_t* extracted_params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &extracted_params);

    TEST_ASSERT_FALSE(result);

    json_decref(request_json);
}

// Test invalid query_ref (not integer)
static void test_extract_request_fields_invalid_query_ref(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_string("not_an_int"));
    json_object_set_new(request_json, "database", json_string("testdb"));

    int query_ref;
    const char* database;
    json_t* extracted_params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &extracted_params);

    TEST_ASSERT_FALSE(result);

    json_decref(request_json);
}

// Test missing database
static void test_extract_request_fields_missing_database(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(42));

    int query_ref;
    const char* database;
    json_t* extracted_params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &extracted_params);

    TEST_ASSERT_FALSE(result);

    json_decref(request_json);
}

// Test invalid database (not string)
static void test_extract_request_fields_invalid_database(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(42));
    json_object_set_new(request_json, "database", json_integer(123));

    int query_ref;
    const char* database;
    json_t* extracted_params;

    bool result = extract_request_fields(request_json, &query_ref, &database, &extracted_params);

    TEST_ASSERT_FALSE(result);

    json_decref(request_json);
}

// Test null request_json
static void test_extract_request_fields_null_json(void) {
    int query_ref;
    const char* database;
    json_t* extracted_params;

    bool result = extract_request_fields(NULL, &query_ref, &database, &extracted_params);

    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_extract_request_fields_valid_all);
    RUN_TEST(test_extract_request_fields_valid_no_params);
    RUN_TEST(test_extract_request_fields_missing_query_ref);
    RUN_TEST(test_extract_request_fields_invalid_query_ref);
    RUN_TEST(test_extract_request_fields_missing_database);
    RUN_TEST(test_extract_request_fields_invalid_database);
    RUN_TEST(test_extract_request_fields_null_json);

    return UNITY_END();
}