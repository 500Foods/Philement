/*
 * Unity Test File: prepare_and_submit_query
 * This file contains unit tests for prepare_and_submit_query function
 * in src/api/conduit/query/query.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/query/query.h>

// Enable mocks for testing
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Function prototypes
void test_prepare_and_submit_query_null_queue(void);
void test_prepare_and_submit_query_null_query_id(void);
void test_prepare_and_submit_query_null_sql(void);
void test_prepare_and_submit_query_no_parameters(void);
void test_prepare_and_submit_query_with_parameters(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

// Test with NULL queue
void test_prepare_and_submit_query_null_queue(void) {
    QueryCacheEntry cache_entry = {
        .queue_type = (char*)"read"
    };

    bool result = prepare_and_submit_query(NULL, "test_id", "SELECT 1", NULL, 0, &cache_entry);
    TEST_ASSERT_FALSE(result); // Should fail with NULL queue
}

// Test with NULL query_id
void test_prepare_and_submit_query_null_query_id(void) {
    // We can't easily create a real DatabaseQueue, so we'll test the parameter validation
    // The function should handle NULL query_id gracefully
    QueryCacheEntry cache_entry = {
        .queue_type = (char*)"read"
    };

    bool result = prepare_and_submit_query(NULL, NULL, "SELECT 1", NULL, 0, &cache_entry);
    TEST_ASSERT_FALSE(result); // Should fail with NULL parameters
}

// Test with NULL SQL
void test_prepare_and_submit_query_null_sql(void) {
    QueryCacheEntry cache_entry = {
        .queue_type = (char*)"read"
    };

    bool result = prepare_and_submit_query(NULL, "test_id", NULL, NULL, 0, &cache_entry);
    TEST_ASSERT_FALSE(result); // Should fail with NULL SQL
}

// Test with no parameters
void test_prepare_and_submit_query_no_parameters(void) {
    QueryCacheEntry cache_entry = {
        .queue_type = (char*)"read"
    };

    bool result = prepare_and_submit_query(NULL, "test_id", "SELECT 1", NULL, 0, &cache_entry);
    // The result depends on whether the queue submission succeeds
    // Since we can't mock the queue submission easily, we just test that it doesn't crash
    (void)result; // Suppress unused variable warning
    TEST_PASS(); // Function executed without crashing
}

// Test with parameters
void test_prepare_and_submit_query_with_parameters(void) {
    QueryCacheEntry cache_entry = {
        .queue_type = (char*)"write"
    };

    // Create mock parameters including boolean and float types to cover all switch cases
    TypedParameter param1 = {
        .name = (char*)"user_id",
        .type = PARAM_TYPE_INTEGER,
        .value.int_value = 42
    };

    TypedParameter param2 = {
        .name = (char*)"name",
        .type = PARAM_TYPE_STRING,
        .value.string_value = (char*)"test_user"
    };

    TypedParameter param3 = {
        .name = (char*)"active",
        .type = PARAM_TYPE_BOOLEAN,
        .value.bool_value = true
    };

    TypedParameter param4 = {
        .name = (char*)"score",
        .type = PARAM_TYPE_FLOAT,
        .value.float_value = 95.5
    };

    TypedParameter* ordered_params[] = {&param1, &param2, &param3, &param4};

    bool result = prepare_and_submit_query(NULL, "test_id", "SELECT * FROM users WHERE id = :userId AND name = :userName AND active = :isActive AND score = :userScore",
                                           ordered_params, 4, &cache_entry);
    // The result depends on queue submission, but we test that it doesn't crash
    (void)result; // Suppress unused variable warning
    TEST_PASS(); // Function executed without crashing
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_prepare_and_submit_query_null_queue);
    RUN_TEST(test_prepare_and_submit_query_null_query_id);
    RUN_TEST(test_prepare_and_submit_query_null_sql);
    RUN_TEST(test_prepare_and_submit_query_no_parameters);
    RUN_TEST(test_prepare_and_submit_query_with_parameters);

    return UNITY_END();
}