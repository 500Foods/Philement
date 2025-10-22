/*
 * Unity Test File: Test extract_request_fields function
 * This file contains unit tests for src/api/conduit/query/query.c extract_request_fields function
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>  // For json_t types, assuming it's available

// Forward declaration for the function being tested
bool extract_request_fields(json_t* request_json, int* query_ref, const char** database, json_t** params);

// Test function prototypes
void test_extract_request_fields_valid_all_fields(void);
void test_extract_request_fields_missing_query_ref(void);
void test_extract_request_fields_invalid_query_ref(void);
void test_extract_request_fields_missing_database(void);
void test_extract_request_fields_invalid_database(void);
void test_extract_request_fields_valid_no_params(void);

// Test function prototypes
void test_extract_request_fields_valid_all_fields(void);
void test_extract_request_fields_missing_query_ref(void);
void test_extract_request_fields_invalid_query_ref(void);
void test_extract_request_fields_missing_database(void);
void test_extract_request_fields_invalid_database(void);
void test_extract_request_fields_valid_no_params(void);

// Test function prototypes
void test_extract_request_fields_valid_all_fields(void);
void test_extract_request_fields_missing_query_ref(void);
void test_extract_request_fields_invalid_query_ref(void);
void test_extract_request_fields_missing_database(void);
void test_extract_request_fields_invalid_database(void);
void test_extract_request_fields_valid_no_params(void);

// Function prototypes for tests
void test_extract_request_fields_valid_all_fields(void);
void test_extract_request_fields_missing_query_ref(void);
void test_extract_request_fields_invalid_query_ref(void);
void test_extract_request_fields_missing_database(void);
void test_extract_request_fields_invalid_database(void);
void test_extract_request_fields_valid_no_params(void);

void test_extract_request_fields_valid_all_fields(void);
void test_extract_request_fields_missing_query_ref(void);
void test_extract_request_fields_invalid_query_ref(void);
void test_extract_request_fields_missing_database(void);
void test_extract_request_fields_invalid_database(void);
void test_extract_request_fields_valid_no_params(void);

void setUp(void) {
    // No specific setup needed for this test
}

void tearDown(void) {
    // No specific teardown needed for this test
}

// Test valid request with all required fields and optional params
void test_extract_request_fields_valid_all_fields(void);
void test_extract_request_fields_missing_query_ref(void);
void test_extract_request_fields_invalid_query_ref(void);
void test_extract_request_fields_missing_database(void);
void test_extract_request_fields_invalid_database(void);
void test_extract_request_fields_valid_no_params(void);

void test_extract_request_fields_valid_all_fields(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(123));
    json_object_set_new(request_json, "database", json_string("test_db"));
    json_t* params = json_object();
    json_object_set_new(params, "param1", json_string("value1"));
    json_object_set_new(request_json, "params", params);

    int query_ref = 0;
    const char* database = NULL;
    json_t* extracted_params = NULL;

    bool result = extract_request_fields(request_json, &query_ref, &database, &extracted_params);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(123, query_ref);
    TEST_ASSERT_EQUAL_STRING("test_db", database);
    TEST_ASSERT_EQUAL(params, extracted_params);  // Should be the same object

    json_decref(request_json);
}

// Test missing query_ref
void test_extract_request_fields_missing_query_ref(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "database", json_string("test_db"));

    int query_ref = 0;
    const char* database = NULL;
    json_t* extracted_params = NULL;

    bool result = extract_request_fields(request_json, &query_ref, &database, &extracted_params);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, query_ref);  // Unchanged
    TEST_ASSERT_NULL(database);
    TEST_ASSERT_NULL(extracted_params);

    json_decref(request_json);
}

// Test invalid query_ref (not integer)
void test_extract_request_fields_invalid_query_ref(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_string("not_an_int"));
    json_object_set_new(request_json, "database", json_string("test_db"));

    int query_ref = 0;
    const char* database = NULL;
    json_t* extracted_params = NULL;

    bool result = extract_request_fields(request_json, &query_ref, &database, &extracted_params);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, query_ref);  // Unchanged
    TEST_ASSERT_NULL(database);
    TEST_ASSERT_NULL(extracted_params);

    json_decref(request_json);
}

// Test missing database
void test_extract_request_fields_missing_database(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(123));

    int query_ref = 0;
    const char* database = NULL;
    json_t* extracted_params = NULL;

    bool result = extract_request_fields(request_json, &query_ref, &database, &extracted_params);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, query_ref);  // Unchanged
    TEST_ASSERT_NULL(database);
    TEST_ASSERT_NULL(extracted_params);

    json_decref(request_json);
}

// Test invalid database (not string)
void test_extract_request_fields_invalid_database(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(123));
    json_object_set_new(request_json, "database", json_integer(456));

    int query_ref = 0;
    const char* database = NULL;
    json_t* extracted_params = NULL;

    bool result = extract_request_fields(request_json, &query_ref, &database, &extracted_params);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, query_ref);  // Unchanged
    TEST_ASSERT_NULL(database);
    TEST_ASSERT_NULL(extracted_params);

    json_decref(request_json);
}

// Test valid request without optional params
void test_extract_request_fields_valid_no_params(void) {
    json_t* request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(123));
    json_object_set_new(request_json, "database", json_string("test_db"));

    int query_ref = 0;
    const char* database = NULL;
    json_t* extracted_params = NULL;

    bool result = extract_request_fields(request_json, &query_ref, &database, &extracted_params);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(123, query_ref);
    TEST_ASSERT_EQUAL_STRING("test_db", database);
    TEST_ASSERT_NULL(extracted_params);  // params is optional

    json_decref(request_json);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_extract_request_fields_valid_all_fields);
    RUN_TEST(test_extract_request_fields_missing_query_ref);
    RUN_TEST(test_extract_request_fields_invalid_query_ref);
    RUN_TEST(test_extract_request_fields_missing_database);
    RUN_TEST(test_extract_request_fields_invalid_database);
    RUN_TEST(test_extract_request_fields_valid_no_params);

    return UNITY_END();
}