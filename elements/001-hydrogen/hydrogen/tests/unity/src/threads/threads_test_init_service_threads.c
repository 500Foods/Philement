/*
 * Unity Test File: init_service_threads Function Tests
 * This file contains unit tests for the init_service_threads() function
 * from src/threads/threads.c
 *
 * Coverage Goals:
 * - Test thread initialization with various subsystem names
 * - Test parameter validation and edge cases
 * - Test proper structure initialization
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Forward declarations for the functions being tested
void init_service_threads(ServiceThreads *threads, const char* subsystem_name);

// Test fixtures
static ServiceThreads test_threads;

// Unity framework requires these functions to be externally visible
extern void setUp(void);
extern void tearDown(void);

void setUp(void) {
    // Initialize test fixtures
    memset(&test_threads, 0, sizeof(ServiceThreads));
}

void tearDown(void) {
    // Clean up test fixtures
}

// Function prototypes for test functions
void test_init_service_threads_null_threads(void);
void test_init_service_threads_valid_initialization(void);
void test_init_service_threads_null_subsystem_name(void);
void test_init_service_threads_long_subsystem_name(void);
void test_init_service_threads_empty_subsystem_name(void);
void test_init_service_threads_various_subsystem_names(void);
void test_init_service_threads_structure_reset(void);
void test_init_service_threads_subsystem_bounds_checking(void);

//=============================================================================
// Basic Initialization Tests
//=============================================================================

void test_init_service_threads_valid_initialization(void) {
    // Test valid initialization with a subsystem name
    init_service_threads(&test_threads, "TestService");

    TEST_ASSERT_EQUAL(0, test_threads.thread_count);
    TEST_ASSERT_EQUAL_STRING("TestService", test_threads.subsystem);
    TEST_ASSERT_EQUAL(0, test_threads.virtual_memory);
    TEST_ASSERT_EQUAL(0, test_threads.resident_memory);
    TEST_ASSERT_EQUAL(0.0, test_threads.memory_percent);
}

void test_init_service_threads_null_subsystem_name(void) {
    // Test initialization with NULL subsystem name
    init_service_threads(&test_threads, NULL);

    TEST_ASSERT_EQUAL(0, test_threads.thread_count);
    TEST_ASSERT_EQUAL_STRING("Unknown", test_threads.subsystem);
}

void test_init_service_threads_empty_subsystem_name(void) {
    // Test initialization with empty subsystem name
    init_service_threads(&test_threads, "");

    TEST_ASSERT_EQUAL(0, test_threads.thread_count);
    TEST_ASSERT_EQUAL_STRING("", test_threads.subsystem);
}

void test_init_service_threads_long_subsystem_name(void) {
    // Test initialization with very long subsystem name
    char long_name[100];
    memset(long_name, 'A', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';

    init_service_threads(&test_threads, long_name);

    // Should be truncated to 31 characters + null terminator
    TEST_ASSERT_EQUAL(0, test_threads.thread_count);
    TEST_ASSERT_EQUAL(31, strlen(test_threads.subsystem));  // 31 chars + null = 32 total
    TEST_ASSERT_EQUAL('\0', test_threads.subsystem[31]);
}

void test_init_service_threads_various_subsystem_names(void) {
    // Test various subsystem name scenarios
    const char* test_names[] = {
        "Logging",
        "WebServer",
        "WebSocket",
        "mDNS Server",
        "Print",
        "Test Service With Spaces",
        "Test-Service-With-Dashes",
        "Test_Service_With_Underscores"
    };

    for (size_t i = 0; i < sizeof(test_names) / sizeof(test_names[0]); i++) {
        init_service_threads(&test_threads, test_names[i]);
        TEST_ASSERT_EQUAL(0, test_threads.thread_count);
        TEST_ASSERT_EQUAL_STRING(test_names[i], test_threads.subsystem);
    }
}

void test_init_service_threads_structure_reset(void) {
    // Test that initialization properly resets all structure fields
    // First, fill with garbage values
    test_threads.thread_count = 999;
    test_threads.virtual_memory = 999999;
    test_threads.resident_memory = 888888;
    test_threads.memory_percent = 123.45;
    memset(test_threads.subsystem, 'X', sizeof(test_threads.subsystem));
    memset(test_threads.thread_ids, 0xFF, sizeof(test_threads.thread_ids));
    memset(test_threads.thread_tids, 0xFF, sizeof(test_threads.thread_tids));
    memset(test_threads.thread_metrics, 0xFF, sizeof(test_threads.thread_metrics));

    init_service_threads(&test_threads, "TestService");

    // Verify what the function actually resets (based on source code)
    TEST_ASSERT_EQUAL(0, test_threads.thread_count);
    TEST_ASSERT_EQUAL_STRING("TestService", test_threads.subsystem);

    // Note: virtual_memory and resident_memory are NOT reset by init_service_threads
    // They are only reset by update_service_thread_metrics()
    // So we don't test for their reset here

    // Verify arrays are zeroed (what the function actually does)
    // Note: We need to be careful about the TID array - it might contain syscall results
    for (int i = 0; i < MAX_SERVICE_THREADS; i++) {
        TEST_ASSERT_EQUAL(0, test_threads.thread_ids[i]);
        // TID should be 0 after memset - if it's not, that's a problem
        TEST_ASSERT_EQUAL(0, test_threads.thread_tids[i]);
        TEST_ASSERT_EQUAL(0, test_threads.thread_metrics[i].virtual_bytes);
        TEST_ASSERT_EQUAL(0, test_threads.thread_metrics[i].resident_bytes);
    }
}

void test_init_service_threads_subsystem_bounds_checking(void) {
    // Test that subsystem name is properly bounded
    char exactly_31_chars[32]; // 31 chars + null terminator
    memset(exactly_31_chars, 'B', 31);
    exactly_31_chars[31] = '\0';

    init_service_threads(&test_threads, exactly_31_chars);

    TEST_ASSERT_EQUAL(31, strlen(test_threads.subsystem));
    TEST_ASSERT_EQUAL_STRING(exactly_31_chars, test_threads.subsystem);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic initialization tests
    RUN_TEST(test_init_service_threads_valid_initialization);
    RUN_TEST(test_init_service_threads_null_subsystem_name);
    RUN_TEST(test_init_service_threads_empty_subsystem_name);
    RUN_TEST(test_init_service_threads_long_subsystem_name);
    RUN_TEST(test_init_service_threads_various_subsystem_names);
    RUN_TEST(test_init_service_threads_structure_reset);
    RUN_TEST(test_init_service_threads_subsystem_bounds_checking);

    return UNITY_END();
}
