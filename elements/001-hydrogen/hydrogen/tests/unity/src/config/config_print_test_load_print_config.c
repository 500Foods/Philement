/*
 * Unity Test File: Print Configuration Tests
 * This file contains unit tests for the load_print_config function
 * from src/config/config_print.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config_print.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
bool load_print_config(json_t* root, AppConfig* config);
void cleanup_print_config(PrintConfig* config);
void dump_print_config(const PrintConfig* config);

// Forward declarations for test functions
void test_load_print_config_null_root(void);
void test_load_print_config_empty_json(void);
void test_load_print_config_basic_fields(void);
void test_load_print_config_priorities(void);
void test_load_print_config_timeouts(void);
void test_load_print_config_buffers(void);
void test_load_print_config_motion(void);
void test_cleanup_print_config_null_pointer(void);
void test_cleanup_print_config_empty_config(void);
void test_dump_print_config_null_pointer(void);
void test_dump_print_config_basic(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_print_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_print_config(NULL, &config);

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.print.enabled);  // Default is enabled
    TEST_ASSERT_EQUAL(100, config.print.max_queued_jobs);  // Default value
    TEST_ASSERT_EQUAL(4, config.print.max_concurrent_jobs);  // Default value
    TEST_ASSERT_EQUAL(50, config.print.priorities.default_priority);  // Default priority

    cleanup_print_config(&config.print);
}

void test_load_print_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_print_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.print.enabled);  // Default is enabled
    TEST_ASSERT_EQUAL(100, config.print.max_queued_jobs);  // Default value
    TEST_ASSERT_EQUAL(4, config.print.max_concurrent_jobs);  // Default value
    TEST_ASSERT_EQUAL(50, config.print.priorities.default_priority);  // Default priority
    TEST_ASSERT_EQUAL(5000, config.print.timeouts.shutdown_wait_ms);  // Default timeout
    TEST_ASSERT_EQUAL_FLOAT(100.0, config.print.motion.max_speed);  // Default speed

    json_decref(root);
    cleanup_print_config(&config.print);
}

// ===== BASIC FIELD TESTS =====

void test_load_print_config_basic_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* print_section = json_object();

    // Set up basic print configuration
    json_object_set(print_section, "Enabled", json_false());
    json_object_set(print_section, "MaxQueuedJobs", json_integer(200));
    json_object_set(print_section, "MaxConcurrentJobs", json_integer(8));

    json_object_set(root, "Print", print_section);

    bool result = load_print_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.print.enabled);
    TEST_ASSERT_EQUAL(200, config.print.max_queued_jobs);
    TEST_ASSERT_EQUAL(8, config.print.max_concurrent_jobs);

    json_decref(root);
    cleanup_print_config(&config.print);
}

// ===== PRIORITIES TESTS =====

void test_load_print_config_priorities(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* print_section = json_object();
    json_t* priorities_section = json_object();

    // Set up priorities configuration
    json_object_set(priorities_section, "DefaultPriority", json_integer(40));
    json_object_set(priorities_section, "EmergencyPriority", json_integer(90));
    json_object_set(priorities_section, "MaintenancePriority", json_integer(70));
    json_object_set(priorities_section, "SystemPriority", json_integer(85));

    json_object_set(print_section, "Priorities", priorities_section);
    json_object_set(root, "Print", print_section);

    bool result = load_print_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(40, config.print.priorities.default_priority);
    TEST_ASSERT_EQUAL(90, config.print.priorities.emergency_priority);
    TEST_ASSERT_EQUAL(70, config.print.priorities.maintenance_priority);
    TEST_ASSERT_EQUAL(85, config.print.priorities.system_priority);

    json_decref(root);
    cleanup_print_config(&config.print);
}

// ===== TIMEOUTS TESTS =====

void test_load_print_config_timeouts(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* print_section = json_object();
    json_t* timeouts_section = json_object();

    // Set up timeouts configuration
    json_object_set(timeouts_section, "ShutdownWaitMs", json_integer(10000));
    json_object_set(timeouts_section, "JobProcessingTimeoutMs", json_integer(600000));

    json_object_set(print_section, "Timeouts", timeouts_section);
    json_object_set(root, "Print", print_section);

    bool result = load_print_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(10000, config.print.timeouts.shutdown_wait_ms);
    TEST_ASSERT_EQUAL(600000, config.print.timeouts.job_processing_timeout_ms);

    json_decref(root);
    cleanup_print_config(&config.print);
}

// ===== BUFFERS TESTS =====

void test_load_print_config_buffers(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* print_section = json_object();
    json_t* buffers_section = json_object();

    // Set up buffers configuration
    json_object_set(buffers_section, "JobMessageSize", json_integer(8192));
    json_object_set(buffers_section, "StatusMessageSize", json_integer(2048));

    json_object_set(print_section, "Buffers", buffers_section);
    json_object_set(root, "Print", print_section);

    bool result = load_print_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(8192, config.print.buffers.job_message_size);
    TEST_ASSERT_EQUAL(2048, config.print.buffers.status_message_size);

    json_decref(root);
    cleanup_print_config(&config.print);
}

// ===== MOTION TESTS =====

void test_load_print_config_motion(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* print_section = json_object();
    json_t* motion_section = json_object();

    // Set up motion configuration
    json_object_set(motion_section, "MaxSpeed", json_real(120.5));
    json_object_set(motion_section, "MaxSpeedXY", json_real(110.0));
    json_object_set(motion_section, "MaxSpeedZ", json_real(25.0));
    json_object_set(motion_section, "MaxSpeedTravel", json_real(160.0));
    json_object_set(motion_section, "Acceleration", json_real(600.0));
    json_object_set(motion_section, "ZAcceleration", json_real(120.0));
    json_object_set(motion_section, "EAcceleration", json_real(300.0));
    json_object_set(motion_section, "Jerk", json_real(12.0));
    json_object_set(motion_section, "SmoothMoves", json_false());

    json_object_set(print_section, "Motion", motion_section);
    json_object_set(root, "Print", print_section);

    bool result = load_print_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_FLOAT(120.5, config.print.motion.max_speed);
    TEST_ASSERT_EQUAL_FLOAT(110.0, config.print.motion.max_speed_xy);
    TEST_ASSERT_EQUAL_FLOAT(25.0, config.print.motion.max_speed_z);
    TEST_ASSERT_EQUAL_FLOAT(160.0, config.print.motion.max_speed_travel);
    TEST_ASSERT_EQUAL_FLOAT(600.0, config.print.motion.acceleration);
    TEST_ASSERT_EQUAL_FLOAT(120.0, config.print.motion.z_acceleration);
    TEST_ASSERT_EQUAL_FLOAT(300.0, config.print.motion.e_acceleration);
    TEST_ASSERT_EQUAL_FLOAT(12.0, config.print.motion.jerk);
    TEST_ASSERT_FALSE(config.print.motion.smooth_moves);

    json_decref(root);
    cleanup_print_config(&config.print);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_print_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_print_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_print_config_empty_config(void) {
    PrintConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_print_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enabled);
    TEST_ASSERT_EQUAL(0, config.max_queued_jobs);
    TEST_ASSERT_EQUAL(0, config.max_concurrent_jobs);
    TEST_ASSERT_EQUAL(0, config.priorities.default_priority);
    TEST_ASSERT_EQUAL_FLOAT(0.0, config.motion.max_speed);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_print_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_print_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_print_config_basic(void) {
    PrintConfig config = {0};

    // Initialize with test data
    config.enabled = true;
    config.max_queued_jobs = 100;
    config.max_concurrent_jobs = 4;
    config.priorities.default_priority = 50;
    config.priorities.emergency_priority = 100;
    config.timeouts.shutdown_wait_ms = 5000;
    config.timeouts.job_processing_timeout_ms = 300000;
    config.buffers.job_message_size = 4096;
    config.buffers.status_message_size = 1024;
    config.motion.max_speed = 100.0;
    config.motion.max_speed_xy = 100.0;
    config.motion.max_speed_z = 20.0;
    config.motion.acceleration = 500.0;
    config.motion.jerk = 10.0;
    config.motion.smooth_moves = true;

    // Dump should not crash and handle the data properly
    dump_print_config(&config);

    // Clean up
    cleanup_print_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_print_config_null_root);
    RUN_TEST(test_load_print_config_empty_json);

    // Basic field tests
    RUN_TEST(test_load_print_config_basic_fields);
    RUN_TEST(test_load_print_config_priorities);
    RUN_TEST(test_load_print_config_timeouts);
    RUN_TEST(test_load_print_config_buffers);
    RUN_TEST(test_load_print_config_motion);

    // Cleanup function tests
    RUN_TEST(test_cleanup_print_config_null_pointer);
    RUN_TEST(test_cleanup_print_config_empty_config);

    // Dump function tests
    RUN_TEST(test_dump_print_config_null_pointer);
    RUN_TEST(test_dump_print_config_basic);

    return UNITY_END();
}