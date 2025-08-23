/*
 * Unity Test File: report_thread_status Function Tests
 * This file contains unit tests for the report_thread_status() function
 * from src/threads/threads.c
 *
 * Coverage Goals:
 * - Test thread status reporting functionality
 * - Test behavior with different thread configurations
 * - Test logging integration without crashes
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Forward declarations for the functions being tested
void init_service_threads(ServiceThreads *threads, const char* subsystem_name);
void add_service_thread(ServiceThreads *threads, pthread_t thread_id);
void report_thread_status(void);

// Global thread structures (extern declarations)
extern ServiceThreads logging_threads;
extern ServiceThreads webserver_threads;
extern ServiceThreads websocket_threads;
extern ServiceThreads mdns_server_threads;
extern ServiceThreads print_threads;

// Test fixtures
static ServiceThreads test_threads;

// Unity framework requires these functions to be externally visible
extern void setUp(void);
extern void tearDown(void);

void setUp(void) {
    // Initialize test fixtures and global thread structures
    memset(&test_threads, 0, sizeof(ServiceThreads));
    init_service_threads(&test_threads, "TestService");

    // Initialize global thread structures
    init_service_threads(&logging_threads, "Logging");
    init_service_threads(&webserver_threads, "WebServer");
    init_service_threads(&websocket_threads, "WebSocket");
    init_service_threads(&mdns_server_threads, "mDNS Server");
    init_service_threads(&print_threads, "Print");
}

void tearDown(void) {
    // Clean up test fixtures
}

// Function prototypes for test functions
void test_report_thread_status_empty_threads(void);
void test_report_thread_status_with_threads(void);
void test_report_thread_status_full_threads(void);
void test_report_thread_status_max_threads(void);
void test_report_thread_status_thread_counting(void);
void test_report_thread_status_service_totals(void);
void test_report_thread_status_subsystem_names(void);
void test_report_thread_status_logging_integration(void);
void test_report_thread_status_no_double_logging(void);
void test_report_thread_status_preserves_thread_arrays(void);

//=============================================================================
// Basic Status Reporting Tests
//=============================================================================

void test_report_thread_status_empty_threads(void) {
    // Test status reporting with all empty thread lists
    // All global structures are already initialized in setUp

    // This should not crash and should complete successfully
    report_thread_status();
    TEST_ASSERT_TRUE(true);
}

void test_report_thread_status_with_threads(void) {
    // Test status reporting with some threads added
    add_service_thread(&logging_threads, (pthread_t)1);
    add_service_thread(&logging_threads, (pthread_t)2);
    add_service_thread(&webserver_threads, (pthread_t)3);
    add_service_thread(&print_threads, (pthread_t)4);

    // This should not crash and should complete successfully
    report_thread_status();
    TEST_ASSERT_TRUE(true);
}

void test_report_thread_status_full_threads(void) {
    // Test status reporting with threads in all services
    add_service_thread(&logging_threads, (pthread_t)1);
    add_service_thread(&webserver_threads, (pthread_t)2);
    add_service_thread(&websocket_threads, (pthread_t)3);
    add_service_thread(&mdns_server_threads, (pthread_t)4);
    add_service_thread(&print_threads, (pthread_t)5);

    // This should not crash and should complete successfully
    report_thread_status();
    TEST_ASSERT_TRUE(true);
}

void test_report_thread_status_max_threads(void) {
    // Test status reporting with maximum threads in one service
    for (int i = 0; i < MAX_SERVICE_THREADS; i++) {
        add_service_thread(&logging_threads, (pthread_t)i);
    }

    // This should not crash and should complete successfully
    report_thread_status();
    TEST_ASSERT_TRUE(true);
}

//=============================================================================
// Thread Counting Tests
//=============================================================================

void test_report_thread_status_thread_counting(void) {
    // Test that thread counting works correctly
    add_service_thread(&logging_threads, (pthread_t)1);
    add_service_thread(&logging_threads, (pthread_t)2);
    add_service_thread(&webserver_threads, (pthread_t)3);
    add_service_thread(&websocket_threads, (pthread_t)4);
    add_service_thread(&mdns_server_threads, (pthread_t)5);
    add_service_thread(&print_threads, (pthread_t)6);

    // Verify counts before reporting
    TEST_ASSERT_EQUAL(2, logging_threads.thread_count);
    TEST_ASSERT_EQUAL(1, webserver_threads.thread_count);
    TEST_ASSERT_EQUAL(1, websocket_threads.thread_count);
    TEST_ASSERT_EQUAL(1, mdns_server_threads.thread_count);
    TEST_ASSERT_EQUAL(1, print_threads.thread_count);

    // Report status (this should not modify counts)
    report_thread_status();

    // Verify counts are unchanged
    TEST_ASSERT_EQUAL(2, logging_threads.thread_count);
    TEST_ASSERT_EQUAL(1, webserver_threads.thread_count);
    TEST_ASSERT_EQUAL(1, websocket_threads.thread_count);
    TEST_ASSERT_EQUAL(1, mdns_server_threads.thread_count);
    TEST_ASSERT_EQUAL(1, print_threads.thread_count);
}

void test_report_thread_status_service_totals(void) {
    // Test that service totals are calculated correctly
    add_service_thread(&logging_threads, (pthread_t)1);
    add_service_thread(&webserver_threads, (pthread_t)2);
    add_service_thread(&webserver_threads, (pthread_t)3);
    add_service_thread(&print_threads, (pthread_t)4);

    // Set some memory values
    logging_threads.virtual_memory = 1000;
    logging_threads.resident_memory = 2000;
    webserver_threads.virtual_memory = 3000;
    webserver_threads.resident_memory = 4000;
    print_threads.virtual_memory = 5000;
    print_threads.resident_memory = 6000;

    // Report status (this should not modify service totals)
    report_thread_status();

    // Verify service totals are unchanged
    TEST_ASSERT_EQUAL(1000, logging_threads.virtual_memory);
    TEST_ASSERT_EQUAL(2000, logging_threads.resident_memory);
    TEST_ASSERT_EQUAL(3000, webserver_threads.virtual_memory);
    TEST_ASSERT_EQUAL(4000, webserver_threads.resident_memory);
    TEST_ASSERT_EQUAL(5000, print_threads.virtual_memory);
    TEST_ASSERT_EQUAL(6000, print_threads.resident_memory);
}

//=============================================================================
// Subsystem Name Tests
//=============================================================================

void test_report_thread_status_subsystem_names(void) {
    // Test that subsystem names are preserved during reporting
    add_service_thread(&logging_threads, (pthread_t)1);
    add_service_thread(&webserver_threads, (pthread_t)2);

    // Verify subsystem names before reporting
    TEST_ASSERT_EQUAL_STRING("Logging", logging_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING("WebServer", webserver_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING("WebSocket", websocket_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING("mDNS Server", mdns_server_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING("Print", print_threads.subsystem);

    // Report status (this should not modify subsystem names)
    report_thread_status();

    // Verify subsystem names are unchanged
    TEST_ASSERT_EQUAL_STRING("Logging", logging_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING("WebServer", webserver_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING("WebSocket", websocket_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING("mDNS Server", mdns_server_threads.subsystem);
    TEST_ASSERT_EQUAL_STRING("Print", print_threads.subsystem);
}

//=============================================================================
// Logging Integration Tests
//=============================================================================

void test_report_thread_status_logging_integration(void) {
    // Test that the function integrates properly with the logging system
    // Add some threads to make the report more interesting
    add_service_thread(&logging_threads, (pthread_t)1);
    add_service_thread(&logging_threads, (pthread_t)2);
    add_service_thread(&logging_threads, (pthread_t)3);
    add_service_thread(&webserver_threads, (pthread_t)4);

    // This should use the logging system without crashing
    report_thread_status();
    TEST_ASSERT_TRUE(true);
}

void test_report_thread_status_no_double_logging(void) {
    // Test that the function doesn't cause issues with logging state
    // The function should complete without affecting global logging state

    // Add some threads
    add_service_thread(&logging_threads, (pthread_t)1);

    // Report status
    report_thread_status();

    // Should be able to report again without issues
    report_thread_status();
    TEST_ASSERT_TRUE(true);
}

//=============================================================================
// Thread Array Integrity Tests
//=============================================================================

void test_report_thread_status_preserves_thread_arrays(void) {
    // Test that reporting doesn't corrupt thread arrays
    add_service_thread(&logging_threads, (pthread_t)1);
    add_service_thread(&logging_threads, (pthread_t)2);
    add_service_thread(&webserver_threads, (pthread_t)3);

    // Store original thread IDs
    pthread_t logging_id_0 = logging_threads.thread_ids[0];
    pthread_t logging_id_1 = logging_threads.thread_ids[1];
    pthread_t webserver_id_0 = webserver_threads.thread_ids[0];

    // Report status
    report_thread_status();

    // Verify thread arrays are unchanged
    TEST_ASSERT_EQUAL(logging_id_0, logging_threads.thread_ids[0]);
    TEST_ASSERT_EQUAL(logging_id_1, logging_threads.thread_ids[1]);
    TEST_ASSERT_EQUAL(webserver_id_0, webserver_threads.thread_ids[0]);
    TEST_ASSERT_EQUAL(2, logging_threads.thread_count);
    TEST_ASSERT_EQUAL(1, webserver_threads.thread_count);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic status reporting tests
    RUN_TEST(test_report_thread_status_empty_threads);
    RUN_TEST(test_report_thread_status_with_threads);
    RUN_TEST(test_report_thread_status_full_threads);
    RUN_TEST(test_report_thread_status_max_threads);

    // Thread counting tests
    RUN_TEST(test_report_thread_status_thread_counting);
    RUN_TEST(test_report_thread_status_service_totals);

    // Subsystem name tests
    RUN_TEST(test_report_thread_status_subsystem_names);

    // Logging integration tests
    RUN_TEST(test_report_thread_status_logging_integration);
    RUN_TEST(test_report_thread_status_no_double_logging);

    // Thread array integrity tests
    RUN_TEST(test_report_thread_status_preserves_thread_arrays);

    return UNITY_END();
}
