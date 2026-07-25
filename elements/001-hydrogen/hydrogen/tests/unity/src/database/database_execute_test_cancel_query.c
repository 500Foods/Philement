/*
 * Unity Test File: database_execute_test_cancel_query
 * This file contains unit tests for database_cancel_query function
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
bool database_cancel_query(const char* query_id);

// Test function prototypes
void test_database_cancel_query_basic_functionality(void);
void test_database_cancel_query_null_query_id(void);
void test_database_cancel_query_empty_query_id(void);
void test_database_cancel_query_uninitialized_subsystem(void);
void test_database_cancel_query_no_manager(void);
void test_database_cancel_query_pending_found(void);

void setUp(void) {
    database_subsystem_init();
}

void tearDown(void) {
    database_subsystem_shutdown();
    mock_system_reset_all();
}

// Test database_cancel_query function
void test_database_cancel_query_basic_functionality(void) {
    bool result = database_cancel_query("query_123");
    TEST_ASSERT_FALSE(result);
}

void test_database_cancel_query_null_query_id(void) {
    bool result = database_cancel_query(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_database_cancel_query_empty_query_id(void) {
    bool result = database_cancel_query("");
    TEST_ASSERT_FALSE(result);
}

void test_database_cancel_query_uninitialized_subsystem(void) {
    database_subsystem_shutdown();
    bool result = database_cancel_query("query_123");
    TEST_ASSERT_FALSE(result);
}

// Test: get_pending_result_manager() returns NULL (malloc failure)
void test_database_cancel_query_no_manager(void) {
    mock_system_set_malloc_failure(1);
    bool result = database_cancel_query("query_123");
    TEST_ASSERT_FALSE(result);
}

// Test: pending result found and cancelled returns true
void test_database_cancel_query_pending_found(void) {
    PendingResultManager* manager = get_pending_result_manager();
    TEST_ASSERT_NOT_NULL(manager);

    pending_result_register(manager, "cancel_query_123", 30, SR_DATABASE);

    bool result = database_cancel_query("cancel_query_123");
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_cancel_query_basic_functionality);
    RUN_TEST(test_database_cancel_query_null_query_id);
    RUN_TEST(test_database_cancel_query_empty_query_id);
    RUN_TEST(test_database_cancel_query_uninitialized_subsystem);
    RUN_TEST(test_database_cancel_query_no_manager);
    RUN_TEST(test_database_cancel_query_pending_found);

    return UNITY_END();
}
