/*
 * Unity Test File: orchestrator_test_extract_code_from_result.c
 *
 * Exercises orchestrator_extract_code_from_result, the helper that
 * pulls the `code` column out of a conduit query result (a
 * JSON array-of-objects in QueryResult->data_json). It is a pure
 * JSON-parsing function, so every branch is reachable from a Unity
 * test without any database or lua infrastructure:
 *
 *   - NULL input string
 *   - malformed JSON (json_loads failure)
 *   - valid JSON that is not an array
 *   - an array with zero rows
 *   - an array whose first row is not an object
 *   - an array whose first row has no `code` key
 *   - an array whose first row has a `code` that is not a string
 *   - a happy-path row that yields the code string
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <stdlib.h>
#include <string.h>

// Modules under test
#include <src/scripting/orchestrator.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_extract_code_null_input(void);
void test_extract_code_malformed_json(void);
void test_extract_code_not_an_array(void);
void test_extract_code_empty_array(void);
void test_extract_code_row_not_object(void);
void test_extract_code_missing_code_key(void);
void test_extract_code_code_not_string(void);
void test_extract_code_happy_path(void);

void setUp(void) {
    // No global state to manage for this pure parser.
}

void tearDown(void) {
    // Nothing to clean up between tests.
}

void test_extract_code_null_input(void) {
    char* code = orchestrator_extract_code_from_result(NULL);
    TEST_ASSERT_NULL(code);
}

void test_extract_code_malformed_json(void) {
    char* code = orchestrator_extract_code_from_result("{not valid json");
    TEST_ASSERT_NULL(code);
}

void test_extract_code_not_an_array(void) {
    char* code = orchestrator_extract_code_from_result("{\"foo\":\"bar\"}");
    TEST_ASSERT_NULL(code);
}

void test_extract_code_empty_array(void) {
    char* code = orchestrator_extract_code_from_result("[]");
    TEST_ASSERT_NULL(code);
}

void test_extract_code_row_not_object(void) {
    char* code = orchestrator_extract_code_from_result("[1, 2, 3]");
    TEST_ASSERT_NULL(code);
}

void test_extract_code_missing_code_key(void) {
    char* code = orchestrator_extract_code_from_result(
        "[{\"group_name\":\"Orchestrators\",\"script_name\":\"Orchestrator\"}]");
    TEST_ASSERT_NULL(code);
}

void test_extract_code_code_not_string(void) {
    // `code` present but typed as an integer, not a string.
    char* code = orchestrator_extract_code_from_result(
        "[{\"code\":12345}]");
    TEST_ASSERT_NULL(code);
}

void test_extract_code_happy_path(void) {
    char* code = orchestrator_extract_code_from_result(
        "[{\"code\":\"print('hello from orchestrator')\"}]");
    TEST_ASSERT_NOT_NULL(code);
    TEST_ASSERT_EQUAL_STRING("print('hello from orchestrator')", code);
    free(code);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_extract_code_null_input);
    RUN_TEST(test_extract_code_malformed_json);
    RUN_TEST(test_extract_code_not_an_array);
    RUN_TEST(test_extract_code_empty_array);
    RUN_TEST(test_extract_code_row_not_object);
    RUN_TEST(test_extract_code_missing_code_key);
    RUN_TEST(test_extract_code_code_not_string);
    RUN_TEST(test_extract_code_happy_path);

    return UNITY_END();
}
