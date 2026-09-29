/*
 * Unity Test File: Firebird Build Error Result
 * Tests firebird_build_error_result() — creates a QueryResult with error info.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/query.h>

#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

/* Forward declaration for function being tested */
QueryResult* firebird_build_error_result(const char* error_msg, DatabaseErrorClass err_class);

/* Test function prototypes */
void test_build_error_result_custom_message(void);
void test_build_error_result_null_message(void);
void test_build_error_result_default_fields(void);
void test_build_error_result_error_class_transport(void);
void test_build_error_result_error_class_other(void);
void test_build_error_result_error_class_none(void);
void test_build_error_result_oom(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_build_error_result_custom_message(void) {
    QueryResult* result = firebird_build_error_result("Custom error message", DB_ERR_TRANSPORT);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_STRING("Custom error message", result->error_message);
    TEST_ASSERT_EQUAL_INT(DB_ERR_TRANSPORT, result->error_class);
    TEST_ASSERT_NOT_NULL(result->data_json);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    TEST_ASSERT_EQUAL(0, result->row_count);
    TEST_ASSERT_EQUAL(0, result->column_count);
    TEST_ASSERT_EQUAL(-1, result->affected_rows);

    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_build_error_result_null_message(void) {
    QueryResult* result = firebird_build_error_result(NULL, DB_ERR_OTHER);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_STRING("Firebird query failed", result->error_message);
    TEST_ASSERT_EQUAL_INT(DB_ERR_OTHER, result->error_class);

    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_build_error_result_default_fields(void) {
    QueryResult* result = firebird_build_error_result("Test error", DB_ERR_TRANSPORT);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_STRING("Test error", result->error_message);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    TEST_ASSERT_EQUAL(0, result->row_count);
    TEST_ASSERT_EQUAL(0, result->column_count);
    TEST_ASSERT_EQUAL(-1, result->affected_rows);
    TEST_ASSERT_NULL(result->column_names);

    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_build_error_result_error_class_transport(void) {
    QueryResult* result = firebird_build_error_result("Transport error", DB_ERR_TRANSPORT);
    TEST_ASSERT_EQUAL_INT(DB_ERR_TRANSPORT, result->error_class);
    TEST_ASSERT_FALSE(result->success);

    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_build_error_result_error_class_other(void) {
    QueryResult* result = firebird_build_error_result("Other error", DB_ERR_OTHER);
    TEST_ASSERT_EQUAL_INT(DB_ERR_OTHER, result->error_class);
    TEST_ASSERT_FALSE(result->success);

    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_build_error_result_error_class_none(void) {
    QueryResult* result = firebird_build_error_result("None error", DB_ERR_NONE);
    TEST_ASSERT_EQUAL_INT(DB_ERR_NONE, result->error_class);
    TEST_ASSERT_FALSE(result->success);

    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_build_error_result_oom(void) {
    mock_system_set_calloc_failure(1);
    QueryResult* result = firebird_build_error_result("OOM error", DB_ERR_OTHER);
    TEST_ASSERT_NULL(result);
    mock_system_reset_all();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_build_error_result_custom_message);
    RUN_TEST(test_build_error_result_null_message);
    RUN_TEST(test_build_error_result_default_fields);
    RUN_TEST(test_build_error_result_error_class_transport);
    RUN_TEST(test_build_error_result_error_class_other);
    RUN_TEST(test_build_error_result_error_class_none);
    RUN_TEST(test_build_error_result_oom);

    return UNITY_END();
}
