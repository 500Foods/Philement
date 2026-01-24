/*
 * Unity Test File: handle_field_extraction
 * This file contains unit tests for handle_field_extraction function
 * in src/api/conduit/helpers/request_parsing.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks
#define USE_MOCK_API_UTILS
#include <unity/mocks/mock_api_utils.h>

// Local mock for api_send_json_response
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t *json_obj, unsigned int status_code);

// Include source header
#include <src/api/conduit/conduit_helpers.h>

// Mock implementation
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t *json_obj, unsigned int status_code) {
    (void)connection;
    (void)json_obj;
    (void)status_code;
    return MHD_YES; // Always succeed for tests
}

// Function prototypes
void test_handle_field_extraction_valid(void);
void test_handle_field_extraction_missing_query_ref(void);
void test_handle_field_extraction_invalid_query_ref(void);
void test_handle_field_extraction_missing_database(void);
void test_handle_field_extraction_invalid_database(void);

void setUp(void) {
    mock_api_utils_reset_all();
}

void tearDown(void) {
    // No cleanup needed
}

// Test valid field extraction
void test_handle_field_extraction_valid(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    json_t *request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(111));
    json_object_set_new(request_json, "database", json_string("extraction_db"));

    int query_ref = 0;
    const char *database = NULL;
    json_t *params_json = NULL;

    enum MHD_Result result = handle_field_extraction(connection, request_json, &query_ref, &database, &params_json);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(111, query_ref);
    TEST_ASSERT_EQUAL_STRING("extraction_db", database);
    TEST_ASSERT_NULL(params_json); // No params in this test

    json_decref(request_json);
}

// Test missing query_ref
void test_handle_field_extraction_missing_query_ref(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    json_t *request_json = json_object();
    json_object_set_new(request_json, "database", json_string("testdb"));

    int query_ref = 0;
    const char *database = NULL;
    json_t *params_json = NULL;

    enum MHD_Result result = handle_field_extraction(connection, request_json, &query_ref, &database, &params_json);

    TEST_ASSERT_EQUAL(MHD_NO, result);

    json_decref(request_json);
}

// Test invalid query_ref type
void test_handle_field_extraction_invalid_query_ref(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    json_t *request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_string("not_a_number"));
    json_object_set_new(request_json, "database", json_string("testdb"));

    int query_ref = 0;
    const char *database = NULL;
    json_t *params_json = NULL;

    enum MHD_Result result = handle_field_extraction(connection, request_json, &query_ref, &database, &params_json);

    TEST_ASSERT_EQUAL(MHD_NO, result);

    json_decref(request_json);
}

// Test missing database
void test_handle_field_extraction_missing_database(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    json_t *request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(222));

    int query_ref = 0;
    const char *database = NULL;
    json_t *params_json = NULL;

    enum MHD_Result result = handle_field_extraction(connection, request_json, &query_ref, &database, &params_json);

    TEST_ASSERT_EQUAL(MHD_NO, result);

    json_decref(request_json);
}

// Test invalid database type
void test_handle_field_extraction_invalid_database(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    json_t *request_json = json_object();
    json_object_set_new(request_json, "query_ref", json_integer(333));
    json_object_set_new(request_json, "database", json_integer(444));

    int query_ref = 0;
    const char *database = NULL;
    json_t *params_json = NULL;

    enum MHD_Result result = handle_field_extraction(connection, request_json, &query_ref, &database, &params_json);

    TEST_ASSERT_EQUAL(MHD_NO, result);

    json_decref(request_json);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_field_extraction_valid);
    RUN_TEST(test_handle_field_extraction_missing_query_ref);
    RUN_TEST(test_handle_field_extraction_invalid_query_ref);
    RUN_TEST(test_handle_field_extraction_missing_database);
    RUN_TEST(test_handle_field_extraction_invalid_database);

    return UNITY_END();
}