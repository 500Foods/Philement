// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/database/database_pending.h>
#include <src/database/database_types.h>
#include <string.h>

// Include source header
#include <src/api/conduit/query/query.h>

// No mocks needed - use real implementations

// Helper to clean up a specific pending result
static void cleanup_specific_pending(const char* query_id) {
    PendingResultManager* mgr = get_pending_result_manager();
    if (!mgr || !query_id) return;

    pthread_mutex_lock(&mgr->manager_lock);

    for (size_t i = 0; i < mgr->count; i++) {
        PendingQueryResult* pending = mgr->results[i];
        if (pending && strcmp(pending->query_id, query_id) == 0) {
            // Clean up pending
            free(pending->query_id);
            if (pending->result) {
                free(pending->result);
            }
            pthread_mutex_destroy(&pending->result_lock);
            pthread_cond_destroy(&pending->result_ready);
            free(pending);

            // Shift array
            for (size_t j = i; j < mgr->count - 1; j++) {
                mgr->results[j] = mgr->results[j + 1];
            }
            mgr->count--;
            break;
        }
    }

    pthread_mutex_unlock(&mgr->manager_lock);
}

void setUp(void);
void tearDown(void);

void setUp(void) {
    // Initialize global manager if needed
    (void)get_pending_result_manager();
}

void tearDown(void) {
    // Clean up any expired pendings
    pending_result_cleanup_expired(get_pending_result_manager());
}

// Test with NULL pending
static void test_wait_for_query_result_null_pending(void) {
    PendingQueryResult* pending = NULL;

    const QueryResult* result = wait_for_query_result(pending);

    TEST_ASSERT_NULL(result);
}

// Test with wait_result != 0 (failure) - use timeout=0 to simulate immediate timeout
static void test_wait_for_query_result_wait_failure(void) {
    char* query_id = strdup("test_wait_failure");
    TEST_ASSERT_NOT_NULL(query_id);

    PendingQueryResult* pending = pending_result_register(get_pending_result_manager(), query_id, 0);
    TEST_ASSERT_NOT_NULL(pending);

    const QueryResult* result = wait_for_query_result(pending);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_TRUE(pending_result_is_timed_out(pending));

    cleanup_specific_pending(query_id);
    free(query_id);
}

// Test with wait_result == 0 and valid get result
static void test_wait_for_query_result_success(void) {
    char* query_id = strdup("test_success");
    TEST_ASSERT_NOT_NULL(query_id);

    PendingQueryResult* pending = pending_result_register(get_pending_result_manager(), query_id, 30);
    TEST_ASSERT_NOT_NULL(pending);

    // Create dummy result
    QueryResult* dummy_result = calloc(1, sizeof(QueryResult));
    TEST_ASSERT_NOT_NULL(dummy_result);
    dummy_result->success = true;
    dummy_result->row_count = 1;

    // Signal ready
    bool signaled = pending_result_signal_ready(get_pending_result_manager(), query_id, dummy_result);
    TEST_ASSERT_TRUE(signaled);

    const QueryResult* result = wait_for_query_result(pending);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(dummy_result, result);
    TEST_ASSERT_TRUE(pending_result_is_completed(pending));

    cleanup_specific_pending(query_id);
    free(query_id);
}

// Test with wait_result == 0 but get returns NULL
static void test_wait_for_query_result_get_null(void) {
    char* query_id = strdup("test_get_null");
    TEST_ASSERT_NOT_NULL(query_id);

    PendingQueryResult* pending = pending_result_register(get_pending_result_manager(), query_id, 30);
    TEST_ASSERT_NOT_NULL(pending);

    // Signal ready with NULL result
    bool signaled = pending_result_signal_ready(get_pending_result_manager(), query_id, NULL);
    TEST_ASSERT_TRUE(signaled);

    const QueryResult* result = wait_for_query_result(pending);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_TRUE(pending_result_is_completed(pending));

    cleanup_specific_pending(query_id);
    free(query_id);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_wait_for_query_result_null_pending);
    RUN_TEST(test_wait_for_query_result_wait_failure);
    RUN_TEST(test_wait_for_query_result_success);
    RUN_TEST(test_wait_for_query_result_get_null);

    return UNITY_END();
}