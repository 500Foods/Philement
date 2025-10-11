/*
 * Unity Test File: check_timeout_expired Function Tests
 * This file contains unit tests for the check_timeout_expired() function
 * from src/database/postgresql/connection.c
 */

// Standard project header plus Unity Framework header
#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../../src/database/postgresql/connection.h"

// Forward declaration for the function being tested
bool check_timeout_expired(time_t start_time, int timeout_seconds);

// Function prototypes for test functions
void test_check_timeout_expired_not_expired(void);
void test_check_timeout_expired_exactly_expired(void);
void test_check_timeout_expired_well_expired(void);
void test_check_timeout_expired_zero_timeout(void);
void test_check_timeout_expired_negative_timeout(void);
void test_check_timeout_expired_future_start_time(void);
void test_check_timeout_expired_same_time(void);

// Test fixtures
void setUp(void) {
    // No setup needed for this simple function
}

void tearDown(void) {
    // No cleanup needed for this simple function
}

// Test functions
void test_check_timeout_expired_not_expired(void) {
    time_t now = time(NULL);
    time_t start_time = now - 10; // 10 seconds ago
    int timeout_seconds = 30; // 30 second timeout

    bool result = check_timeout_expired(start_time, timeout_seconds);
    TEST_ASSERT_FALSE(result); // Should not be expired
}

void test_check_timeout_expired_exactly_expired(void) {
    time_t now = time(NULL);
    time_t start_time = now - 30; // 30 seconds ago
    int timeout_seconds = 30; // 30 second timeout

    bool result = check_timeout_expired(start_time, timeout_seconds);
    TEST_ASSERT_TRUE(result); // Should be expired
}

void test_check_timeout_expired_well_expired(void) {
    time_t now = time(NULL);
    time_t start_time = now - 60; // 60 seconds ago
    int timeout_seconds = 30; // 30 second timeout

    bool result = check_timeout_expired(start_time, timeout_seconds);
    TEST_ASSERT_TRUE(result); // Should be expired
}

void test_check_timeout_expired_zero_timeout(void) {
    time_t now = time(NULL);
    time_t start_time = now - 10; // 10 seconds ago
    int timeout_seconds = 0; // 0 second timeout

    bool result = check_timeout_expired(start_time, timeout_seconds);
    TEST_ASSERT_TRUE(result); // Should be expired immediately
}

void test_check_timeout_expired_negative_timeout(void) {
    time_t now = time(NULL);
    time_t start_time = now - 10; // 10 seconds ago
    int timeout_seconds = -5; // Negative timeout

    bool result = check_timeout_expired(start_time, timeout_seconds);
    TEST_ASSERT_TRUE(result); // Should be expired (negative timeout means always expired)
}

void test_check_timeout_expired_future_start_time(void) {
    time_t now = time(NULL);
    time_t start_time = now + 10; // 10 seconds in the future
    int timeout_seconds = 30; // 30 second timeout

    bool result = check_timeout_expired(start_time, timeout_seconds);
    TEST_ASSERT_FALSE(result); // Should not be expired (start time is in future)
}

void test_check_timeout_expired_same_time(void) {
    time_t now = time(NULL);
    time_t start_time = now; // Same time
    int timeout_seconds = 30; // 30 second timeout

    bool result = check_timeout_expired(start_time, timeout_seconds);
    TEST_ASSERT_FALSE(result); // Should not be expired (just started)
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_timeout_expired_not_expired);
    RUN_TEST(test_check_timeout_expired_exactly_expired);
    RUN_TEST(test_check_timeout_expired_well_expired);
    RUN_TEST(test_check_timeout_expired_zero_timeout);
    RUN_TEST(test_check_timeout_expired_negative_timeout);
    RUN_TEST(test_check_timeout_expired_future_start_time);
    RUN_TEST(test_check_timeout_expired_same_time);

    return UNITY_END();
}