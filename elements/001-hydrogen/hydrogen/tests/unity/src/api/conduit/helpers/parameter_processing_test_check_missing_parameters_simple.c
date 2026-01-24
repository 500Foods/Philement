/*
 * Unity Test File: parameter_processing_test_check_missing_parameters_simple.c
 * This file contains unit tests for the check_missing_parameters_simple function
 */

// Standard includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <jansson.h>

// Include source header
#include <src/api/conduit/conduit_helpers.h>

// Forward declarations for functions being tested
char* check_missing_parameters_simple(const char* sql_template, ParameterList* param_list);

void setUp(void) {
    // No setup needed for these tests
}

void tearDown(void) {
    // No cleanup needed for these tests
}

// Test NULL sql_template
static void test_check_missing_parameters_simple_null_sql_template(void) {
    ParameterList param_list = {0};
    char* result = check_missing_parameters_simple(NULL, &param_list);
    TEST_ASSERT_NULL(result);
}

// Test empty sql_template
static void test_check_missing_parameters_simple_empty_sql_template(void) {
    ParameterList param_list = {0};
    char* result = check_missing_parameters_simple("", &param_list);
    TEST_ASSERT_NULL(result);
}

// Test NULL param_list
static void test_check_missing_parameters_simple_null_param_list(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId";
    char* result = check_missing_parameters_simple(sql, NULL);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "userId"));
    free(result);
}

// Test no missing parameters
static void test_check_missing_parameters_simple_no_missing(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId";
    ParameterList param_list = {0};
    param_list.count = 1;
    param_list.params = calloc(1, sizeof(TypedParameter*));
    param_list.params[0] = calloc(1, sizeof(TypedParameter));
    param_list.params[0]->name = strdup("userId");

    char* result = check_missing_parameters_simple(sql, &param_list);
    TEST_ASSERT_NULL(result);

    // Cleanup
    free(param_list.params[0]->name);
    free(param_list.params[0]);
    free(param_list.params);
}

// Test missing parameters
static void test_check_missing_parameters_simple_missing_params(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId AND name = :userName";
    ParameterList param_list = {0};
    param_list.count = 1;
    param_list.params = calloc(1, sizeof(TypedParameter*));
    param_list.params[0] = calloc(1, sizeof(TypedParameter));
    param_list.params[0]->name = strdup("userId");

    char* result = check_missing_parameters_simple(sql, &param_list);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "userName"));
    free(result);

    // Cleanup
    free(param_list.params[0]->name);
    free(param_list.params[0]);
    free(param_list.params);
}

// Test multiple missing parameters
static void test_check_missing_parameters_simple_multiple_missing(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId AND name = :userName AND age = :userAge";
    ParameterList param_list = {0};
    param_list.count = 1;
    param_list.params = calloc(1, sizeof(TypedParameter*));
    param_list.params[0] = calloc(1, sizeof(TypedParameter));
    param_list.params[0]->name = strdup("userId");

    char* result = check_missing_parameters_simple(sql, &param_list);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "userName"));
    TEST_ASSERT_NOT_NULL(strstr(result, "userAge"));
    free(result);

    // Cleanup
    free(param_list.params[0]->name);
    free(param_list.params[0]);
    free(param_list.params);
}

// Test duplicate parameters in SQL template
static void test_check_missing_parameters_simple_duplicate_sql_params(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId OR parent_id = :userId";
    ParameterList param_list = {0};
    param_list.count = 1;
    param_list.params = calloc(1, sizeof(TypedParameter*));
    param_list.params[0] = calloc(1, sizeof(TypedParameter));
    param_list.params[0]->name = strdup("userId");

    char* result = check_missing_parameters_simple(sql, &param_list);
    TEST_ASSERT_NULL(result);

    // Cleanup
    free(param_list.params[0]->name);
    free(param_list.params[0]);
    free(param_list.params);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_missing_parameters_simple_null_sql_template);
    RUN_TEST(test_check_missing_parameters_simple_empty_sql_template);
    RUN_TEST(test_check_missing_parameters_simple_null_param_list);
    RUN_TEST(test_check_missing_parameters_simple_no_missing);
    RUN_TEST(test_check_missing_parameters_simple_missing_params);
    RUN_TEST(test_check_missing_parameters_simple_multiple_missing);
    RUN_TEST(test_check_missing_parameters_simple_duplicate_sql_params);

    return UNITY_END();
}