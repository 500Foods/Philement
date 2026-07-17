/*
 * Unity Test File: Conduit Status - database readiness helper
 * Tests conduit_status_check_database_readiness() in
 * src/api/conduit/status/status.c
 *
 * CHANGELOG:
 * 2026-07-16: Initial implementation
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/status/status.h>

// Test function prototypes
void test_check_database_readiness_null(void);
void test_check_database_readiness_no_bootstrap(void);
void test_check_database_readiness_no_cache(void);
void test_check_database_readiness_empty_cache(void);
void test_check_database_readiness_ready(void);

void setUp(void) {
    // No global state to reset
}

void tearDown(void) {
    // No global state to clean
}

// NULL queue is never ready
void test_check_database_readiness_null(void) {
    TEST_ASSERT_FALSE(conduit_status_check_database_readiness(NULL));
}

// Queue exists but bootstrap not completed is not ready
void test_check_database_readiness_no_bootstrap(void) {
    DatabaseQueue db_queue = {0};
    db_queue.bootstrap_completed = false;
    db_queue.query_cache = NULL;
    TEST_ASSERT_FALSE(conduit_status_check_database_readiness(&db_queue));
}

// Queue exists, bootstrap done, but no query cache is not ready
void test_check_database_readiness_no_cache(void) {
    DatabaseQueue db_queue = {0};
    db_queue.bootstrap_completed = true;
    db_queue.query_cache = NULL;
    TEST_ASSERT_FALSE(conduit_status_check_database_readiness(&db_queue));
}

// Query cache present but empty (entry_count == 0) is not ready
void test_check_database_readiness_empty_cache(void) {
    DatabaseQueue db_queue = {0};
    db_queue.bootstrap_completed = true;
    QueryTableCache cache = {0};
    cache.entry_count = 0;
    db_queue.query_cache = &cache;
    TEST_ASSERT_FALSE(conduit_status_check_database_readiness(&db_queue));
}

// Bootstrap done and query cache populated -> ready
void test_check_database_readiness_ready(void) {
    DatabaseQueue db_queue = {0};
    db_queue.bootstrap_completed = true;
    QueryTableCache cache = {0};
    cache.entry_count = 3;
    db_queue.query_cache = &cache;
    TEST_ASSERT_TRUE(conduit_status_check_database_readiness(&db_queue));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_database_readiness_null);
    RUN_TEST(test_check_database_readiness_no_bootstrap);
    RUN_TEST(test_check_database_readiness_no_cache);
    RUN_TEST(test_check_database_readiness_empty_cache);
    RUN_TEST(test_check_database_readiness_ready);

    return UNITY_END();
}
