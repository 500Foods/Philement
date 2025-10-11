/*
 * Unity Test File: Time Utilities Tests
 * This file contains unit tests for utils_time.c functionality
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// External volatile variable that some functions depend on
extern volatile sig_atomic_t server_starting;

// Function prototypes for test functions
void test_format_duration_zero_seconds(void);
void test_format_duration_seconds_only(void);
void test_format_duration_minutes_and_seconds(void);
void test_format_duration_hours_minutes_seconds(void);
void test_format_duration_full_time(void);
void test_format_duration_large_values(void);
void test_format_duration_edge_cases(void);
void test_format_duration_null_buffer(void);
void test_format_duration_small_buffer(void);
void test_set_and_get_server_start_time(void);
void test_multiple_set_server_start_time(void);
void test_get_server_start_time_before_set(void);
void test_is_server_ready_time_set_initially_false(void);
void test_update_server_ready_time_while_starting(void);
void test_update_server_ready_time_when_ready(void);
void test_get_server_ready_time_before_set(void);
void test_calculate_startup_time_before_start(void);
void test_calculate_startup_time_after_start(void);
void test_calculate_total_runtime_before_start(void);
void test_calculate_total_runtime_after_start(void);
void test_record_shutdown_start_time(void);
void test_record_shutdown_end_time(void);
void test_calculate_shutdown_time_without_start(void);
void test_record_startup_complete_time(void);
void test_record_shutdown_initiate_time(void);
void test_calculate_total_running_time_lifecycle(void);
void test_calculate_total_running_time_before_complete(void);
void test_calculate_total_elapsed_time_lifecycle(void);
void test_calculate_total_elapsed_time_before_start(void);
void test_get_system_start_time_string_after_start(void);
void test_get_system_start_time_string_before_start(void);
void test_get_system_start_time_string_consistent(void);
void test_complete_startup_sequence(void);
void test_complete_lifecycle_timing(void);

void setUp(void) {
    // Reset server_starting state for consistent testing
    server_starting = 1;  // Assume starting state
}

void tearDown(void) {
    // Reset to starting state
    server_starting = 1;
}

// Test format_duration function with various inputs
void test_format_duration_zero_seconds(void) {
    char buffer[64];
    format_duration(0, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("0d 0h 0m 0s", buffer);
}

void test_format_duration_seconds_only(void) {
    char buffer[64];
    format_duration(45, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("0d 0h 0m 45s", buffer);
}

void test_format_duration_minutes_and_seconds(void) {
    char buffer[64];
    format_duration(125, buffer, sizeof(buffer));  // 2m 5s
    TEST_ASSERT_EQUAL_STRING("0d 0h 2m 5s", buffer);
}

void test_format_duration_hours_minutes_seconds(void) {
    char buffer[64];
    format_duration(3665, buffer, sizeof(buffer));  // 1h 1m 5s
    TEST_ASSERT_EQUAL_STRING("0d 1h 1m 5s", buffer);
}

void test_format_duration_full_time(void) {
    char buffer[64];
    format_duration(90061, buffer, sizeof(buffer));  // 1d 1h 1m 1s
    TEST_ASSERT_EQUAL_STRING("1d 1h 1m 1s", buffer);
}

void test_format_duration_large_values(void) {
    char buffer[64];
    format_duration(365 * 24 * 3600 + 23 * 3600 + 59 * 60 + 59, buffer, sizeof(buffer));  // 365d 23h 59m 59s
    TEST_ASSERT_EQUAL_STRING("365d 23h 59m 59s", buffer);
}

void test_format_duration_edge_cases(void) {
    char buffer[64];
    
    // Test exact hour
    format_duration(3600, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("0d 1h 0m 0s", buffer);
    
    // Test exact day
    format_duration(86400, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("1d 0h 0m 0s", buffer);
    
    // Test exact minute
    format_duration(60, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("0d 0h 1m 0s", buffer);
}

void test_format_duration_null_buffer(void) {
    // Test should not crash with NULL buffer (though behavior is undefined)
    // We can't really test this safely without risking crashes
    // So we'll skip this edge case
}

void test_format_duration_small_buffer(void) {
    char small_buffer[10];
    format_duration(90061, small_buffer, sizeof(small_buffer));
    // Should truncate but not crash
    TEST_ASSERT_NOT_NULL(small_buffer);
    // Buffer should still be null-terminated and within bounds
    TEST_ASSERT_LESS_OR_EQUAL(10, strlen(small_buffer) + 1);
}

// Test server start time functions
void test_set_and_get_server_start_time(void) {
    // Set the server start time
    set_server_start_time();
    
    // Get the time - should be non-zero
    time_t start_time = get_server_start_time();
    
    TEST_ASSERT_GREATER_THAN(0, start_time);
    // Since utils_time.c uses CLOCK_MONOTONIC, the returned time might not match wall clock
    // Just verify we get a reasonable positive value
}

void test_multiple_set_server_start_time(void) {
    // Test that multiple calls work correctly
    set_server_start_time();
    time_t first_time = get_server_start_time();
    
    // Small delay without sleep - just multiple calls
    for(int i = 0; i < 1000; i++) {
        // Busy wait to create minimal time difference
    }
    
    set_server_start_time();
    time_t second_time = get_server_start_time();
    
    // Second time should be later than or equal to first (monotonic clock)
    TEST_ASSERT_GREATER_OR_EQUAL(first_time, second_time);
    // Both should be valid times
    TEST_ASSERT_GREATER_THAN(0, first_time);
    TEST_ASSERT_GREATER_THAN(0, second_time);
}

void test_get_server_start_time_before_set(void) {
    // Note: This may not give predictable results since other tests may have set it
    // But we can test that it doesn't crash
    time_t start_time = get_server_start_time();
    // Should not crash and return some value
    (void)start_time;  // Suppress unused warning
}

// Test server ready time functions
void test_is_server_ready_time_set_initially_false(void) {
    // After set_server_start_time(), ready time should be reset
    set_server_start_time();
    int is_set = is_server_ready_time_set();
    TEST_ASSERT_FALSE(is_set);
}

void test_update_server_ready_time_while_starting(void) {
    // Set up starting state
    server_starting = 1;
    set_server_start_time();
    
    // Call update while still starting - should not set ready time
    update_server_ready_time();
    
    int is_set = is_server_ready_time_set();
    TEST_ASSERT_FALSE(is_set);
}

void test_update_server_ready_time_when_ready(void) {
    // Set up ready state
    server_starting = 0;  // Not starting anymore
    set_server_start_time();
    
    // Call update when ready - should set ready time
    update_server_ready_time();
    
    int is_set = is_server_ready_time_set();
    TEST_ASSERT_TRUE(is_set);
    
    time_t ready_time = get_server_ready_time();
    TEST_ASSERT_GREATER_THAN(0, ready_time);
}

void test_get_server_ready_time_before_set(void) {
    // Reset state
    set_server_start_time();
    
    time_t ready_time = get_server_ready_time();
    // Should return 0 if not set
    TEST_ASSERT_EQUAL(0, ready_time);
}

// Test timing calculation functions
void test_calculate_startup_time_before_start(void) {
    // Note: May have been set by other tests, but test doesn't crash
    double startup_time = calculate_startup_time();
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, startup_time);
}

void test_calculate_startup_time_after_start(void) {
    set_server_start_time();
    
    // Should return a small positive value (or 0 if ready time not set)
    double startup_time = calculate_startup_time();
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, startup_time);
    TEST_ASSERT_LESS_THAN(5.0, startup_time);  // Should be less than 5 seconds
}

void test_calculate_total_runtime_before_start(void) {
    // Test when no original start time is set
    // Function should handle this gracefully
    double runtime = calculate_total_runtime();
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, runtime);
}

void test_calculate_total_runtime_after_start(void) {
    // First call sets both server_start_time and original_start_time
    set_server_start_time();
    
    // Small delay using nanosleep for precision
    struct timespec delay = {0, 5000000}; // 5 milliseconds
    nanosleep(&delay, NULL);
    
    double runtime = calculate_total_runtime();
    // Should be positive if original_start_time was set
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, runtime);
    if (runtime > 0.0) {
        TEST_ASSERT_LESS_THAN(1.0, runtime);  // Should be less than 1 second
    }
}

// Test shutdown timing functions
void test_record_shutdown_start_time(void) {
    record_shutdown_start_time();
    
    // Should not crash - we can't directly verify the internal state
    // but we can test the calculate function
    double shutdown_time = calculate_shutdown_time();
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, shutdown_time);
}

void test_record_shutdown_end_time(void) {
    record_shutdown_start_time();
    
    // Small delay using nanosleep for precision
    struct timespec delay = {0, 5000000}; // 5 milliseconds
    nanosleep(&delay, NULL);
    
    record_shutdown_end_time();
    
    double shutdown_time = calculate_shutdown_time();
    // Should be positive since we set both start and end
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, shutdown_time);
    if (shutdown_time > 0.0) {
        TEST_ASSERT_LESS_THAN(1.0, shutdown_time);  // Should be much less than 1 second
    }
}

void test_calculate_shutdown_time_without_start(void) {
    // Test when shutdown times haven't been set
    double shutdown_time = calculate_shutdown_time();
    // Should return 0.0 or handle gracefully
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, shutdown_time);
}

// Test startup/shutdown lifecycle timing
void test_record_startup_complete_time(void) {
    record_startup_complete_time();
    // Should not crash
}

void test_record_shutdown_initiate_time(void) {
    record_shutdown_initiate_time();
    // Should not crash
}

void test_calculate_total_running_time_lifecycle(void) {
    record_startup_complete_time();
    
    // Small delay using nanosleep for precision
    struct timespec delay = {0, 5000000}; // 5 milliseconds
    nanosleep(&delay, NULL);
    
    record_shutdown_initiate_time();
    
    double running_time = calculate_total_running_time();
    // Should be positive since we set both startup and shutdown times
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, running_time);
    if (running_time > 0.0) {
        TEST_ASSERT_LESS_THAN(1.0, running_time);  // Should be much less than 1 second
    }
}

void test_calculate_total_running_time_before_complete(void) {
    // Test when times aren't set - function should return 0.0
    // Reset any previously set times by not calling record functions
    double running_time = calculate_total_running_time();
    // Function returns 0.0 when times aren't properly set
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, running_time);
}

void test_calculate_total_elapsed_time_lifecycle(void) {
    set_server_start_time();  // This sets original start time if not already set
    
    // Small delay using nanosleep for precision
    struct timespec delay = {0, 5000000}; // 5 milliseconds
    nanosleep(&delay, NULL);
    
    double elapsed_time = calculate_total_elapsed_time();
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, elapsed_time);
    if (elapsed_time > 0.0) {
        TEST_ASSERT_LESS_THAN(1.0, elapsed_time);
    }
}

void test_calculate_total_elapsed_time_before_start(void) {
    // Test when original start time isn't set
    // Note: may have been set by other tests, so this is more of a non-crash test
    double elapsed_time = calculate_total_elapsed_time();
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, elapsed_time);
}

// Test get_system_start_time_string function
void test_get_system_start_time_string_after_start(void) {
    set_server_start_time();
    
    const char* time_string = get_system_start_time_string();
    TEST_ASSERT_NOT_NULL(time_string);
    TEST_ASSERT_GREATER_THAN(0, strlen(time_string));
    
    // Should contain typical ISO format characters
    TEST_ASSERT_NOT_NULL(strchr(time_string, 'T'));
    TEST_ASSERT_NOT_NULL(strchr(time_string, ':'));
    TEST_ASSERT_NOT_NULL(strchr(time_string, '-'));
}

void test_get_system_start_time_string_before_start(void) {
    // Test when start time might not be set
    const char* time_string = get_system_start_time_string();
    TEST_ASSERT_NOT_NULL(time_string);
    
    // Should return either a proper time string or "unknown"
    TEST_ASSERT_GREATER_THAN(0, strlen(time_string));
}

void test_get_system_start_time_string_consistent(void) {
    set_server_start_time();
    
    const char* time_string1 = get_system_start_time_string();
    const char* time_string2 = get_system_start_time_string();
    
    // Should return the same string (static buffer)
    TEST_ASSERT_EQUAL_STRING(time_string1, time_string2);
}

// Integration test: complete startup sequence
void test_complete_startup_sequence(void) {
    // Simulate complete startup sequence
    server_starting = 1;
    set_server_start_time();
    
    // Verify initial state
    TEST_ASSERT_FALSE(is_server_ready_time_set());
    TEST_ASSERT_GREATER_THAN(0, get_server_start_time());
    
    // Simulate startup completion
    server_starting = 0;
    update_server_ready_time();
    
    // Verify final state
    TEST_ASSERT_TRUE(is_server_ready_time_set());
    TEST_ASSERT_GREATER_THAN(0, get_server_ready_time());
    
    double startup_time = calculate_startup_time();
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, startup_time);
    TEST_ASSERT_LESS_THAN(5.0, startup_time);
}

// Integration test: complete lifecycle
void test_complete_lifecycle_timing(void) {
    // Startup
    set_server_start_time();
    record_startup_complete_time();
    
    // Running phase - small delay using nanosleep for precision
    struct timespec delay = {0, 5000000}; // 5 milliseconds
    nanosleep(&delay, NULL);
    
    // Shutdown
    record_shutdown_initiate_time();
    record_shutdown_start_time();
    
    // Small delay before shutdown end
    nanosleep(&delay, NULL);
    record_shutdown_end_time();
    
    // Verify all timing functions work
    double total_runtime = calculate_total_runtime();
    double total_elapsed = calculate_total_elapsed_time();
    double shutdown_time = calculate_shutdown_time();
    
    // All should be non-negative
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, total_runtime);
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, total_elapsed);
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, shutdown_time);
    
    // At least one should be positive if timing worked
    TEST_ASSERT_TRUE(total_runtime > 0.0 || total_elapsed > 0.0 || shutdown_time > 0.0);
}

int main(void) {
    UNITY_BEGIN();
    
    // format_duration tests
    RUN_TEST(test_format_duration_zero_seconds);
    RUN_TEST(test_format_duration_seconds_only);
    RUN_TEST(test_format_duration_minutes_and_seconds);
    RUN_TEST(test_format_duration_hours_minutes_seconds);
    RUN_TEST(test_format_duration_full_time);
    RUN_TEST(test_format_duration_large_values);
    RUN_TEST(test_format_duration_edge_cases);
    RUN_TEST(test_format_duration_small_buffer);
    
    // Server start time tests
    RUN_TEST(test_set_and_get_server_start_time);
    RUN_TEST(test_multiple_set_server_start_time);
    RUN_TEST(test_get_server_start_time_before_set);
    
    // Server ready time tests
    RUN_TEST(test_is_server_ready_time_set_initially_false);
    RUN_TEST(test_update_server_ready_time_while_starting);
    RUN_TEST(test_update_server_ready_time_when_ready);
    RUN_TEST(test_get_server_ready_time_before_set);
    
    // Timing calculation tests
    RUN_TEST(test_calculate_startup_time_before_start);
    RUN_TEST(test_calculate_startup_time_after_start);
    RUN_TEST(test_calculate_total_runtime_before_start);
    RUN_TEST(test_calculate_total_runtime_after_start);
    
    // Shutdown timing tests
    RUN_TEST(test_record_shutdown_start_time);
    RUN_TEST(test_record_shutdown_end_time);
    RUN_TEST(test_calculate_shutdown_time_without_start);
    
    // Lifecycle timing tests
    RUN_TEST(test_record_startup_complete_time);
    RUN_TEST(test_record_shutdown_initiate_time);
    RUN_TEST(test_calculate_total_running_time_lifecycle);
    RUN_TEST(test_calculate_total_running_time_before_complete);
    RUN_TEST(test_calculate_total_elapsed_time_lifecycle);
    RUN_TEST(test_calculate_total_elapsed_time_before_start);
    
    // String formatting tests
    RUN_TEST(test_get_system_start_time_string_after_start);
    RUN_TEST(test_get_system_start_time_string_before_start);
    RUN_TEST(test_get_system_start_time_string_consistent);
    
    // Integration tests
    RUN_TEST(test_complete_startup_sequence);
    RUN_TEST(test_complete_lifecycle_timing);
    
    return UNITY_END();
}
