/*
 * Unity Test File: database_execute_test_get_result
 * This file contains unit tests for database_get_result function
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/database_pending.h>
#include <src/database/database_engine.h>

// Include mock system header for malloc failure testing
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Forward declarations for functions being tested
bool database_get_result(const char* query_id, char* result_buffer, size_t buffer_size);

// Test function prototypes
void test_database_get_result_basic_functionality(void);
void test_database_get_result_null_query_id(void);
void test_database_get_result_null_result_buffer(void);
void test_database_get_result_zero_buffer_size(void);
void test_database_get_result_empty_query_id(void);
void test_database_get_result_uninitialized_subsystem(void);
void test_database_get_result_no_manager(void);
void test_database_get_result_pending_not_found(void);
void test_database_get_result_not_completed(void);
void test_database_get_result_no_data_json(void);
void test_database_get_result_buffer_too_small(void);
void test_database_get_result_success(void);

void setUp(void) {
    database_subsystem_init();
}

void tearDown(void) {
    database_subsystem_shutdown();
    mock_system_reset_all();
}

// Test database_get_result function
void test_database_get_result_basic_functionality(void) {
    char buffer[256] = {0};
    bool result = database_get_result("query_123", buffer, sizeof(buffer));
    TEST_ASSERT_FALSE(result);
}

void test_database_get_result_null_query_id(void) {
    char buffer[256] = {0};
    bool result = database_get_result(NULL, buffer, sizeof(buffer));
    TEST_ASSERT_FALSE(result);
}

void test_database_get_result_null_result_buffer(void) {
    bool result = database_get_result("query_123", NULL, 256);
    TEST_ASSERT_FALSE(result);
}

void test_database_get_result_zero_buffer_size(void) {
    char buffer[256] = {0};
    bool result = database_get_result("query_123", buffer, 0);
    TEST_ASSERT_FALSE(result);
}

void test_database_get_result_empty_query_id(void) {
    char buffer[256] = {0};
    bool result = database_get_result("", buffer, sizeof(buffer));
    TEST_ASSERT_FALSE(result);
}

void test_database_get_result_uninitialized_subsystem(void) {
    database_subsystem_shutdown();
    char buffer[256] = {0};
    bool result = database_get_result("query_123", buffer, sizeof(buffer));
    TEST_ASSERT_FALSE(result);
}

// Test: get_pending_result_manager() returns NULL (malloc failure)
void test_database_get_result_no_manager(void) {
    mock_system_set_malloc_failure(1);
    char buffer[256] = {0};
    bool result = database_get_result("query_123", buffer, sizeof(buffer));
    TEST_ASSERT_FALSE(result);
}

// Test: pending result not found returns false
void test_database_get_result_pending_not_found(void) {
    char buffer[256] = {0};
    bool result = database_get_result("nonexistent_query", buffer, sizeof(buffer));
    TEST_ASSERT_FALSE(result);
}

// Test: pending result not completed returns false
void test_database_get_result_not_completed(void) {
    PendingResultManager* manager = get_pending_result_manager();
    TEST_ASSERT_NOT_NULL(manager);

    pending_result_register(manager, "inflight_query", 30, SR_DATABASE);

    char buffer[256] = {0};
    bool result = database_get_result("inflight_query", buffer, sizeof(buffer));
    TEST_ASSERT_FALSE(result);
}

// Test: result with no data_json returns false
void test_database_get_result_no_data_json(void) {
    PendingResultManager* manager = get_pending_result_manager();
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "no_data_query", 30, SR_DATABASE);
    TEST_ASSERT_NOT_NULL(pending);

    QueryResult* result_data = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(result_data);
    result_data->success = true;
    result_data->data_json = NULL;

    pending_result_signal_ready(manager, "no_data_query", result_data, SR_DATABASE);

    char buffer[256] = {0};
    bool result = database_get_result("no_data_query", buffer, sizeof(buffer));
    TEST_ASSERT_FALSE(result);
}

// Test: buffer too small for result data returns false
void test_database_get_result_buffer_too_small(void) {
    PendingResultManager* manager = get_pending_result_manager();
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "small_buf_query", 30, SR_DATABASE);
    TEST_ASSERT_NOT_NULL(pending);

    QueryResult* result_data = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(result_data);
    result_data->success = true;
    result_data->data_json = strdup("{\"this_is_a_very_long_json_result\": 12345}");

    pending_result_signal_ready(manager, "small_buf_query", result_data, SR_DATABASE);

    char buffer[8] = {0};
    bool result = database_get_result("small_buf_query", buffer, sizeof(buffer));
    TEST_ASSERT_FALSE(result);
}

// Test: successful result copy returns true
void test_database_get_result_success(void) {
    PendingResultManager* manager = get_pending_result_manager();
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "success_get_query", 30, SR_DATABASE);
    TEST_ASSERT_NOT_NULL(pending);

    QueryResult* result_data = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(result_data);
    result_data->success = true;
    result_data->data_json = strdup("{\"status\":\"ok\"}");

    pending_result_signal_ready(manager, "success_get_query", result_data, SR_DATABASE);

    char buffer[256] = {0};
    bool result = database_get_result("success_get_query", buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("{\"status\":\"ok\"}", buffer);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_get_result_basic_functionality);
    RUN_TEST(test_database_get_result_null_query_id);
    RUN_TEST(test_database_get_result_null_result_buffer);
    RUN_TEST(test_database_get_result_zero_buffer_size);
    RUN_TEST(test_database_get_result_empty_query_id);
    RUN_TEST(test_database_get_result_uninitialized_subsystem);
    RUN_TEST(test_database_get_result_no_manager);
    RUN_TEST(test_database_get_result_pending_not_found);
    RUN_TEST(test_database_get_result_not_completed);
    RUN_TEST(test_database_get_result_no_data_json);
    RUN_TEST(test_database_get_result_buffer_too_small);
    RUN_TEST(test_database_get_result_success);

    return UNITY_END();
}
