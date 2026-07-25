/*
 * Unity Test File: database_execute_test_get_query_age
 * This file contains unit tests for database_get_query_age function
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/database_pending.h>

// Forward declarations for functions being tested
time_t database_get_query_age(const char* query_id);

// Test function prototypes
void test_database_get_query_age_basic_functionality(void);
void test_database_get_query_age_null_query_id(void);
void test_database_get_query_age_empty_query_id(void);
void test_database_get_query_age_uninitialized_subsystem(void);
void test_database_get_query_age_pending_found(void);
void test_database_get_query_age_pending_future_submitted_at(void);
void test_database_get_query_age_pending_not_found_fallback(void);

void setUp(void) {
    database_subsystem_init();
}

void tearDown(void) {
    database_subsystem_shutdown();
}

// Test database_get_query_age function
void test_database_get_query_age_basic_functionality(void) {
    time_t result = database_get_query_age("query_123");
    TEST_ASSERT_EQUAL(0, result);
}

void test_database_get_query_age_null_query_id(void) {
    time_t result = database_get_query_age(NULL);
    TEST_ASSERT_EQUAL(0, result);
}

void test_database_get_query_age_empty_query_id(void) {
    time_t result = database_get_query_age("");
    TEST_ASSERT_EQUAL(0, result);
}

void test_database_get_query_age_uninitialized_subsystem(void) {
    database_subsystem_shutdown();
    time_t result = database_get_query_age("query_123");
    TEST_ASSERT_EQUAL(0, result);
}

// Test: pending result found with past submitted_at returns age
void test_database_get_query_age_pending_found(void) {
    PendingResultManager* manager = get_pending_result_manager();
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "age_query_123", 30, SR_DATABASE);
    TEST_ASSERT_NOT_NULL(pending);

    // Set submitted_at to 3 seconds ago
    pending->submitted_at = time(NULL) - 3;

    time_t result = database_get_query_age("age_query_123");
    TEST_ASSERT_GREATER_OR_EQUAL(3, result);
    TEST_ASSERT_LESS_THAN(10, result);
}

// Test: pending result with future submitted_at returns 0
void test_database_get_query_age_pending_future_submitted_at(void) {
    PendingResultManager* manager = get_pending_result_manager();
    TEST_ASSERT_NOT_NULL(manager);

    PendingQueryResult* pending = pending_result_register(manager, "future_query_123", 30, SR_DATABASE);
    TEST_ASSERT_NOT_NULL(pending);

    // Set submitted_at to 10 seconds in the future
    pending->submitted_at = time(NULL) + 10;

    time_t result = database_get_query_age("future_query_123");
    TEST_ASSERT_EQUAL(0, result);
}

// Test: pending not found falls back to find_max_query_age_across_queues
void test_database_get_query_age_pending_not_found_fallback(void) {
    time_t result = database_get_query_age("nonexistent_query");
    TEST_ASSERT_EQUAL(0, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_get_query_age_basic_functionality);
    RUN_TEST(test_database_get_query_age_null_query_id);
    RUN_TEST(test_database_get_query_age_empty_query_id);
    RUN_TEST(test_database_get_query_age_uninitialized_subsystem);
    RUN_TEST(test_database_get_query_age_pending_found);
    RUN_TEST(test_database_get_query_age_pending_future_submitted_at);
    RUN_TEST(test_database_get_query_age_pending_not_found_fallback);

    return UNITY_END();
}
