/*
 * Unity Test File: Resources Configuration Tests
 * This file contains unit tests for the load_resources_config function
 * from src/config/config_resources.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config_resources.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
bool load_resources_config(json_t* root, AppConfig* config);
void cleanup_resources_config(ResourceConfig* config);
void dump_resources_config(const ResourceConfig* config);

// Forward declarations for test functions
void test_load_resources_config_null_root(void);
void test_load_resources_config_empty_json(void);
void test_load_resources_config_memory_limits(void);
void test_load_resources_config_queue_settings(void);
void test_load_resources_config_thread_limits(void);
void test_load_resources_config_file_limits(void);
void test_load_resources_config_monitoring(void);
void test_cleanup_resources_config_null_pointer(void);
void test_cleanup_resources_config_empty_config(void);
void test_dump_resources_config_null_pointer(void);
void test_dump_resources_config_basic(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_resources_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_resources_config(NULL, &config);

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1024, config.resources.max_memory_mb);  // Default value
    TEST_ASSERT_EQUAL(1048576, config.resources.max_buffer_size);  // Default value
    TEST_ASSERT_EQUAL(4, config.resources.min_threads);  // Default value

    cleanup_resources_config(&config.resources);
}

void test_load_resources_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_resources_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1024, config.resources.max_memory_mb);  // Default value
    TEST_ASSERT_EQUAL(1048576, config.resources.max_buffer_size);  // Default value
    TEST_ASSERT_EQUAL(65536, config.resources.post_processor_buffer_size);  // Default value
    TEST_ASSERT_EQUAL(4, config.resources.min_threads);  // Default value
    TEST_ASSERT_EQUAL(32, config.resources.max_threads);  // Default value
    TEST_ASSERT_TRUE(config.resources.enforce_limits);  // Default is true

    json_decref(root);
    cleanup_resources_config(&config.resources);
}

// ===== MEMORY LIMITS TESTS =====

void test_load_resources_config_memory_limits(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* resources_section = json_object();
    json_t* memory_section = json_object();

    // Set up memory limits configuration
    json_object_set(memory_section, "MaxMemoryMB", json_integer(2048));
    json_object_set(memory_section, "MaxBufferSize", json_integer(2097152));
    json_object_set(memory_section, "MinBufferSize", json_integer(8192));

    json_object_set(resources_section, "Memory", memory_section);
    json_object_set(root, "Resources", resources_section);

    bool result = load_resources_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2048, config.resources.max_memory_mb);
    TEST_ASSERT_EQUAL(2097152, config.resources.max_buffer_size);
    TEST_ASSERT_EQUAL(8192, config.resources.min_buffer_size);

    json_decref(root);
    cleanup_resources_config(&config.resources);
}

// ===== QUEUE SETTINGS TESTS =====

void test_load_resources_config_queue_settings(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* resources_section = json_object();
    json_t* queues_section = json_object();

    // Set up queue settings configuration
    json_object_set(queues_section, "MaxQueueSize", json_integer(20000));
    json_object_set(queues_section, "MaxQueueMemoryMB", json_integer(200));
    json_object_set(queues_section, "MaxQueueBlocks", json_integer(2000));
    json_object_set(queues_section, "QueueTimeoutMS", json_integer(10000));

    json_object_set(resources_section, "Queues", queues_section);
    json_object_set(root, "Resources", resources_section);

    bool result = load_resources_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(20000, config.resources.max_queue_size);
    TEST_ASSERT_EQUAL(200, config.resources.max_queue_memory_mb);
    TEST_ASSERT_EQUAL(2000, config.resources.max_queue_blocks);
    TEST_ASSERT_EQUAL(10000, config.resources.queue_timeout_ms);

    json_decref(root);
    cleanup_resources_config(&config.resources);
}

// ===== THREAD LIMITS TESTS =====

void test_load_resources_config_thread_limits(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* resources_section = json_object();
    json_t* threads_section = json_object();

    // Set up thread limits configuration
    json_object_set(threads_section, "MinThreads", json_integer(8));
    json_object_set(threads_section, "MaxThreads", json_integer(64));
    json_object_set(threads_section, "ThreadStackSize", json_integer(131072));

    json_object_set(resources_section, "Threads", threads_section);
    json_object_set(root, "Resources", resources_section);

    bool result = load_resources_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(8, config.resources.min_threads);
    TEST_ASSERT_EQUAL(64, config.resources.max_threads);
    TEST_ASSERT_EQUAL(131072, config.resources.thread_stack_size);

    json_decref(root);
    cleanup_resources_config(&config.resources);
}

// ===== FILE LIMITS TESTS =====

void test_load_resources_config_file_limits(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* resources_section = json_object();
    json_t* files_section = json_object();

    // Set up file limits configuration
    json_object_set(files_section, "MaxOpenFiles", json_integer(2048));
    json_object_set(files_section, "MaxFileSizeMB", json_integer(2048));
    json_object_set(files_section, "MaxLogSizeMB", json_integer(200));

    json_object_set(resources_section, "Files", files_section);
    json_object_set(root, "Resources", resources_section);

    bool result = load_resources_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2048, config.resources.max_open_files);
    TEST_ASSERT_EQUAL(2048, config.resources.max_file_size_mb);
    TEST_ASSERT_EQUAL(200, config.resources.max_log_size_mb);

    json_decref(root);
    cleanup_resources_config(&config.resources);
}

// ===== MONITORING TESTS =====

void test_load_resources_config_monitoring(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* resources_section = json_object();
    json_t* monitoring_section = json_object();

    // Set up monitoring configuration
    json_object_set(monitoring_section, "EnforceLimits", json_false());
    json_object_set(monitoring_section, "LogUsage", json_false());
    json_object_set(monitoring_section, "CheckIntervalMS", json_integer(10000));

    json_object_set(resources_section, "Monitoring", monitoring_section);
    json_object_set(root, "Resources", resources_section);

    bool result = load_resources_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.resources.enforce_limits);
    TEST_ASSERT_FALSE(config.resources.log_usage);
    TEST_ASSERT_EQUAL(10000, config.resources.check_interval_ms);

    json_decref(root);
    cleanup_resources_config(&config.resources);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_resources_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_resources_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_resources_config_empty_config(void) {
    ResourceConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_resources_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_EQUAL(0, config.max_memory_mb);
    TEST_ASSERT_EQUAL(0, config.max_buffer_size);
    TEST_ASSERT_EQUAL(0, config.min_threads);
    TEST_ASSERT_EQUAL(0, config.max_threads);
    TEST_ASSERT_FALSE(config.enforce_limits);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_resources_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_resources_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_resources_config_basic(void) {
    ResourceConfig config = {0};

    // Initialize with test data
    config.max_memory_mb = 1024;
    config.max_buffer_size = 1048576;
    config.min_buffer_size = 4096;
    config.max_queue_size = 10000;
    config.max_queue_memory_mb = 100;
    config.max_queue_blocks = 1000;
    config.queue_timeout_ms = 5000;
    config.post_processor_buffer_size = 65536;
    config.min_threads = 4;
    config.max_threads = 32;
    config.thread_stack_size = 65536;
    config.max_open_files = 1024;
    config.max_file_size_mb = 1024;
    config.max_log_size_mb = 100;
    config.enforce_limits = true;
    config.log_usage = true;
    config.check_interval_ms = 5000;

    // Dump should not crash and handle the data properly
    dump_resources_config(&config);

    // Clean up
    cleanup_resources_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_resources_config_null_root);
    RUN_TEST(test_load_resources_config_empty_json);

    // Memory limits tests
    RUN_TEST(test_load_resources_config_memory_limits);
    RUN_TEST(test_load_resources_config_queue_settings);
    RUN_TEST(test_load_resources_config_thread_limits);
    RUN_TEST(test_load_resources_config_file_limits);
    RUN_TEST(test_load_resources_config_monitoring);

    // Cleanup function tests
    RUN_TEST(test_cleanup_resources_config_null_pointer);
    RUN_TEST(test_cleanup_resources_config_empty_config);

    // Dump function tests
    RUN_TEST(test_dump_resources_config_null_pointer);
    RUN_TEST(test_dump_resources_config_basic);

    return UNITY_END();
}