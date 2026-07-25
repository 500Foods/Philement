/*
 * Unity Test File: database_execute_test_cleanup_old_results
 * This file contains unit tests for database_cleanup_old_results function
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/database_pending.h>

// Include mock system header for malloc failure testing
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Forward declarations for functions being tested
size_t database_cleanup_old_results(time_t max_age_seconds);

// Test function prototypes
void test_database_cleanup_old_results_basic_functionality(void);
void test_database_cleanup_old_results_zero_age(void);
void test_database_cleanup_old_results_large_age(void);
void test_database_cleanup_old_results_uninitialized_subsystem(void);
void test_database_cleanup_old_results_no_manager(void);

void setUp(void) {
    database_subsystem_init();
}

void tearDown(void) {
    database_subsystem_shutdown();
    mock_system_reset_all();
}

// Test database_cleanup_old_results function
void test_database_cleanup_old_results_basic_functionality(void) {
    database_cleanup_old_results(3600);
    TEST_PASS();
}

void test_database_cleanup_old_results_zero_age(void) {
    database_cleanup_old_results(0);
    TEST_PASS();
}

void test_database_cleanup_old_results_large_age(void) {
    database_cleanup_old_results(31536000);
    TEST_PASS();
}

void test_database_cleanup_old_results_uninitialized_subsystem(void) {
    database_subsystem_shutdown();
    database_cleanup_old_results(3600);
    TEST_PASS();
}

// Test: get_pending_result_manager() returns NULL (malloc failure)
void test_database_cleanup_old_results_no_manager(void) {
    mock_system_set_malloc_failure(1);
    size_t result = database_cleanup_old_results(3600);
    TEST_ASSERT_EQUAL(0, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_cleanup_old_results_basic_functionality);
    RUN_TEST(test_database_cleanup_old_results_zero_age);
    RUN_TEST(test_database_cleanup_old_results_large_age);
    RUN_TEST(test_database_cleanup_old_results_uninitialized_subsystem);
    RUN_TEST(test_database_cleanup_old_results_no_manager);

    return UNITY_END();
}
