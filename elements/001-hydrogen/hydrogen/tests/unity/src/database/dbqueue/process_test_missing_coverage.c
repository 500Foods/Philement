/*
 * Unity Test File: process_test_missing_coverage
 * This file contains unit tests to cover the remaining uncovered lines in process.c
 *
 * NOTE: In the Unity test environment, USE_MOCK_PTHREAD is defined for the
 * dbqueue source files. This means pthread_create is redirected to
 * mock_pthread_create, which does NOT actually spawn a real thread. The
 * tests below therefore exercise database_queue_worker_thread() directly
 * (in-line, in the main thread) rather than via start_worker + join.
 */

// IMPORTANT: Define mocks BEFORE including any source headers
#define USE_MOCK_DATABASE_ENGINE

// Include mock headers immediately after defines
#include <unity/mocks/mock_database_engine.h>

// Now include Unity framework
#include <unity.h>

// Now include source headers (functions will be mocked)
#include <src/hydrogen.h>
#include <semaphore.h>
#include <pthread.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Forward declaration for the worker thread function (tested directly)
extern void* database_queue_worker_thread(void* arg);

// Test function prototypes
void test_database_queue_process_single_query_success(void);
void test_database_queue_process_single_query_no_connection(void);
void test_database_queue_process_single_query_failure(void);
void test_database_queue_worker_thread_main_loop_shutdown_check(void);
void test_database_queue_start_worker_failure_path(void);
void test_database_queue_manage_child_queues_scaling_down(void);
void test_database_queue_process_single_query_null_queue(void);
void test_database_queue_worker_thread_main_loop_processing(void);

void setUp(void) {
    // Initialize subsystems for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
    mock_database_engine_reset_all();
}

void tearDown(void) {
    // Clean up test fixtures
    mock_database_engine_reset_all();
    usleep(10000);  // 10ms delay for thread cleanup
}

/*
 * Test: database_queue_process_single_query with NULL queue
 * Should return immediately without crashing
 */
void test_database_queue_process_single_query_null_queue(void) {
    database_queue_process_single_query(NULL);
    TEST_ASSERT_TRUE(true); // Should not crash
}

/*
 * Test: database_queue_process_single_query success path
 * Covers lines 85-138: Success path with statistics update and result cleanup
 */
void test_database_queue_process_single_query_success(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb_exec", "sqlite:///tmp/exec.db", QUEUE_TYPE_FAST, NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Set up a mock persistent connection to trigger the execution path
    DatabaseHandle* mock_connection = calloc(1, sizeof(DatabaseHandle));
    if (mock_connection) {
        mock_connection->designator = strdup("mock_connection");
        mock_connection->engine_type = DB_ENGINE_SQLITE;
    }
    queue->persistent_connection = mock_connection;

    // Configure mock to return success with a result
    mock_database_engine_set_execute_result(true);
    QueryResult* mock_result = calloc(1, sizeof(QueryResult));
    if (mock_result) {
        mock_result->row_count = 5;
        mock_result->execution_time_ms = 42;
        mock_database_engine_set_execute_query_result(mock_result);
    }

    // Create a query with all required fields
    DatabaseQuery* query = calloc(1, sizeof(DatabaseQuery));
    TEST_ASSERT_NOT_NULL(query);
    query->query_id = strdup("success_query");
    query->query_template = strdup("SELECT 1");
    query->parameter_json = strdup("{}");

    // Submit the query
    bool submit_result = database_queue_submit_query(queue, query);
    TEST_ASSERT_TRUE(submit_result);

    // Process the query - this will hit the SUCCESS path
    database_queue_process_single_query(queue);

    // Verify queue is empty after processing
    size_t depth = database_queue_get_depth(queue);
    TEST_ASSERT_EQUAL(0, depth);

    database_queue_destroy(queue);
}

/*
 * Test: database_queue_process_single_query without persistent connection
 * Covers the else branch when no persistent connection exists
 */
void test_database_queue_process_single_query_no_connection(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb_no_conn", "sqlite:///tmp/no_conn.db", QUEUE_TYPE_FAST, NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Don't set persistent_connection - this triggers the simulation path

    // Create a query
    DatabaseQuery* query = calloc(1, sizeof(DatabaseQuery));
    TEST_ASSERT_NOT_NULL(query);
    query->query_id = strdup("no_conn_query");
    query->query_template = strdup("SELECT 1 as test");
    query->parameter_json = strdup("{}");

    // Submit the query
    bool submit_result = database_queue_submit_query(queue, query);
    TEST_ASSERT_TRUE(submit_result);

    // Process the query - exercises the no-connection path
    database_queue_process_single_query(queue);

    // The test is successful if we reach this point without crashing
    TEST_PASS();

    database_queue_destroy(queue);
}

/*
 * Test: database_queue_process_single_query failure path
 * Covers lines 139-164: Failure path with error logging and statistics update
 */
void test_database_queue_process_single_query_failure(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb_fail", "sqlite:///tmp/fail.db", QUEUE_TYPE_FAST, NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Set up a mock persistent connection
    DatabaseHandle* mock_connection = calloc(1, sizeof(DatabaseHandle));
    if (mock_connection) {
        mock_connection->designator = strdup("mock_connection");
        mock_connection->engine_type = DB_ENGINE_SQLITE;
    }
    queue->persistent_connection = mock_connection;

    // Configure mock for failure
    mock_database_engine_set_execute_result(false);

    // Create another query for failure test
    DatabaseQuery* fail_query = calloc(1, sizeof(DatabaseQuery));
    TEST_ASSERT_NOT_NULL(fail_query);
    fail_query->query_id = strdup("fail_query");
    fail_query->query_template = strdup("SELECT invalid");
    fail_query->parameter_json = strdup("{}");

    // Submit the failing query
    bool submit_result = database_queue_submit_query(queue, fail_query);
    TEST_ASSERT_TRUE(submit_result);

    // Process the query - this will hit the FAILURE path
    database_queue_process_single_query(queue);

    // Verify queue is empty after processing
    size_t depth = database_queue_get_depth(queue);
    TEST_ASSERT_EQUAL(0, depth);

    database_queue_destroy(queue);
}

/*
 * Test: database_queue_worker_thread main loop shutdown check
 * We call the worker_thread function directly with shutdown_requested=true
 * so it exits the loop immediately after the initial heartbeat.
 */
void test_database_queue_worker_thread_main_loop_shutdown_check(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb_shutdown", "sqlite:///tmp/shutdown.db", QUEUE_TYPE_FAST, NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Set shutdown flag - the worker loop will exit immediately
    queue->shutdown_requested = true;

    // Call worker_thread directly (no real thread spawned due to mock_pthread_create)
    void* result = database_queue_worker_thread(queue);
    TEST_ASSERT_NULL(result);

    // Clean up - worker_thread_started was never set, so destroy is safe
    database_queue_destroy(queue);
}

/*
 * Test: database_queue_start_worker failure path (NULL parameter)
 * Covers the early return when db_queue is NULL
 */
void test_database_queue_start_worker_failure_path(void) {
    bool result = database_queue_start_worker(NULL);
    TEST_ASSERT_FALSE(result);
}

/*
 * Test: database_queue_manage_child_queues
 * Auto-scale was removed; the function is a no-op for Lead queues.
 * This test verifies it runs safely without spawning children.
 */
void test_database_queue_manage_child_queues_scaling_down(void) {
    DatabaseQueue* lead_queue = database_queue_create_lead("testdb_scale_down", "sqlite:///tmp/scale_down.db", NULL);
    TEST_ASSERT_NOT_NULL(lead_queue);
    TEST_ASSERT_TRUE(lead_queue->is_lead_queue);

    // manage_child_queues is a no-op (auto-scale removed), call it to verify safety
    database_queue_manage_child_queues(lead_queue);

    // Verify no children were spawned (function is a no-op)
    TEST_ASSERT_EQUAL(0, lead_queue->child_queue_count);

    // Clean up
    database_queue_destroy(lead_queue);
}

/*
 * Test: database_queue_worker_thread main loop processing
 * We call the worker_thread function directly. With shutdown_requested=true,
 * the loop exits after the initial heartbeat setup, exercising the same
 * shutdown-check code path as the worker loop.
 */
void test_database_queue_worker_thread_main_loop_processing(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb_main_loop", "sqlite:///tmp/main_loop.db", QUEUE_TYPE_FAST, NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Set shutdown flag - worker loop will exit immediately after heartbeat
    queue->shutdown_requested = true;

    // Call worker_thread directly
    void* result = database_queue_worker_thread(queue);
    TEST_ASSERT_NULL(result);

    // Clean up - worker_thread_started was never set
    database_queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_process_single_query_success);
    RUN_TEST(test_database_queue_process_single_query_no_connection);
    RUN_TEST(test_database_queue_process_single_query_failure);
    RUN_TEST(test_database_queue_worker_thread_main_loop_shutdown_check);
    RUN_TEST(test_database_queue_manage_child_queues_scaling_down);
    RUN_TEST(test_database_queue_start_worker_failure_path);
    RUN_TEST(test_database_queue_process_single_query_null_queue);
    RUN_TEST(test_database_queue_worker_thread_main_loop_processing);

    return UNITY_END();
}
