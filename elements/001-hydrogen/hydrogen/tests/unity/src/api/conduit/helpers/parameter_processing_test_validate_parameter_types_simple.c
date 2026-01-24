/*
 * Unity Test File: parameter_processing_test_validate_parameter_types_simple.c
 * This file contains unit tests for the validate_parameter_types_simple function
 */

// Standard includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <jansson.h>

// Include source header
#include <src/api/conduit/conduit_helpers.h>

// Forward declarations for functions being tested
char* validate_parameter_types_simple(json_t* params_json);

void setUp(void) {
    // No setup needed for these tests
}

void tearDown(void) {
    // No cleanup needed for these tests
}

// Test NULL params_json
static void test_validate_parameter_types_simple_null_params(void) {
    char* result = validate_parameter_types_simple(NULL);
    TEST_ASSERT_NULL(result);
}

// Test empty object
static void test_validate_parameter_types_simple_empty_object(void) {
    json_t* params = json_object();
    char* result = validate_parameter_types_simple(params);
    TEST_ASSERT_NULL(result);
    json_decref(params);
}

// Test valid parameters
static void test_validate_parameter_types_simple_valid_params(void) {
    json_t* params = json_object();
    json_t* integer_obj = json_object();
    json_t* string_obj = json_object();
    json_t* boolean_obj = json_object();
    json_t* float_obj = json_object();
    json_t* text_obj = json_object();
    json_t* date_obj = json_object();
    json_t* time_obj = json_object();
    json_t* datetime_obj = json_object();
    json_t* timestamp_obj = json_object();

    // Add valid parameters
    json_object_set_new(integer_obj, "userId", json_integer(123));
    json_object_set_new(string_obj, "name", json_string("test"));
    json_object_set_new(boolean_obj, "active", json_boolean(true));
    json_object_set_new(float_obj, "price", json_real(19.99));
    json_object_set_new(text_obj, "description", json_string("text"));
    json_object_set_new(date_obj, "birth_date", json_string("1990-01-01"));
    json_object_set_new(time_obj, "login_time", json_string("14:30:00"));
    json_object_set_new(datetime_obj, "created_at", json_string("2023-12-01 10:00:00"));
    json_object_set_new(timestamp_obj, "updated_at", json_string("2023-12-01 10:00:00.123"));

    json_object_set_new(params, "INTEGER", integer_obj);
    json_object_set_new(params, "STRING", string_obj);
    json_object_set_new(params, "BOOLEAN", boolean_obj);
    json_object_set_new(params, "FLOAT", float_obj);
    json_object_set_new(params, "TEXT", text_obj);
    json_object_set_new(params, "DATE", date_obj);
    json_object_set_new(params, "TIME", time_obj);
    json_object_set_new(params, "DATETIME", datetime_obj);
    json_object_set_new(params, "TIMESTAMP", timestamp_obj);

    char* result = validate_parameter_types_simple(params);
    TEST_ASSERT_NULL(result);

    json_decref(params);
}

// Test invalid INTEGER type
static void test_validate_parameter_types_simple_invalid_integer(void) {
    json_t* params = json_object();
    json_t* integer_obj = json_object();

    json_object_set_new(integer_obj, "userId", json_string("not_an_integer"));
    json_object_set_new(params, "INTEGER", integer_obj);

    char* result = validate_parameter_types_simple(params);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "userId(string) is not userId(INTEGER)"));

    free(result);
    json_decref(params);
}

// Test invalid STRING type
static void test_validate_parameter_types_simple_invalid_string(void) {
    json_t* params = json_object();
    json_t* string_obj = json_object();

    json_object_set_new(string_obj, "name", json_integer(123));
    json_object_set_new(params, "STRING", string_obj);

    char* result = validate_parameter_types_simple(params);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "name(integer) is not name(STRING)"));

    free(result);
    json_decref(params);
}

// Test invalid BOOLEAN type
static void test_validate_parameter_types_simple_invalid_boolean(void) {
    json_t* params = json_object();
    json_t* boolean_obj = json_object();

    json_object_set_new(boolean_obj, "active", json_string("true"));
    json_object_set_new(params, "BOOLEAN", boolean_obj);

    char* result = validate_parameter_types_simple(params);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "active(string) is not active(BOOLEAN)"));

    free(result);
    json_decref(params);
}

// Test invalid FLOAT type
static void test_validate_parameter_types_simple_invalid_float(void) {
    json_t* params = json_object();
    json_t* float_obj = json_object();

    json_object_set_new(float_obj, "price", json_boolean(true));
    json_object_set_new(params, "FLOAT", float_obj);

    char* result = validate_parameter_types_simple(params);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "price(boolean) is not price(FLOAT)"));

    free(result);
    json_decref(params);
}

// Test invalid TEXT type
static void test_validate_parameter_types_simple_invalid_text(void) {
    json_t* params = json_object();
    json_t* text_obj = json_object();

    json_object_set_new(text_obj, "description", json_integer(123));
    json_object_set_new(params, "TEXT", text_obj);

    char* result = validate_parameter_types_simple(params);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "description(integer) is not description(TEXT)"));

    free(result);
    json_decref(params);
}

// Test multiple invalid types
static void test_validate_parameter_types_simple_multiple_invalid(void) {
    json_t* params = json_object();
    json_t* integer_obj = json_object();
    json_t* string_obj = json_object();

    json_object_set_new(integer_obj, "userId", json_string("invalid"));
    json_object_set_new(string_obj, "name", json_integer(456));

    json_object_set_new(params, "INTEGER", integer_obj);
    json_object_set_new(params, "STRING", string_obj);

    char* result = validate_parameter_types_simple(params);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "userId(string) is not userId(INTEGER)"));
    TEST_ASSERT_NOT_NULL(strstr(result, "name(integer) is not name(STRING)"));

    free(result);
    json_decref(params);
}

// Test null values
static void test_validate_parameter_types_simple_null_values(void) {
    json_t* params = json_object();
    json_t* integer_obj = json_object();

    json_object_set_new(integer_obj, "userId", json_null());
    json_object_set_new(params, "INTEGER", integer_obj);

    char* result = validate_parameter_types_simple(params);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "userId(null) is not userId(INTEGER)"));

    free(result);
    json_decref(params);
}

// Test float accepts integer
static void test_validate_parameter_types_simple_float_accepts_integer(void) {
    json_t* params = json_object();
    json_t* float_obj = json_object();

    json_object_set_new(float_obj, "price", json_integer(20));
    json_object_set_new(params, "FLOAT", float_obj);

    char* result = validate_parameter_types_simple(params);
    TEST_ASSERT_NULL(result);

    json_decref(params);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_parameter_types_simple_null_params);
    RUN_TEST(test_validate_parameter_types_simple_empty_object);
    RUN_TEST(test_validate_parameter_types_simple_valid_params);
    RUN_TEST(test_validate_parameter_types_simple_invalid_integer);
    RUN_TEST(test_validate_parameter_types_simple_invalid_string);
    RUN_TEST(test_validate_parameter_types_simple_invalid_boolean);
    RUN_TEST(test_validate_parameter_types_simple_invalid_float);
    RUN_TEST(test_validate_parameter_types_simple_invalid_text);
    RUN_TEST(test_validate_parameter_types_simple_multiple_invalid);
    RUN_TEST(test_validate_parameter_types_simple_null_values);
    RUN_TEST(test_validate_parameter_types_simple_float_accepts_integer);

    return UNITY_END();
}