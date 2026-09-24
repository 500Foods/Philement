/*
 * Unity Test File: database_manage_test_create_and_start_queue
 * This file contains unit tests for database_create_and_start_queue
 * from src/database/database_manage.c
 *
 * database_create_and_start_queue is a thin wrapper around
 * database_queue_create_lead + database_queue_start_worker.
 *
 * NOTE: In the Unity test environment, USE_MOCK_PTHREAD is defined for
 * dbqueue source files, and USE_MOCK_SYSTEM is defined globally for Unity
 * tests. This means:
 *   - malloc/calloc/strdup are redirected to mocks (enabling fault injection)
 *   - pthread_create is redirected to mock_pthread_create (which does NOT
 *     actually spawn a real thread — it just sets a fake thread ID)
 *
 * Because of the mocked pthread_create, when start_worker succeeds it sets
 * worker_thread_started=true and a fake thread ID. Calling stop_worker
 * afterward would call pthread_timedjoin_np on the fake ID, causing a segfault.
 * The success-path test therefore resets worker_thread_started=false before
 * calling stop_worker/destroy, mimicking what would happen if the thread had
 * already exited.
 */

// Enable mocks BEFORE including source headers
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Include source headers after mocks
#include <src/hydrogen.h>
#include <unity.h>
#include <src/database/database.h>
#include <src/database/database_manage.h>

// Forward declarations for functions being tested
DatabaseQueue* database_create_and_start_queue(const char* name, const char* conn_str, const char* bootstrap_query);

// Test function prototypes
void test_database_create_and_start_queue_null_name(void);
void test_database_create_and_start_queue_null_conn_str(void);
void test_database_create_and_start_queue_null_name_and_conn_str(void);
void test_database_create_and_start_queue_empty_name(void);
void test_database_create_and_start_queue_create_lead_failure(void);
void test_database_create_and_start_queue_success(void);

void setUp(void) {
    // Initialize database subsystem for testing
    database_subsystem_init();

    // Reset all mocks to default state
    mock_system_reset_all();
}

void tearDown(void) {
    // Clean up test fixtures
    database_subsystem_shutdown();
    mock_system_reset_all();
}

// Test NULL name parameter - should return NULL immediately
void test_database_create_and_start_queue_null_name(void) {
    DatabaseQueue* result = database_create_and_start_queue(NULL, "sqlite::memory:", NULL);
    TEST_ASSERT_NULL(result);
}

// Test NULL conn_str parameter - should return NULL immediately
void test_database_create_and_start_queue_null_conn_str(void) {
    DatabaseQueue* result = database_create_and_start_queue("test", NULL, NULL);
    TEST_ASSERT_NULL(result);
}

// Test NULL name and NULL conn_str - should return NULL immediately
void test_database_create_and_start_queue_null_name_and_conn_str(void) {
    DatabaseQueue* result = database_create_and_start_queue(NULL, NULL, NULL);
    TEST_ASSERT_NULL(result);
}

// Test empty name - database_queue_create_lead rejects empty names
// via database_queue_validate_lead_params, so the wrapper returns NULL
// without ever starting a worker thread.
void test_database_create_and_start_queue_empty_name(void) {
    DatabaseQueue* result = database_create_and_start_queue("", "sqlite::memory:", NULL);
    TEST_ASSERT_NULL(result);
}

// Test malloc failure during queue creation - database_queue_create_lead
// fails at database_queue_allocate_basic (malloc returns NULL), so the
// wrapper logs the failure and returns NULL without starting a worker.
void test_database_create_and_start_queue_create_lead_failure(void) {
    mock_system_set_malloc_failure(1);
    DatabaseQueue* result = database_create_and_start_queue("test", "sqlite::memory:", NULL);
    TEST_ASSERT_NULL(result);
}

// Test full success path - both queue creation and worker start succeed.
// Since USE_MOCK_PTHREAD is defined for dbqueue source files, pthread_create
// is redirected to mock_pthread_create which does NOT spawn a real thread.
// start_worker sets worker_thread_started=true and a fake thread ID.
// We reset worker_thread_started=false before calling stop_worker/destroy
// to avoid pthread_timedjoin_np on the fake thread ID (which would segfault).
void test_database_create_and_start_queue_success(void) {
    DatabaseQueue* result = database_create_and_start_queue("test", "sqlite::memory:", NULL);
    TEST_ASSERT_NOT_NULL(result);

    if (result) {
        // start_worker succeeded (mocked pthread_create returns 0).
        // Reset worker_thread_started to false so stop_worker doesn't
        // try to join the fake thread ID (which would segfault).
        result->worker_thread_started = false;

        // Clean up - stop_worker (no-op now) and destroy the queue
        database_queue_stop_worker(result);
        database_queue_destroy(result);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_create_and_start_queue_null_name);
    RUN_TEST(test_database_create_and_start_queue_null_conn_str);
    RUN_TEST(test_database_create_and_start_queue_null_name_and_conn_str);
    RUN_TEST(test_database_create_and_start_queue_empty_name);
    RUN_TEST(test_database_create_and_start_queue_create_lead_failure);
    RUN_TEST(test_database_create_and_start_queue_success);

    return UNITY_END();
}
