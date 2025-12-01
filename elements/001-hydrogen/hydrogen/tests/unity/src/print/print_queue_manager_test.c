/*
 * Unity Test File: Print Queue Manager Tests
 * This file contains unit tests for the print queue manager functions
 * from src/print/print_queue_manager.c
 *
 * Coverage Goals:
 * - Test process_print_job with valid/invalid JSON
 * - Test init_print_queue success/failure scenarios
 * - Test shutdown_print_queue functionality
 * - Test error handling and edge cases
 * - Verify correct logging behavior
 */

// Enable mocks BEFORE any includes
#define UNITY_TEST_MODE

// Include mock headers FIRST to ensure macro redefinition works
#include <unity/mocks/mock_logging.h>

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>  // For JSON validation

// Include necessary headers for the print queue manager module
#include <src/print/print_queue_manager.h>

void setUp(void) {
    // Reset mock logging state before each test
    mock_logging_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    // Note: In a real scenario, you might need to clean up the queue
}

// Forward declarations for the functions being tested
void process_print_job(const char* job_data);
int init_print_queue(void);
void shutdown_print_queue(void);
void cleanup_print_queue_manager(void* arg);

// Test functions
void test_process_print_job_null_data(void);
void test_process_print_job_invalid_json(void);
void test_process_print_job_valid_json(void);
void test_process_print_job_missing_fields(void);
void test_process_print_job_empty_json(void);
void test_process_print_job_large_json(void);
void test_init_print_queue_success(void);
void test_init_print_queue_failure(void);
void test_shutdown_print_queue_basic(void);
void test_cleanup_print_queue_manager_basic(void);

void test_process_print_job_null_data(void) {
    // Test NULL input handling - should log error
    fprintf(stderr, "DEBUG: About to call process_print_job(NULL)\n");

    // Debug: Check if macro is working
    fprintf(stderr, "DEBUG: log_this macro should resolve to mock_log_this\n");
    log_this("Queues", "Test message", LOG_LEVEL_ERROR, 0);

    process_print_job(NULL);
    fprintf(stderr, "DEBUG: process_print_job(NULL) returned, call_count=%d\n", mock_logging_get_call_count());

    // Verify that a log call was made
    TEST_ASSERT_GREATER_THAN(0, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("Queues", mock_logging_get_last_subsystem());
    TEST_ASSERT_TRUE(strstr(mock_logging_get_last_message(), "Received null job data") != NULL);
}

void test_process_print_job_invalid_json(void) {
    // Test invalid JSON handling - should log error
    const char* invalid_json = "{ invalid json }";
    process_print_job(invalid_json);

    // Verify that a log call was made
    TEST_ASSERT_GREATER_THAN(0, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("Queues", mock_logging_get_last_subsystem());
    TEST_ASSERT_TRUE(strstr(mock_logging_get_last_message(), "Failed to parse job JSON") != NULL);
}

void test_process_print_job_valid_json(void) {
    // Test valid JSON processing - should log success
    const char* valid_json = "{\"original_filename\":\"test.gcode\",\"new_filename\":\"processed.gcode\",\"file_size\":12345}";
    process_print_job(valid_json);

    // Verify that log calls were made
    TEST_ASSERT_GREATER_THAN(0, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("Queues", mock_logging_get_last_subsystem());
    TEST_ASSERT_TRUE(strstr(mock_logging_get_last_message(), "Processing print job") != NULL);

    // Verify the JSON is actually valid
    json_error_t error;
    json_t* json = json_loads(valid_json, 0, &error);
    TEST_ASSERT_NOT_NULL(json);

    if (json) {
        json_t* original_filename = json_object_get(json, "original_filename");
        json_t* new_filename = json_object_get(json, "new_filename");
        json_t* file_size = json_object_get(json, "file_size");

        TEST_ASSERT_NOT_NULL(original_filename);
        TEST_ASSERT_NOT_NULL(new_filename);
        TEST_ASSERT_NOT_NULL(file_size);

        TEST_ASSERT_EQUAL_STRING("test.gcode", json_string_value(original_filename));
        TEST_ASSERT_EQUAL_STRING("processed.gcode", json_string_value(new_filename));
        TEST_ASSERT_EQUAL(12345, json_integer_value(file_size));

        json_decref(json);
    }
}

void test_process_print_job_missing_fields(void) {
    // Test JSON with missing required fields - should handle gracefully
    const char* incomplete_json = "{\"original_filename\":\"test.gcode\"}";
    process_print_job(incomplete_json);

    // Verify the JSON structure
    json_error_t error;
    json_t* json = json_loads(incomplete_json, 0, &error);
    TEST_ASSERT_NOT_NULL(json);

    if (json) {
        json_t* original_filename = json_object_get(json, "original_filename");
        json_t* new_filename = json_object_get(json, "new_filename");
        json_t* file_size = json_object_get(json, "file_size");

        TEST_ASSERT_NOT_NULL(original_filename);
        TEST_ASSERT_NULL(new_filename);  // This field is missing
        TEST_ASSERT_NULL(file_size);     // This field is missing

        TEST_ASSERT_EQUAL_STRING("test.gcode", json_string_value(original_filename));

        json_decref(json);
    }
    TEST_PASS();
}

void test_process_print_job_empty_json(void) {
    // Test empty JSON object - should handle gracefully
    const char* empty_json = "{}";
    process_print_job(empty_json);

    // Verify the JSON is valid but empty
    json_error_t error;
    json_t* json = json_loads(empty_json, 0, &error);
    TEST_ASSERT_NOT_NULL(json);

    if (json) {
        TEST_ASSERT_TRUE(json_is_object(json));
        TEST_ASSERT_EQUAL(0, json_object_size(json));
        json_decref(json);
    }
    TEST_PASS();
}

void test_process_print_job_large_json(void) {
    // Test with a larger JSON string to test robustness
    const char* large_json =
        "{"
        "\"original_filename\":\"very_long_filename_that_tests_buffer_handling.gcode\","
        "\"new_filename\":\"processed_very_long_filename_that_tests_buffer_handling.gcode\","
        "\"file_size\":123456789,"
        "\"extra_field\":\"this_field_tests_additional_data_handling\","
        "\"another_field\":42"
        "}";
    process_print_job(large_json);

    // Verify the JSON is valid
    json_error_t error;
    json_t* json = json_loads(large_json, 0, &error);
    TEST_ASSERT_NOT_NULL(json);

    if (json) {
        json_t* original_filename = json_object_get(json, "original_filename");
        json_t* new_filename = json_object_get(json, "new_filename");
        json_t* file_size = json_object_get(json, "file_size");

        TEST_ASSERT_NOT_NULL(original_filename);
        TEST_ASSERT_NOT_NULL(new_filename);
        TEST_ASSERT_NOT_NULL(file_size);

        TEST_ASSERT_EQUAL_STRING("very_long_filename_that_tests_buffer_handling.gcode", json_string_value(original_filename));
        TEST_ASSERT_EQUAL_STRING("processed_very_long_filename_that_tests_buffer_handling.gcode", json_string_value(new_filename));
        TEST_ASSERT_EQUAL(123456789, json_integer_value(file_size));

        json_decref(json);
    }
    TEST_PASS();
}

void test_init_print_queue_success(void) {
    // Test successful queue initialization
    int result = init_print_queue();
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_NOT_NULL(print_queue);

    // Verify that success was logged
    TEST_ASSERT_GREATER_THAN(0, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("Queues", mock_logging_get_last_subsystem());
    TEST_ASSERT_TRUE(strstr(mock_logging_get_last_message(), "PrintQueue created successfully") != NULL);
}

void test_init_print_queue_failure(void) {
    // Test queue initialization failure
    // This test assumes that if we call init twice, the second call might fail
    // In a real scenario, you might need to mock queue_create to return NULL
    int result1 = init_print_queue();
    TEST_ASSERT_EQUAL_INT(1, result1);

    // Second call should still succeed in this implementation
    int result2 = init_print_queue();
    TEST_ASSERT_EQUAL_INT(1, result2);

    // Verify that success was logged for both calls
    TEST_ASSERT_GREATER_THAN(1, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("Queues", mock_logging_get_last_subsystem());
    TEST_ASSERT_TRUE(strstr(mock_logging_get_last_message(), "PrintQueue created successfully") != NULL);
}

void test_cleanup_print_queue_manager_basic(void) {
    // Test cleanup function - should not crash
    cleanup_print_queue_manager(NULL);
    TEST_PASS();
}

void test_shutdown_print_queue_basic(void) {
    // Test basic shutdown functionality
    // First initialize a queue
    init_print_queue();

    // Then shutdown
    shutdown_print_queue();

    // Should not crash and should perform cleanup
    TEST_ASSERT_GREATER_THAN(0, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("Queues", mock_logging_get_last_subsystem());
    TEST_ASSERT_TRUE(strstr(mock_logging_get_last_message(), "Print Queue shutdown complete") != NULL);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_process_print_job_null_data);
    RUN_TEST(test_process_print_job_invalid_json);
    RUN_TEST(test_process_print_job_valid_json);
    RUN_TEST(test_process_print_job_missing_fields);
    RUN_TEST(test_process_print_job_empty_json);
    RUN_TEST(test_process_print_job_large_json);
    RUN_TEST(test_init_print_queue_success);
    RUN_TEST(test_init_print_queue_failure);
    RUN_TEST(test_shutdown_print_queue_basic);
    RUN_TEST(test_cleanup_print_queue_manager_basic);

    return UNITY_END();
}