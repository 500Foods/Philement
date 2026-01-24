/*
 * Unity Test File: parameter_processing_test_analyze_parameter_validation.c
 * This file contains unit tests for the analyze_parameter_validation function
 */

// Standard includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <jansson.h>

// Include source header
#include <src/api/conduit/conduit_helpers.h>

// Forward declarations for functions being tested
bool analyze_parameter_validation(const char* sql_template, json_t* params_json,
                                  char*** missing_params, size_t* missing_count,
                                  char*** unused_params, size_t* unused_count,
                                  char* invalid_buffer, size_t invalid_buffer_size, size_t* invalid_pos);

void setUp(void) {
    // No setup needed for these tests
}

void tearDown(void) {
    // No cleanup needed for these tests
}

// Test NULL sql_template
static void test_analyze_parameter_validation_null_sql_template(void) {
    json_t* params = json_object();
    char** missing = NULL;
    size_t missing_count = 0;
    char** unused = NULL;
    size_t unused_count = 0;
    char invalid_buffer[1024] = {0};
    size_t invalid_pos = 0;

    bool result = analyze_parameter_validation(NULL, params, &missing, &missing_count,
                                               &unused, &unused_count, invalid_buffer,
                                               sizeof(invalid_buffer), &invalid_pos);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(missing);
    TEST_ASSERT_EQUAL(0, missing_count);
    TEST_ASSERT_NULL(unused);
    TEST_ASSERT_EQUAL(0, unused_count);
    TEST_ASSERT_EQUAL(0, invalid_pos);

    json_decref(params);
}

// Test empty sql_template
static void test_analyze_parameter_validation_empty_sql_template(void) {
    json_t* params = json_object();
    char** missing = NULL;
    size_t missing_count = 0;
    char** unused = NULL;
    size_t unused_count = 0;
    char invalid_buffer[1024] = {0};
    size_t invalid_pos = 0;

    bool result = analyze_parameter_validation("", params, &missing, &missing_count,
                                               &unused, &unused_count, invalid_buffer,
                                               sizeof(invalid_buffer), &invalid_pos);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(missing);
    TEST_ASSERT_EQUAL(0, missing_count);
    TEST_ASSERT_NULL(unused);
    TEST_ASSERT_EQUAL(0, unused_count);
    TEST_ASSERT_EQUAL(0, invalid_pos);

    json_decref(params);
}

// Test NULL params_json
static void test_analyze_parameter_validation_null_params(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId";
    char** missing = NULL;
    size_t missing_count = 0;
    char** unused = NULL;
    size_t unused_count = 0;
    char invalid_buffer[1024] = {0};
    size_t invalid_pos = 0;

    bool result = analyze_parameter_validation(sql, NULL, &missing, &missing_count,
                                               &unused, &unused_count, invalid_buffer,
                                               sizeof(invalid_buffer), &invalid_pos);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(missing);
    TEST_ASSERT_EQUAL(1, missing_count);
    TEST_ASSERT_EQUAL_STRING("userId", missing[0]);
    TEST_ASSERT_NULL(unused);
    TEST_ASSERT_EQUAL(0, unused_count);
    TEST_ASSERT_EQUAL(0, invalid_pos);

    // Cleanup
    free(missing[0]);
    free(missing);
}

// Test valid parameters - no missing or unused
static void test_analyze_parameter_validation_valid_params(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId AND name = :userName";
    json_t* params = json_object();
    json_t* integer_obj = json_object();
    json_t* string_obj = json_object();

    json_object_set_new(integer_obj, "userId", json_integer(123));
    json_object_set_new(string_obj, "userName", json_string("test"));

    json_object_set_new(params, "INTEGER", integer_obj);
    json_object_set_new(params, "STRING", string_obj);

    char** missing = NULL;
    size_t missing_count = 0;
    char** unused = NULL;
    size_t unused_count = 0;
    char invalid_buffer[1024] = {0};
    size_t invalid_pos = 0;

    bool result = analyze_parameter_validation(sql, params, &missing, &missing_count,
                                               &unused, &unused_count, invalid_buffer,
                                               sizeof(invalid_buffer), &invalid_pos);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(missing);
    TEST_ASSERT_EQUAL(0, missing_count);
    TEST_ASSERT_NULL(unused);
    TEST_ASSERT_EQUAL(0, unused_count);
    TEST_ASSERT_EQUAL(0, invalid_pos);

    json_decref(params);
}

// Test missing parameters
static void test_analyze_parameter_validation_missing_params(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId AND name = :userName";
    json_t* params = json_object();
    json_t* integer_obj = json_object();

    json_object_set_new(integer_obj, "userId", json_integer(123));
    json_object_set_new(params, "INTEGER", integer_obj);

    char** missing = NULL;
    size_t missing_count = 0;
    char** unused = NULL;
    size_t unused_count = 0;
    char invalid_buffer[1024] = {0};
    size_t invalid_pos = 0;

    bool result = analyze_parameter_validation(sql, params, &missing, &missing_count,
                                               &unused, &unused_count, invalid_buffer,
                                               sizeof(invalid_buffer), &invalid_pos);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(missing);
    TEST_ASSERT_EQUAL(1, missing_count);
    TEST_ASSERT_EQUAL_STRING("userName", missing[0]);
    TEST_ASSERT_NULL(unused);
    TEST_ASSERT_EQUAL(0, unused_count);
    TEST_ASSERT_EQUAL(0, invalid_pos);

    // Cleanup
    free(missing[0]);
    free(missing);
    json_decref(params);
}

// Test unused parameters
static void test_analyze_parameter_validation_unused_params(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId";
    json_t* params = json_object();
    json_t* integer_obj = json_object();
    json_t* string_obj = json_object();

    json_object_set_new(integer_obj, "userId", json_integer(123));
    json_object_set_new(integer_obj, "unusedId", json_integer(456));
    json_object_set_new(string_obj, "userName", json_string("test"));

    json_object_set_new(params, "INTEGER", integer_obj);
    json_object_set_new(params, "STRING", string_obj);

    char** missing = NULL;
    size_t missing_count = 0;
    char** unused = NULL;
    size_t unused_count = 0;
    char invalid_buffer[1024] = {0};
    size_t invalid_pos = 0;

    bool result = analyze_parameter_validation(sql, params, &missing, &missing_count,
                                               &unused, &unused_count, invalid_buffer,
                                               sizeof(invalid_buffer), &invalid_pos);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(missing);
    TEST_ASSERT_EQUAL(0, missing_count);
    TEST_ASSERT_NOT_NULL(unused);
    TEST_ASSERT_EQUAL(2, unused_count);
    TEST_ASSERT_EQUAL_STRING("unusedId", unused[0]);
    TEST_ASSERT_EQUAL_STRING("userName", unused[1]);
    TEST_ASSERT_EQUAL(0, invalid_pos);

    // Cleanup
    free(unused[0]);
    free(unused[1]);
    free(unused);
    json_decref(params);
}

// Test invalid parameter types
static void test_analyze_parameter_validation_invalid_types(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId";
    json_t* params = json_object();
    json_t* integer_obj = json_object();

    // Wrong type: boolean in INTEGER section
    json_object_set_new(integer_obj, "userId", json_true());
    json_object_set_new(params, "INTEGER", integer_obj);

    char** missing = NULL;
    size_t missing_count = 0;
    char** unused = NULL;
    size_t unused_count = 0;
    char invalid_buffer[1024] = {0};
    size_t invalid_pos = 0;

    bool result = analyze_parameter_validation(sql, params, &missing, &missing_count,
                                                &unused, &unused_count, invalid_buffer,
                                                sizeof(invalid_buffer), &invalid_pos);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(missing);
    TEST_ASSERT_EQUAL(0, missing_count);
    TEST_ASSERT_NULL(unused);
    TEST_ASSERT_EQUAL(0, unused_count);
    TEST_ASSERT_GREATER_THAN(0, invalid_pos);
    TEST_ASSERT_NOT_NULL(strstr(invalid_buffer, "userId(boolean) should be userId(INTEGER)"));

    json_decref(params);
}

// Test duplicate parameters in SQL template
static void test_analyze_parameter_validation_duplicate_sql_params(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId OR parent_id = :userId";
    json_t* params = json_object();
    json_t* integer_obj = json_object();

    json_object_set_new(integer_obj, "userId", json_integer(123));
    json_object_set_new(params, "INTEGER", integer_obj);

    char** missing = NULL;
    size_t missing_count = 0;
    char** unused = NULL;
    size_t unused_count = 0;
    char invalid_buffer[1024] = {0};
    size_t invalid_pos = 0;

    bool result = analyze_parameter_validation(sql, params, &missing, &missing_count,
                                               &unused, &unused_count, invalid_buffer,
                                               sizeof(invalid_buffer), &invalid_pos);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(missing);
    TEST_ASSERT_EQUAL(0, missing_count);
    TEST_ASSERT_NULL(unused);
    TEST_ASSERT_EQUAL(0, unused_count);
    TEST_ASSERT_EQUAL(0, invalid_pos);

    json_decref(params);
}

// Test empty parameter objects
static void test_analyze_parameter_validation_empty_param_objects(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId";
    json_t* params = json_object();
    json_t* integer_obj = json_object();
    json_t* string_obj = json_object();

    // Empty objects
    json_object_set_new(params, "INTEGER", integer_obj);
    json_object_set_new(params, "STRING", string_obj);

    char** missing = NULL;
    size_t missing_count = 0;
    char** unused = NULL;
    size_t unused_count = 0;
    char invalid_buffer[1024] = {0};
    size_t invalid_pos = 0;

    bool result = analyze_parameter_validation(sql, params, &missing, &missing_count,
                                                &unused, &unused_count, invalid_buffer,
                                                sizeof(invalid_buffer), &invalid_pos);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(missing);
    TEST_ASSERT_EQUAL(1, missing_count);
    TEST_ASSERT_EQUAL_STRING("userId", missing[0]);
    TEST_ASSERT_NULL(unused);
    TEST_ASSERT_EQUAL(0, unused_count);
    TEST_ASSERT_EQUAL(0, invalid_pos);

    // Cleanup
    free(missing[0]);
    free(missing);
    json_decref(params);
}


int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_analyze_parameter_validation_null_sql_template);
    RUN_TEST(test_analyze_parameter_validation_empty_sql_template);
    RUN_TEST(test_analyze_parameter_validation_null_params);
    RUN_TEST(test_analyze_parameter_validation_valid_params);
    RUN_TEST(test_analyze_parameter_validation_missing_params);
    RUN_TEST(test_analyze_parameter_validation_unused_params);
    RUN_TEST(test_analyze_parameter_validation_invalid_types);
    RUN_TEST(test_analyze_parameter_validation_duplicate_sql_params);
    RUN_TEST(test_analyze_parameter_validation_empty_param_objects);

    return UNITY_END();
}