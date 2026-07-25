/*
 * Unity Test File: database_execute_test_query_status
 * This file contains unit tests for database_query_status function
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
DatabaseQueryStatus database_query_status(const char* query_id);

// Test function prototypes
void test_database_query_status_null_query_id(void);
void test_database_query_status_empty_query_id(void);
void test_database_query_status_uninitialized_subsystem(void);
void test_database_query_status_no_manager(void);
void test_database_query_status_pending_not_found(void);
void test_database_query_status_timed_out(void);
void test_database_query_status_not_completed(void);
void test_database_query_status_result_null(void);
void test_database_query_status_success(void);
void test_database_query_status_timeout_error_class(void);
void test_database_query_status_other_error(void);

void setUp(void) {
    database_subsystem_init();
}

void tearDown(void) {
    database_subsystem_shutdown();
    mock_system_reset_all();
}

// Test: NULL query_id returns DB_QUERY_ERROR
void test_database_query_status_null_query_id(void) {
    DatabaseQueryStatus result = database_query_status(NULL);
    TEST_ASSERT_EQUAL(DB_QUERY_ERROR, result);
}

// Test: empty query_id returns DB_QUERY_ERROR
void test_database_query_status_empty_query_id(void) {
    DatabaseQueryStatus result = database_query_status("");
    TEST_ASSERT_EQUAL(DB_QUERY_ERROR, result);
}

// Test: uninitialized subsystem returns DB_QUERY_ERROR
void test_database_query_status_uninitialized_subsystem(void) {
    database_subsystem_shutdown();
    DatabaseQueryStatus result = database_query_status("query_123");
    TEST_ASSERT_EQUAL(DB_QUERY_ERROR, result);
}

// Test: get_pending_result_manager() returns NULL (malloc failure)
void test_database_query_status_no_manager(void) {
    mock_system_set_malloc_failure(1);
    DatabaseQueryStatus result = database_query_status("query_123");
    TEST_ASSERT_EQUAL(DB_QUERY_ERROR, result);
}

// Test: pending result not found returns DB_QUERY_ERROR
void test_database_query_status_pending_not_found(void) {
    DatabaseQueryStatus result = database_query_status("nonexistent_query");
    TEST_ASSERT_EQUAL(DB_QUERY_ERROR, result);
}

// Test: pending result timed out returns DB_QUERY_TIMEOUT
void test_database_query_status_timed_out(void) {
    PendingResultManager* manager = get_pending_result_manager();
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "timeout_query", 30, SR_DATABASE);
    TEST_ASSERT_NOT_NULL(pending);

    pending_result_cancel(manager, "timeout_query", SR_DATABASE);

    DatabaseQueryStatus result = database_query_status("timeout_query");
    TEST_ASSERT_EQUAL(DB_QUERY_TIMEOUT, result);
}

// Test: pending result not completed returns DB_QUERY_ERROR
void test_database_query_status_not_completed(void) {
    PendingResultManager* manager = get_pending_result_manager();
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "inflight_query", 30, SR_DATABASE);
    TEST_ASSERT_NOT_NULL(pending);

    DatabaseQueryStatus result = database_query_status("inflight_query");
    TEST_ASSERT_EQUAL(DB_QUERY_ERROR, result);
}

// Test: pending completed but result is NULL returns DB_QUERY_ERROR
void test_database_query_status_result_null(void) {
    PendingResultManager* manager = get_pending_result_manager();
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "null_result_query", 30, SR_DATABASE);
    TEST_ASSERT_NOT_NULL(pending);

    pthread_mutex_lock(&pending->result_lock);
    pending->completed = true;
    pending->result = NULL;
    pthread_mutex_unlock(&pending->result_lock);

    DatabaseQueryStatus result = database_query_status("null_result_query");
    TEST_ASSERT_EQUAL(DB_QUERY_ERROR, result);
}

// Test: successful result returns DB_QUERY_SUCCESS
void test_database_query_status_success(void) {
    PendingResultManager* manager = get_pending_result_manager();
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "success_query", 30, SR_DATABASE);
    TEST_ASSERT_NOT_NULL(pending);

    QueryResult* result_data = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(result_data);
    result_data->success = true;
    result_data->data_json = strdup("{\"status\":\"ok\"}");
    result_data->error_class = DB_ERR_NONE;

    bool signaled = pending_result_signal_ready(manager, "success_query", result_data, SR_DATABASE);
    TEST_ASSERT_TRUE(signaled);

    DatabaseQueryStatus result = database_query_status("success_query");
    TEST_ASSERT_EQUAL(DB_QUERY_SUCCESS, result);
}

// Test: result with DB_ERR_TIMEOUT error_class returns DB_QUERY_TIMEOUT
void test_database_query_status_timeout_error_class(void) {
    PendingResultManager* manager = get_pending_result_manager();
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "timeout_err_query", 30, SR_DATABASE);
    TEST_ASSERT_NOT_NULL(pending);

    QueryResult* result_data = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(result_data);
    result_data->success = false;
    result_data->error_class = DB_ERR_TIMEOUT;
    result_data->error_message = strdup("Query timed out");

    bool signaled = pending_result_signal_ready(manager, "timeout_err_query", result_data, SR_DATABASE);
    TEST_ASSERT_TRUE(signaled);

    DatabaseQueryStatus result = database_query_status("timeout_err_query");
    TEST_ASSERT_EQUAL(DB_QUERY_TIMEOUT, result);
}

// Test: result with other error_class returns DB_QUERY_ERROR
void test_database_query_status_other_error(void) {
    PendingResultManager* manager = get_pending_result_manager();
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "other_err_query", 30, SR_DATABASE);
    TEST_ASSERT_NOT_NULL(pending);

    QueryResult* result_data = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(result_data);
    result_data->success = false;
    result_data->error_class = DB_ERR_OTHER;
    result_data->error_message = strdup("Syntax error");

    bool signaled = pending_result_signal_ready(manager, "other_err_query", result_data, SR_DATABASE);
    TEST_ASSERT_TRUE(signaled);

    DatabaseQueryStatus result = database_query_status("other_err_query");
    TEST_ASSERT_EQUAL(DB_QUERY_ERROR, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_query_status_null_query_id);
    RUN_TEST(test_database_query_status_empty_query_id);
    RUN_TEST(test_database_query_status_uninitialized_subsystem);
    RUN_TEST(test_database_query_status_no_manager);
    RUN_TEST(test_database_query_status_pending_not_found);
    RUN_TEST(test_database_query_status_timed_out);
    RUN_TEST(test_database_query_status_not_completed);
    RUN_TEST(test_database_query_status_result_null);
    RUN_TEST(test_database_query_status_success);
    RUN_TEST(test_database_query_status_timeout_error_class);
    RUN_TEST(test_database_query_status_other_error);

    return UNITY_END();
}
