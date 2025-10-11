/*
 * Unity Test File: status_test_core_init_cleanup.c
 *
 * Tests for status_core_init and status_core_cleanup functions from status_core.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Function prototypes for test functions
void test_status_core_init_basic_functionality(void);
void test_status_core_cleanup_basic_functionality(void);
void test_status_core_init_multiple_calls(void);
void test_status_core_cleanup_multiple_calls(void);
void test_status_core_init_cleanup_sequence(void);
void test_status_core_cleanup_without_init(void);
void test_status_core_init_cleanup_init_sequence(void);

void setUp(void) {
    // No special setup needed for core init/cleanup tests
}

void tearDown(void) {
    // No special cleanup needed for core init/cleanup tests
}

// Tests for status_core_init function
void test_status_core_init_basic_functionality(void) {
    // Test that status_core_init can be called without crashing
    status_core_init();

    // The function should exist and be callable
    TEST_ASSERT_TRUE(status_core_init != NULL);

    // Should not crash
    TEST_PASS();
}

void test_status_core_init_multiple_calls(void) {
    // Test calling status_core_init multiple times
    status_core_init();
    status_core_init();
    status_core_init();

    // Should not crash with multiple calls
    TEST_PASS();
}

void test_status_core_init_cleanup_sequence(void) {
    // Test proper init -> cleanup sequence
    status_core_init();
    status_core_cleanup();

    // Should not crash
    TEST_PASS();
}

void test_status_core_init_cleanup_init_sequence(void) {
    // Test init -> cleanup -> init sequence
    status_core_init();
    status_core_cleanup();
    status_core_init();

    // Should not crash
    TEST_PASS();
}

// Tests for status_core_cleanup function
void test_status_core_cleanup_basic_functionality(void) {
    // Test that status_core_cleanup can be called without crashing
    status_core_cleanup();

    // The function should exist and be callable
    TEST_ASSERT_TRUE(status_core_cleanup != NULL);

    // Should not crash
    TEST_PASS();
}

void test_status_core_cleanup_multiple_calls(void) {
    // Test calling status_core_cleanup multiple times
    status_core_cleanup();
    status_core_cleanup();
    status_core_cleanup();

    // Should not crash with multiple calls
    TEST_PASS();
}

void test_status_core_cleanup_without_init(void) {
    // Test calling cleanup without prior init
    status_core_cleanup();

    // Should not crash even without initialization
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_status_core_init_basic_functionality);
    RUN_TEST(test_status_core_cleanup_basic_functionality);
    RUN_TEST(test_status_core_init_multiple_calls);
    RUN_TEST(test_status_core_cleanup_multiple_calls);
    RUN_TEST(test_status_core_init_cleanup_sequence);
    RUN_TEST(test_status_core_cleanup_without_init);
    RUN_TEST(test_status_core_init_cleanup_init_sequence);

    return UNITY_END();
}
