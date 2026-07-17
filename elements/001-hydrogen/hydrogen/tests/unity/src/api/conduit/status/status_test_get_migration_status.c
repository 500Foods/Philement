/*
 * Unity Test File: Conduit Status - migration status helper
 * Tests conduit_status_get_migration_status() in
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
void test_get_migration_status_null(void);
void test_get_migration_status_no_bootstrap(void);
void test_get_migration_status_completed(void);

void setUp(void) {
    // No global state to reset
}

void tearDown(void) {
    // No global state to clean
}

// NULL queue -> not_found
void test_get_migration_status_null(void) {
    TEST_ASSERT_EQUAL_STRING("not_found", conduit_status_get_migration_status(NULL));
}

// Queue exists but bootstrap not completed -> in_progress
void test_get_migration_status_no_bootstrap(void) {
    DatabaseQueue db_queue = {0};
    db_queue.bootstrap_completed = false;
    TEST_ASSERT_EQUAL_STRING("in_progress", conduit_status_get_migration_status(&db_queue));
}

// Bootstrap completed -> completed
void test_get_migration_status_completed(void) {
    DatabaseQueue db_queue = {0};
    db_queue.bootstrap_completed = true;
    TEST_ASSERT_EQUAL_STRING("completed", conduit_status_get_migration_status(&db_queue));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_migration_status_null);
    RUN_TEST(test_get_migration_status_no_bootstrap);
    RUN_TEST(test_get_migration_status_completed);

    return UNITY_END();
}
