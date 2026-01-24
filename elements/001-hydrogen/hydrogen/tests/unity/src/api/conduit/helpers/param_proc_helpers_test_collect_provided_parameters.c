/*
 * Unity Test File: Collect Provided Parameters
 * This file contains unit tests for collect_provided_parameters() function
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

static void test_collect_provided_parameters_basic(void) {
    // Test basic functionality
    json_t* params_json = json_object();
    json_t* integer_params = json_object();
    json_t* string_params = json_object();
    
    json_object_set(integer_params, "userId", json_integer(123));
    json_object_set(integer_params, "limit", json_integer(50));
    json_object_set(string_params, "username", json_string("johndoe"));
    json_object_set(string_params, "email", json_string("user@example.com"));
    
    json_object_set(params_json, "INTEGER", integer_params);
    json_object_set(params_json, "STRING", string_params);
    
    size_t count = 0;
    char** params = collect_provided_parameters(params_json, &count);
    
    TEST_ASSERT_NOT_NULL(params);
    TEST_ASSERT_EQUAL_INT(4, count);
    
    // Check if all parameters are present (order may vary)
    bool has_userId = false;
    bool has_limit = false;
    bool has_username = false;
    bool has_email = false;
    
    for (size_t i = 0; i < count; i++) {
        if (strcmp(params[i], "userId") == 0) has_userId = true;
        if (strcmp(params[i], "limit") == 0) has_limit = true;
        if (strcmp(params[i], "username") == 0) has_username = true;
        if (strcmp(params[i], "email") == 0) has_email = true;
    }
    
    TEST_ASSERT_TRUE(has_userId);
    TEST_ASSERT_TRUE(has_limit);
    TEST_ASSERT_TRUE(has_username);
    TEST_ASSERT_TRUE(has_email);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free(params[i]);
    }
    free(params);
    json_decref(params_json);
}

static void test_collect_provided_parameters_empty(void) {
    // Test empty JSON object
    json_t* params_json = json_object();
    
    size_t count = 0;
    char** params = collect_provided_parameters(params_json, &count);
    
    TEST_ASSERT_NULL(params);
    TEST_ASSERT_EQUAL_INT(0, count);
    
    json_decref(params_json);
}

static void test_collect_provided_parameters_null(void) {
    // Test null JSON object
    size_t count = 0;
    char** params = collect_provided_parameters(NULL, &count);
    
    TEST_ASSERT_NULL(params);
    TEST_ASSERT_EQUAL_INT(0, count);
}

static void test_collect_provided_parameters_no_types(void) {
    // Test JSON without any type sections
    json_t* params_json = json_object();
    json_object_set(params_json, "INVALID_TYPE", json_string("not an object"));
    
    size_t count = 0;
    char** params = collect_provided_parameters(params_json, &count);
    
    TEST_ASSERT_NULL(params);
    TEST_ASSERT_EQUAL_INT(0, count);
    
    json_decref(params_json);
}

static void test_collect_provided_parameters_duplicate_params(void) {
    // Test duplicate parameters across type sections
    json_t* params_json = json_object();
    json_t* integer_params = json_object();
    json_t* string_params = json_object();
    
    json_object_set(integer_params, "id", json_integer(123));
    json_object_set(string_params, "id", json_string("123")); // Same parameter name in different type
    
    json_object_set(params_json, "INTEGER", integer_params);
    json_object_set(params_json, "STRING", string_params);
    
    size_t count = 0;
    char** params = collect_provided_parameters(params_json, &count);
    
    TEST_ASSERT_NOT_NULL(params);
    TEST_ASSERT_EQUAL_INT(1, count); // Should only collect once
    TEST_ASSERT_EQUAL_STRING("id", params[0]);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free(params[i]);
    }
    free(params);
    json_decref(params_json);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_collect_provided_parameters_basic);
    RUN_TEST(test_collect_provided_parameters_empty);
    RUN_TEST(test_collect_provided_parameters_null);
    RUN_TEST(test_collect_provided_parameters_no_types);
    RUN_TEST(test_collect_provided_parameters_duplicate_params);
    
    return UNITY_END();
}
