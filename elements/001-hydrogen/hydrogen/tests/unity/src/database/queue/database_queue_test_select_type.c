/*
 * Unity Test File: database_queue_select_type
 * This file contains unit tests for database_queue_select_type functionality
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/queue/database_queue.h>

// Forward declarations for functions being tested
DatabaseQueueType database_queue_select_type(const char* queue_path_hint);

// Function prototypes for test functions
void test_database_queue_select_type_slow(void);
void test_database_queue_select_type_medium(void);
void test_database_queue_select_type_fast(void);
void test_database_queue_select_type_cache(void);
void test_database_queue_select_type_null(void);
void test_database_queue_select_type_empty(void);
void test_database_queue_select_type_invalid(void);
void test_database_queue_select_type_case_insensitive(void);
void test_database_queue_select_type_partial_match(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_database_queue_select_type_slow(void) {
    // Test "slow" hint
    DatabaseQueueType result = database_queue_select_type("slow");
    TEST_ASSERT_EQUAL(DB_QUEUE_SLOW, result);
}

void test_database_queue_select_type_medium(void) {
    // Test "medium" hint
    DatabaseQueueType result = database_queue_select_type("medium");
    TEST_ASSERT_EQUAL(DB_QUEUE_MEDIUM, result);
}

void test_database_queue_select_type_fast(void) {
    // Test "fast" hint
    DatabaseQueueType result = database_queue_select_type("fast");
    TEST_ASSERT_EQUAL(DB_QUEUE_FAST, result);
}

void test_database_queue_select_type_cache(void) {
    // Test "cache" hint
    DatabaseQueueType result = database_queue_select_type("cache");
    TEST_ASSERT_EQUAL(DB_QUEUE_CACHE, result);
}

void test_database_queue_select_type_null(void) {
    // Test null hint
    DatabaseQueueType result = database_queue_select_type(NULL);
    TEST_ASSERT_EQUAL(DB_QUEUE_MEDIUM, result); // Default value
}

void test_database_queue_select_type_empty(void) {
    // Test empty string hint
    DatabaseQueueType result = database_queue_select_type("");
    TEST_ASSERT_EQUAL(DB_QUEUE_MEDIUM, result); // Default value
}

void test_database_queue_select_type_invalid(void) {
    // Test invalid hint
    DatabaseQueueType result = database_queue_select_type("invalid");
    TEST_ASSERT_EQUAL(DB_QUEUE_MEDIUM, result); // Default value
}

void test_database_queue_select_type_case_insensitive(void) {
    // Test case insensitivity
    DatabaseQueueType result = database_queue_select_type("SLOW");
    TEST_ASSERT_EQUAL(DB_QUEUE_MEDIUM, result); // Should not match due to case
}

void test_database_queue_select_type_partial_match(void) {
    // Test partial matches
    DatabaseQueueType result = database_queue_select_type("slow_query");
    TEST_ASSERT_EQUAL(DB_QUEUE_MEDIUM, result); // Should not match partial
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_select_type_slow);
    RUN_TEST(test_database_queue_select_type_medium);
    RUN_TEST(test_database_queue_select_type_fast);
    RUN_TEST(test_database_queue_select_type_cache);
    RUN_TEST(test_database_queue_select_type_null);
    RUN_TEST(test_database_queue_select_type_empty);
    RUN_TEST(test_database_queue_select_type_invalid);
    RUN_TEST(test_database_queue_select_type_case_insensitive);
    RUN_TEST(test_database_queue_select_type_partial_match);

    return UNITY_END();
}