/*
 * Unity Test File: init_all_service_threads Function Tests
 * This file contains comprehensive unit tests for the init_all_service_threads() function
 * from src/utils/utils.c
 *
 * Coverage Goals:
 * - Verify that all 5 ServiceThreads global structures are properly initialized
 * - Verify thread_count is set to 0 for all structures
 * - Verify subsystem names match expected constants (SR_LOGGING, etc.)
 * - Verify memory fields are zeroed
 * - Verify thread ID and TID arrays are cleared
 * - Verify thread_descriptions are zeroed
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include the header for thread structures and globals
#include <src/threads/threads.h>

// Forward declaration for the function being tested (not in any header)
void init_all_service_threads(void);

// Unity framework requires these functions to be externally visible
extern void setUp(void);
extern void tearDown(void);

void setUp(void) {
    // Fill global ServiceThreads structures with garbage so we can verify
    // that init_all_service_threads properly resets them
    memset(&logging_threads, 0xFF, sizeof(ServiceThreads));
    memset(&webserver_threads, 0xFF, sizeof(ServiceThreads));
    memset(&websocket_threads, 0xFF, sizeof(ServiceThreads));
    memset(&mdns_server_threads, 0xFF, sizeof(ServiceThreads));
    memset(&print_threads, 0xFF, sizeof(ServiceThreads));
}

void tearDown(void) {
    // Reset to zero state after each test
    memset(&logging_threads, 0, sizeof(ServiceThreads));
    memset(&webserver_threads, 0, sizeof(ServiceThreads));
    memset(&websocket_threads, 0, sizeof(ServiceThreads));
    memset(&mdns_server_threads, 0, sizeof(ServiceThreads));
    memset(&print_threads, 0, sizeof(ServiceThreads));
}

// Function prototypes for test functions
void test_init_all_service_threads_thread_counts_zero(void);
void test_init_all_service_threads_subsystem_names(void);
void test_init_all_service_threads_memory_fields_zero(void);
void test_init_all_service_threads_thread_ids_zeroed(void);
void test_init_all_service_threads_thread_descriptions_zeroed(void);
void test_init_all_service_threads_thread_tids_zeroed(void);
void test_init_all_service_threads_thread_metrics_zeroed(void);
void test_init_all_service_threads_idempotent(void);

//=============================================================================
// Thread Count Tests
//=============================================================================

void test_init_all_service_threads_thread_counts_zero(void) {
    init_all_service_threads();

    TEST_ASSERT_EQUAL(0, logging_threads.thread_count);
    TEST_ASSERT_EQUAL(0, webserver_threads.thread_count);
    TEST_ASSERT_EQUAL(0, websocket_threads.thread_count);
    TEST_ASSERT_EQUAL(0, mdns_server_threads.thread_count);
    TEST_ASSERT_EQUAL(0, print_threads.thread_count);
}

//=============================================================================
// Subsystem Name Tests
//=============================================================================

void test_init_all_service_threads_subsystem_names(void) {
    init_all_service_threads();

    // Verify subsystem names match the SR_ constants used in the function
    TEST_ASSERT_EQUAL_STRING(SR_LOGGING, logging_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_WEBSERVER, webserver_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_WEBSOCKET, websocket_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_SERVER, mdns_server_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_PRINT, print_threads.subsystem);
}

//=============================================================================
// Memory Field Tests
//=============================================================================

void test_init_all_service_threads_memory_fields_zero(void) {
    init_all_service_threads();

    TEST_ASSERT_EQUAL(0, logging_threads.virtual_memory);
    TEST_ASSERT_EQUAL(0, logging_threads.resident_memory);
    TEST_ASSERT_EQUAL(0.0, logging_threads.memory_percent);

    TEST_ASSERT_EQUAL(0, webserver_threads.virtual_memory);
    TEST_ASSERT_EQUAL(0, webserver_threads.resident_memory);
    TEST_ASSERT_EQUAL(0.0, webserver_threads.memory_percent);

    TEST_ASSERT_EQUAL(0, websocket_threads.virtual_memory);
    TEST_ASSERT_EQUAL(0, websocket_threads.resident_memory);
    TEST_ASSERT_EQUAL(0.0, websocket_threads.memory_percent);

    TEST_ASSERT_EQUAL(0, mdns_server_threads.virtual_memory);
    TEST_ASSERT_EQUAL(0, mdns_server_threads.resident_memory);
    TEST_ASSERT_EQUAL(0.0, mdns_server_threads.memory_percent);

    TEST_ASSERT_EQUAL(0, print_threads.virtual_memory);
    TEST_ASSERT_EQUAL(0, print_threads.resident_memory);
    TEST_ASSERT_EQUAL(0.0, print_threads.memory_percent);
}

//=============================================================================
// Array Zeroing Tests
//=============================================================================

void test_init_all_service_threads_thread_ids_zeroed(void) {
    init_all_service_threads();

    for (int i = 0; i < MAX_SERVICE_THREADS; i++) {
        TEST_ASSERT_EQUAL(0, logging_threads.thread_ids[i]);
        TEST_ASSERT_EQUAL(0, webserver_threads.thread_ids[i]);
        TEST_ASSERT_EQUAL(0, websocket_threads.thread_ids[i]);
        TEST_ASSERT_EQUAL(0, mdns_server_threads.thread_ids[i]);
        TEST_ASSERT_EQUAL(0, print_threads.thread_ids[i]);
    }
}

void test_init_all_service_threads_thread_tids_zeroed(void) {
    init_all_service_threads();

    for (int i = 0; i < MAX_SERVICE_THREADS; i++) {
        TEST_ASSERT_EQUAL(0, logging_threads.thread_tids[i]);
        TEST_ASSERT_EQUAL(0, webserver_threads.thread_tids[i]);
        TEST_ASSERT_EQUAL(0, websocket_threads.thread_tids[i]);
        TEST_ASSERT_EQUAL(0, mdns_server_threads.thread_tids[i]);
        TEST_ASSERT_EQUAL(0, print_threads.thread_tids[i]);
    }
}

void test_init_all_service_threads_thread_descriptions_zeroed(void) {
    init_all_service_threads();

    // Verify first byte of each description string is zero
    for (int i = 0; i < MAX_SERVICE_THREADS; i++) {
        TEST_ASSERT_EQUAL('\0', logging_threads.thread_descriptions[i][0]);
        TEST_ASSERT_EQUAL('\0', webserver_threads.thread_descriptions[i][0]);
        TEST_ASSERT_EQUAL('\0', websocket_threads.thread_descriptions[i][0]);
        TEST_ASSERT_EQUAL('\0', mdns_server_threads.thread_descriptions[i][0]);
        TEST_ASSERT_EQUAL('\0', print_threads.thread_descriptions[i][0]);
    }
}

void test_init_all_service_threads_thread_metrics_zeroed(void) {
    init_all_service_threads();

    for (int i = 0; i < MAX_SERVICE_THREADS; i++) {
        TEST_ASSERT_EQUAL(0, logging_threads.thread_metrics[i].virtual_bytes);
        TEST_ASSERT_EQUAL(0, logging_threads.thread_metrics[i].resident_bytes);
        TEST_ASSERT_EQUAL(0, webserver_threads.thread_metrics[i].virtual_bytes);
        TEST_ASSERT_EQUAL(0, webserver_threads.thread_metrics[i].resident_bytes);
        TEST_ASSERT_EQUAL(0, websocket_threads.thread_metrics[i].virtual_bytes);
        TEST_ASSERT_EQUAL(0, websocket_threads.thread_metrics[i].resident_bytes);
    }
}

//=============================================================================
// Idempotency Tests
//=============================================================================

void test_init_all_service_threads_idempotent(void) {
    // Call init_all_service_threads multiple times
    init_all_service_threads();
    init_all_service_threads();
    init_all_service_threads();

    // Should still be in initialized state
    TEST_ASSERT_EQUAL(0, logging_threads.thread_count);
    TEST_ASSERT_EQUAL(0, webserver_threads.thread_count);
    TEST_ASSERT_EQUAL(0, websocket_threads.thread_count);
    TEST_ASSERT_EQUAL(0, mdns_server_threads.thread_count);
    TEST_ASSERT_EQUAL(0, print_threads.thread_count);

    TEST_ASSERT_EQUAL_STRING(SR_LOGGING, logging_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_WEBSERVER, webserver_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_WEBSOCKET, websocket_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_SERVER, mdns_server_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_PRINT, print_threads.subsystem);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Thread count tests
    RUN_TEST(test_init_all_service_threads_thread_counts_zero);

    // Subsystem name tests
    RUN_TEST(test_init_all_service_threads_subsystem_names);

    // Memory field tests
    RUN_TEST(test_init_all_service_threads_memory_fields_zero);

    // Array zeroing tests
    RUN_TEST(test_init_all_service_threads_thread_ids_zeroed);
    RUN_TEST(test_init_all_service_threads_thread_tids_zeroed);
    RUN_TEST(test_init_all_service_threads_thread_descriptions_zeroed);
    RUN_TEST(test_init_all_service_threads_thread_metrics_zeroed);

    // Idempotency test
    RUN_TEST(test_init_all_service_threads_idempotent);

    return UNITY_END();
}
