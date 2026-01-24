/*
 * Unity Test File: parameter_processing_test_generate_parameter_messages.c
 * This file contains unit tests for the generate_parameter_messages function
 */

// Standard includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <jansson.h>

// Include source header
#include <src/api/conduit/conduit_helpers.h>

// Forward declarations for functions being tested
char* generate_parameter_messages(const char* sql_template, json_t* params_json);

void setUp(void) {
    // No setup needed for these tests
}

void tearDown(void) {
    // No cleanup needed for these tests
}

// Test NULL sql_template
static void test_generate_parameter_messages_null_sql_template(void) {
    json_t* params = json_object();
    char* result = generate_parameter_messages(NULL, params);
    TEST_ASSERT_NULL(result);
    json_decref(params);
}

// Test empty sql_template
static void test_generate_parameter_messages_empty_sql_template(void) {
    json_t* params = json_object();
    char* result = generate_parameter_messages("", params);
    TEST_ASSERT_NULL(result);
    json_decref(params);
}

// Test NULL params_json
static void test_generate_parameter_messages_null_params(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId";
    char* result = generate_parameter_messages(sql, NULL);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "Missing parameters: userId"));
    free(result);
}

// Test valid parameters - no messages
static void test_generate_parameter_messages_valid_params(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId AND name = :userName";
    json_t* params = json_object();
    json_t* integer_obj = json_object();
    json_t* string_obj = json_object();

    json_object_set_new(integer_obj, "userId", json_integer(123));
    json_object_set_new(string_obj, "userName", json_string("test"));

    json_object_set_new(params, "INTEGER", integer_obj);
    json_object_set_new(params, "STRING", string_obj);

    char* result = generate_parameter_messages(sql, params);
    TEST_ASSERT_NULL(result);

    json_decref(params);
}

// Test missing parameters
static void test_generate_parameter_messages_missing_params(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId AND name = :userName";
    json_t* params = json_object();
    json_t* integer_obj = json_object();

    json_object_set_new(integer_obj, "userId", json_integer(123));
    json_object_set_new(params, "INTEGER", integer_obj);

    char* result = generate_parameter_messages(sql, params);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "Missing parameters: userName"));
    free(result);

    json_decref(params);
}

// Test unused parameters
static void test_generate_parameter_messages_unused_params(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId";
    json_t* params = json_object();
    json_t* integer_obj = json_object();
    json_t* string_obj = json_object();

    json_object_set_new(integer_obj, "userId", json_integer(123));
    json_object_set_new(integer_obj, "unusedId", json_integer(456));
    json_object_set_new(string_obj, "userName", json_string("test"));

    json_object_set_new(params, "INTEGER", integer_obj);
    json_object_set_new(params, "STRING", string_obj);

    char* result = generate_parameter_messages(sql, params);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "Unused parameters: unusedId, userName"));
    free(result);

    json_decref(params);
}

// Test invalid parameter types
static void test_generate_parameter_messages_invalid_types(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId";
    json_t* params = json_object();
    json_t* integer_obj = json_object();

    // Wrong type: string in INTEGER section
    json_object_set_new(integer_obj, "userId", json_string("not_an_integer"));
    json_object_set_new(params, "INTEGER", integer_obj);

    char* result = generate_parameter_messages(sql, params);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "Invalid parameters: userId(string) should be userId(INTEGER)"));
    free(result);

    json_decref(params);
}

// Test missing and invalid parameters combined
static void test_generate_parameter_messages_missing_and_invalid(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId AND name = :userName";
    json_t* params = json_object();
    json_t* integer_obj = json_object();

    // Wrong type and missing parameter
    json_object_set_new(integer_obj, "userId", json_string("invalid"));
    json_object_set_new(params, "INTEGER", integer_obj);

    char* result = generate_parameter_messages(sql, params);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "Missing parameters: userName"));
    TEST_ASSERT_NOT_NULL(strstr(result, "Invalid parameters: userId(string) should be userId(INTEGER)"));
    free(result);

    json_decref(params);
}

// Test missing, unused, and invalid parameters combined
static void test_generate_parameter_messages_all_issues(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId";
    json_t* params = json_object();
    json_t* integer_obj = json_object();
    json_t* string_obj = json_object();

    // Wrong type, and unused parameter
    json_object_set_new(integer_obj, "userId", json_string("invalid"));
    json_object_set_new(integer_obj, "unusedId", json_integer(456));
    json_object_set_new(string_obj, "userName", json_string("test"));

    json_object_set_new(params, "INTEGER", integer_obj);
    json_object_set_new(params, "STRING", string_obj);

    char* result = generate_parameter_messages(sql, params);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "Invalid parameters: userId(string) should be userId(INTEGER)"));
    TEST_ASSERT_NOT_NULL(strstr(result, "Unused parameters: unusedId, userName"));
    free(result);

    json_decref(params);
}

// Test duplicate parameters in SQL template
static void test_generate_parameter_messages_duplicate_sql_params(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId OR parent_id = :userId";
    json_t* params = json_object();
    json_t* integer_obj = json_object();

    json_object_set_new(integer_obj, "userId", json_integer(123));
    json_object_set_new(params, "INTEGER", integer_obj);

    char* result = generate_parameter_messages(sql, params);
    TEST_ASSERT_NULL(result);

    json_decref(params);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_generate_parameter_messages_null_sql_template);
    RUN_TEST(test_generate_parameter_messages_empty_sql_template);
    RUN_TEST(test_generate_parameter_messages_null_params);
    RUN_TEST(test_generate_parameter_messages_valid_params);
    RUN_TEST(test_generate_parameter_messages_missing_params);
    RUN_TEST(test_generate_parameter_messages_unused_params);
    RUN_TEST(test_generate_parameter_messages_invalid_types);
    RUN_TEST(test_generate_parameter_messages_missing_and_invalid);
    RUN_TEST(test_generate_parameter_messages_all_issues);
    RUN_TEST(test_generate_parameter_messages_duplicate_sql_params);

    return UNITY_END();
}