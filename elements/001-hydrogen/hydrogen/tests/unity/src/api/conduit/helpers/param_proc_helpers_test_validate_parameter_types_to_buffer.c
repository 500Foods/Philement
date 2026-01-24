/*
 * Unity Test File: Validate Parameter Types to Buffer
 * This file contains unit tests for validate_parameter_types_to_buffer() function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/api/conduit/helpers/param_proc_helpers.h>

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

static void test_validate_parameter_types_to_buffer_basic(void) {
    json_t* params_json = json_object();
    json_t* integer_params = json_object();
    json_t* string_params = json_object();
    
    json_object_set(integer_params, "userId", json_integer(123));
    json_object_set(integer_params, "age", json_string("30")); // Wrong type
    json_object_set(string_params, "username", json_string("johndoe"));
    json_object_set(string_params, "email", json_integer(123)); // Wrong type
    
    json_object_set(params_json, "INTEGER", integer_params);
    json_object_set(params_json, "STRING", string_params);
    
    char buffer[1024] = {0};
    size_t written = validate_parameter_types_to_buffer(params_json, buffer, sizeof(buffer));
    
    TEST_ASSERT_GREATER_THAN_INT(0, written);
    TEST_ASSERT_TRUE(strstr(buffer, "age") != NULL);
    TEST_ASSERT_TRUE(strstr(buffer, "email") != NULL);
    
    json_decref(params_json);
}

static void test_validate_parameter_types_to_buffer_no_errors(void) {
    json_t* params_json = json_object();
    json_t* integer_params = json_object();
    json_t* string_params = json_object();
    
    json_object_set(integer_params, "userId", json_integer(123));
    json_object_set(integer_params, "age", json_integer(30));
    json_object_set(string_params, "username", json_string("johndoe"));
    json_object_set(string_params, "email", json_string("user@example.com"));
    
    json_object_set(params_json, "INTEGER", integer_params);
    json_object_set(params_json, "STRING", string_params);
    
    char buffer[1024] = {0};
    size_t written = validate_parameter_types_to_buffer(params_json, buffer, sizeof(buffer));
    
    TEST_ASSERT_EQUAL_INT(0, written);
    TEST_ASSERT_EQUAL_STRING("", buffer);
    
    json_decref(params_json);
}

static void test_validate_parameter_types_to_buffer_null_params(void) {
    char buffer[1024] = {0};
    size_t written = validate_parameter_types_to_buffer(NULL, buffer, sizeof(buffer));
    
    TEST_ASSERT_EQUAL_INT(0, written);
    TEST_ASSERT_EQUAL_STRING("", buffer);
}

static void test_validate_parameter_types_to_buffer_empty_params(void) {
    json_t* params_json = json_object();
    
    char buffer[1024] = {0};
    size_t written = validate_parameter_types_to_buffer(params_json, buffer, sizeof(buffer));
    
    TEST_ASSERT_EQUAL_INT(0, written);
    TEST_ASSERT_EQUAL_STRING("", buffer);
    
    json_decref(params_json);
}

static void test_validate_parameter_types_to_buffer_small_buffer(void) {
    json_t* params_json = json_object();
    json_t* integer_params = json_object();
    
    json_object_set(integer_params, "very_long_parameter_name_that_will_cause_buffer_overflow", json_string("not an integer"));
    json_object_set(params_json, "INTEGER", integer_params);
    
    char buffer[30] = {0};
    size_t written = validate_parameter_types_to_buffer(params_json, buffer, sizeof(buffer));
    
    printf("Buffer size: %zu, Written: %zu\n", sizeof(buffer), written);
    printf("Buffer content: '%s'\n", buffer);
    
    TEST_ASSERT_TRUE(written > 0);
    TEST_ASSERT_LESS_OR_EQUAL_INT(sizeof(buffer), written);
    
    json_decref(params_json);
}

static void test_validate_parameter_types_to_buffer_null_buffer(void) {
    json_t* params_json = json_object();
    json_t* integer_params = json_object();
    
    json_object_set(integer_params, "userId", json_string("123"));
    json_object_set(params_json, "INTEGER", integer_params);
    
    size_t written = validate_parameter_types_to_buffer(params_json, NULL, 0);
    
    TEST_ASSERT_EQUAL_INT(0, written);
    
    json_decref(params_json);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_validate_parameter_types_to_buffer_basic);
    RUN_TEST(test_validate_parameter_types_to_buffer_no_errors);
    RUN_TEST(test_validate_parameter_types_to_buffer_null_params);
    RUN_TEST(test_validate_parameter_types_to_buffer_empty_params);
    RUN_TEST(test_validate_parameter_types_to_buffer_small_buffer);
    RUN_TEST(test_validate_parameter_types_to_buffer_null_buffer);
    
    return UNITY_END();
}
