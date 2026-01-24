/*
 * Unity Test File: parameter_processing_test_process_parameters.c
 * This file contains unit tests for the process_parameters function
 */

// Standard includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <jansson.h>

// Enable mock system functions for unit testing
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Include source header
#include <src/api/conduit/conduit_helpers.h>

// Forward declarations for functions being tested
#ifndef USE_MOCK_PROCESS_PARAMETERS
bool process_parameters(json_t* params_json, ParameterList** param_list,
                        const char* sql_template, DatabaseEngineType engine_type,
                        char** converted_sql, TypedParameter*** ordered_params, size_t* param_count);
#endif

void setUp(void) {
    // Reset all mocks to default state
    mock_system_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_system_reset_all();
}

// Test successful parameter processing
static void test_process_parameters_success(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId";
    json_t* params = json_object();
    json_t* integer_obj = json_object();

    json_object_set_new(integer_obj, "userId", json_integer(123));
    json_object_set_new(params, "INTEGER", integer_obj);

    ParameterList* param_list = NULL;
    char* converted_sql = NULL;
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;

    bool result = process_parameters(params, &param_list, sql, DB_ENGINE_POSTGRESQL,
                                   &converted_sql, &ordered_params, &param_count);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(param_list);
    TEST_ASSERT_NOT_NULL(converted_sql);
    TEST_ASSERT_NOT_NULL(ordered_params);
    TEST_ASSERT_GREATER_THAN(0, param_count);

    // Cleanup
    if (param_list) {
        if (param_list->params) {
            for (size_t i = 0; i < param_list->count; i++) {
                if (param_list->params[i]) {
                    free(param_list->params[i]->name);
                    free(param_list->params[i]);
                }
            }
            free(param_list->params);
        }
        free(param_list);
    }
    free(converted_sql);
    free(ordered_params);

    json_decref(params);
}

// Test NULL params_json with SQL that has parameters - should fail
static void test_process_parameters_null_params(void) {
    const char* sql = "SELECT * FROM table WHERE id = :userId";

    ParameterList* param_list = NULL;
    char* converted_sql = NULL;
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;

    bool result = process_parameters(NULL, &param_list, sql, DB_ENGINE_POSTGRESQL,
                                   &converted_sql, &ordered_params, &param_count);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(param_list);  // Empty param_list is created
    TEST_ASSERT_NULL(converted_sql);
    TEST_ASSERT_NULL(ordered_params);
    TEST_ASSERT_EQUAL(0, param_count);

    // Cleanup
    if (param_list) {
        if (param_list->params) {
            free(param_list->params);
        }
        free(param_list);
    }
}



int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_process_parameters_success);
    RUN_TEST(test_process_parameters_null_params);

    return UNITY_END();
}