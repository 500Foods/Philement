/*
 * Unity Test File: detect_potential_deadlock Function Tests
 * This file contains unit tests for the detect_potential_deadlock() function
 * from src/mutex/mutex.c
 *
 * detect_potential_deadlock scans the internal active_lock_attempts array
 * for entries whose subsystem matches the current_id, indicating that another
 * thread is waiting on a mutex in the same subsystem — a potential deadlock.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/mutex/mutex.h>
#include <pthread.h>
#include <unistd.h>

// Forward declaration for the function being tested
void detect_potential_deadlock(MutexId* current_id);

// ---------------------------------------------------------------------------
// Multi-threaded test infrastructure
// ---------------------------------------------------------------------------
// A worker thread that attempts to lock a mutex it cannot acquire. While
// blocked inside mutex_lock_with_timeout, its lock attempt is recorded in
// the static active_lock_attempts array, making it visible to
// detect_potential_deadlock running on the main thread.

struct deadlock_test_args {
    pthread_mutex_t* mutex;
    const char* subsystem;
    volatile int started;
};

static void* deadlock_worker_thread(void* arg) {
    struct deadlock_test_args* args = (struct deadlock_test_args*)arg;
    MutexId id = {
        "deadlock_worker",
        args->subsystem,
        "deadlock_worker_thread",
        __FILE__,
        __LINE__
    };
    args->started = 1;
    MutexResult result = mutex_lock_with_timeout(args->mutex, &id, 2000);
    if (result == MUTEX_SUCCESS) {
        mutex_unlock_with_id(args->mutex, &id);
    }
    return NULL;
}

// ---------------------------------------------------------------------------
// Test setup/teardown
// ---------------------------------------------------------------------------

void setUp(void) {
    // Start fresh: clean up any previous state, then initialise
    mutex_system_cleanup();
    mutex_system_init();
    mutex_reset_stats();
    mutex_enable_deadlock_detection(true);
}

void tearDown(void) {
    mutex_system_cleanup();
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void test_detect_potential_deadlock_null_id(void) {
    // NULL current_id triggers early return before touching any state
    detect_potential_deadlock(NULL);
    TEST_ASSERT_TRUE(true); // Should not crash

    // Stats should be unchanged
    MutexStats stats;
    mutex_get_stats(&stats);
    TEST_ASSERT_EQUAL(0ULL, stats.total_deadlocks_detected);
}

static void test_detect_potential_deadlock_no_entries(void) {
    // Non-NULL id with empty active_lock_attempts (clean state)
    MutexId current_id = {"test", "NOMATCH_SUBSYS", __func__, __FILE__, __LINE__};
    detect_potential_deadlock(&current_id);
    TEST_ASSERT_TRUE(true); // Should not crash

    MutexStats stats;
    mutex_get_stats(&stats);
    TEST_ASSERT_EQUAL(0ULL, stats.total_deadlocks_detected);
}

static void test_detect_potential_deadlock_no_matching_subsystem(void) {
    // Worker records an attempt with subsystem "OTHER_SUBSYS"
    // We call detect_potential_deadlock with subsystem "DTEST_SUBSYS"
    // No match should be found, deadlock count stays 0

    pthread_mutex_t test_mutex;
    pthread_mutex_init(&test_mutex, NULL);
    pthread_mutex_lock(&test_mutex); // Hold the lock so worker blocks

    struct deadlock_test_args args = {&test_mutex, "OTHER_SUBSYS", 0};

    pthread_t thread;
    pthread_create(&thread, NULL, deadlock_worker_thread, &args);

    // Wait for worker to start and record its lock attempt
    while (!args.started) {
        usleep(1000); // 1ms poll
    }
    // Give the worker time to enter mutex_lock_with_timeout and record the attempt
    usleep(50000); // 50ms

    MutexId current_id = {"detect", "DTEST_SUBSYS", __func__, __FILE__, __LINE__};
    detect_potential_deadlock(&current_id);

    // No match found — deadlock count should be 0
    MutexStats stats;
    mutex_get_stats(&stats);
    TEST_ASSERT_EQUAL(0ULL, stats.total_deadlocks_detected);

    // Release the lock so the worker can proceed and finish
    pthread_mutex_unlock(&test_mutex);
    pthread_join(thread, NULL);
    pthread_mutex_destroy(&test_mutex);
}

static void test_detect_potential_deadlock_found_matching(void) {
    // Worker records an attempt with subsystem "DTEST_SUBSYS"
    // We call detect_potential_deadlock with subsystem "DTEST_SUBSYS"
    // A match should be found, deadlock count should increment

    pthread_mutex_t test_mutex;
    pthread_mutex_init(&test_mutex, NULL);
    pthread_mutex_lock(&test_mutex); // Hold the lock so worker blocks

    struct deadlock_test_args args = {&test_mutex, "DTEST_SUBSYS", 0};

    pthread_t thread;
    pthread_create(&thread, NULL, deadlock_worker_thread, &args);

    // Wait for worker to start and record its lock attempt
    while (!args.started) {
        usleep(1000); // 1ms poll
    }
    // Give the worker time to enter mutex_lock_with_timeout and record the attempt
    usleep(50000); // 50ms

    MutexId current_id = {"detect", "DTEST_SUBSYS", __func__, __FILE__, __LINE__};
    detect_potential_deadlock(&current_id);

    // Match found — deadlock count should be > 0
    MutexStats stats;
    mutex_get_stats(&stats);
    TEST_ASSERT_GREATER_THAN(0ULL, stats.total_deadlocks_detected);
    // last_deadlock_time should be set (non-zero, since time(NULL) returns epoch time)
    TEST_ASSERT_NOT_EQUAL(0, stats.last_deadlock_time);

    // Release the lock so the worker can proceed and finish
    pthread_mutex_unlock(&test_mutex);
    pthread_join(thread, NULL);
    pthread_mutex_destroy(&test_mutex);
}

static void test_detect_potential_deadlock_stats_incremented_multiple_times(void) {
    // Call detect_potential_deadlock multiple times with a matching entry
    // present. Each call should increment the deadlock counter.

    pthread_mutex_t test_mutex;
    pthread_mutex_init(&test_mutex, NULL);
    pthread_mutex_lock(&test_mutex);

    struct deadlock_test_args args = {&test_mutex, "MULTI_SUBSYS", 0};

    pthread_t thread;
    pthread_create(&thread, NULL, deadlock_worker_thread, &args);

    while (!args.started) {
        usleep(1000);
    }
    usleep(50000);

    MutexId current_id = {"detect", "MULTI_SUBSYS", __func__, __FILE__, __LINE__};

    // First call should detect the deadlock
    detect_potential_deadlock(&current_id);
    MutexStats stats;
    mutex_get_stats(&stats);
    TEST_ASSERT_EQUAL(1ULL, stats.total_deadlocks_detected);

    // Second call should detect it again (worker still blocked)
    detect_potential_deadlock(&current_id);
    mutex_get_stats(&stats);
    TEST_ASSERT_EQUAL(2ULL, stats.total_deadlocks_detected);

    pthread_mutex_unlock(&test_mutex);
    pthread_join(thread, NULL);
    pthread_mutex_destroy(&test_mutex);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_detect_potential_deadlock_null_id);
    RUN_TEST(test_detect_potential_deadlock_no_entries);
    RUN_TEST(test_detect_potential_deadlock_no_matching_subsystem);
    RUN_TEST(test_detect_potential_deadlock_found_matching);
    RUN_TEST(test_detect_potential_deadlock_stats_incremented_multiple_times);

    return UNITY_END();
}
