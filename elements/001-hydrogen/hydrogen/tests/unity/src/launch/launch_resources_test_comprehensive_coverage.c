/*
 * Unity Test File: Comprehensive Resources Launch Tests
 * Tests for check_resources_launch_readiness function with full edge case coverage
 */

// Enable mocks BEFORE including ANY headers
#define USE_MOCK_SYSTEM

// Include mock headers immediately
#include <unity/mocks/mock_system.h>

// Include Unity framework
#include <unity.h>

// Include source headers (functions will now be mocked)
#include <src/hydrogen.h>
#include <src/launch/launch.h>
#include <src/config/config.h>
#include <src/config/config_defaults.h>

// Forward declarations for functions being tested
LaunchReadiness check_resources_launch_readiness(void);
bool validate_memory_limits(const ResourceConfig* config, int* msg_count, const char** messages);
bool validate_queue_settings(const ResourceConfig* config, int* msg_count, const char** messages);
bool validate_thread_limits(const ResourceConfig* config, int* msg_count, const char** messages);
bool validate_file_limits(const ResourceConfig* config, int* msg_count, const char** messages);
bool validate_monitoring_settings(const ResourceConfig* config, int* msg_count, const char** messages);

// External references
extern AppConfig* app_config;

// Forward declarations for all test functions
void test_resources_null_app_config(void);
void test_resources_buffer_size_too_small(void);
void test_resources_buffer_size_too_large(void);
void test_resources_buffer_exceeds_quarter_memory(void);
void test_resources_queue_memory_exceeds_half_total(void);
void test_resources_max_threads_exceeds_limit(void);
void test_resources_stack_size_too_small(void);
void test_resources_stack_size_too_large(void);
void test_resources_file_size_exceeds_2x_memory(void);
void test_resources_log_size_too_small(void);
void test_resources_log_size_too_large(void);
void test_resources_valid_configuration(void);
void test_resources_multiple_failures(void);
void test_resources_boundary_values_valid(void);
void test_resources_boundary_values_max_valid(void);
void test_resources_memory_below_minimum(void);
void test_resources_memory_above_maximum(void);
void test_resources_queue_size_below_minimum(void);
void test_resources_queue_size_above_maximum(void);
void test_resources_min_threads_below_minimum(void);
void test_resources_min_threads_exceeds_max(void);
void test_resources_open_files_below_minimum(void);
void test_resources_open_files_above_maximum(void);
void test_resources_check_interval_below_minimum(void);
void test_resources_check_interval_above_maximum(void);

// Test helper to create a minimal valid configuration
static void setup_minimal_valid_config(void) {
    if (!app_config) {
        app_config = calloc(1, sizeof(AppConfig));
        if (!app_config) {
            return;  // Let test fail naturally
        }
    }
    
    // Set up minimal valid resource configuration
    // Use defaults from config_defaults.c
    app_config->resources.max_memory_mb = 1024;  // 1GB
    app_config->resources.max_buffer_size = 1048576;  // 1MB
    app_config->resources.min_buffer_size = 1024;  // 1KB
    app_config->resources.max_queue_size = 10000;
    app_config->resources.max_queue_memory_mb = 100;  // 100MB
    app_config->resources.max_queue_blocks = 1000;
    app_config->resources.queue_timeout_ms = 5000;  // 5 seconds
    app_config->resources.post_processor_buffer_size = 65536;  // 64KB
    app_config->resources.min_threads = 2;
    app_config->resources.max_threads = 64;
    app_config->resources.thread_stack_size = 1048576;  // 1MB
    app_config->resources.max_open_files = 1024;
    app_config->resources.max_file_size_mb = 100;  // 100MB
    app_config->resources.max_log_size_mb = 50;  // 50MB
    app_config->resources.enforce_limits = true;
    app_config->resources.log_usage = false;
    app_config->resources.check_interval_ms = 60000;  // 1 minute
}

// Test helper to cleanup configuration
static void cleanup_test_config(void) {
    if (app_config) {
        // Just free the config struct itself - no internal allocations to free
        // since we're not using initialize_config_defaults
        free(app_config);
        app_config = NULL;
    }
}

void setUp(void) {
    // Reset mocks
    mock_system_reset_all();
    
    // Clean slate for each test
    cleanup_test_config();
}

void tearDown(void) {
    // Clean up after each test
    cleanup_test_config();
    
    // Reset mocks
    mock_system_reset_all();
}

// Test 1: NULL app_config (lines 30-34)
void test_resources_null_app_config(void) {
    // Ensure app_config is NULL
    app_config = NULL;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 3: Memory limits - max_buffer_size too small (lines 92-93)
void test_resources_buffer_size_too_small(void) {
    setup_minimal_valid_config();
    
    // Set buffer size below minimum
    app_config->resources.max_buffer_size = MIN_RESOURCE_BUFFER_SIZE - 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 4: Memory limits - max_buffer_size too large (lines 92-93)
void test_resources_buffer_size_too_large(void) {
    setup_minimal_valid_config();
    
    // Set buffer size above maximum
    app_config->resources.max_buffer_size = MAX_RESOURCE_BUFFER_SIZE + 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 5: Memory limits - buffer exceeds 1/4 of total memory (lines 97-98)
void test_resources_buffer_exceeds_quarter_memory(void) {
    setup_minimal_valid_config();
    
    // Set max_memory to 1024 MB and buffer to more than 1/4 (256MB)
    app_config->resources.max_memory_mb = 1024;
    app_config->resources.max_buffer_size = (1024 * 1024 * 1024 / 4) + 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 6: Queue settings - queue memory exceeds 1/2 of total memory (lines 115-116)
void test_resources_queue_memory_exceeds_half_total(void) {
    setup_minimal_valid_config();
    
    // Set max_memory to 1024 MB and queue memory to more than 1/2 (512MB)
    app_config->resources.max_memory_mb = 1024;
    app_config->resources.max_queue_memory_mb = 513;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 7: Thread limits - max_threads exceeds MAX_THREADS (lines 134-137)
void test_resources_max_threads_exceeds_limit(void) {
    setup_minimal_valid_config();
    
    // Set max_threads above the system limit
    app_config->resources.max_threads = MAX_THREADS + 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 8: Thread limits - thread_stack_size too small (lines 142-145)
void test_resources_stack_size_too_small(void) {
    setup_minimal_valid_config();
    
    // Set stack size below minimum
    app_config->resources.thread_stack_size = MIN_STACK_SIZE - 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 9: Thread limits - thread_stack_size too large (lines 142-145)
void test_resources_stack_size_too_large(void) {
    setup_minimal_valid_config();
    
    // Set stack size above maximum
    app_config->resources.thread_stack_size = MAX_STACK_SIZE + 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 10: File limits - max_file_size exceeds 2x total memory (lines 162-163)
void test_resources_file_size_exceeds_2x_memory(void) {
    setup_minimal_valid_config();
    
    // Set max_memory to 1024 MB and file size to more than 2x (2048MB)
    app_config->resources.max_memory_mb = 1024;
    app_config->resources.max_file_size_mb = 2049;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 11: File limits - max_log_size too small (lines 168-171)
void test_resources_log_size_too_small(void) {
    setup_minimal_valid_config();
    
    // Set log size below minimum
    app_config->resources.max_log_size_mb = MIN_LOG_SIZE_MB - 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 12: File limits - max_log_size too large (lines 168-171)
void test_resources_log_size_too_large(void) {
    setup_minimal_valid_config();
    
    // Set log size above maximum
    app_config->resources.max_log_size_mb = MAX_LOG_SIZE_MB + 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 13: Valid configuration - all checks pass
void test_resources_valid_configuration(void) {
    setup_minimal_valid_config();
    
    // Adjust any defaults that might not pass validation
    // Ensure buffer size doesn't exceed 1/4 of memory
    size_t max_allowed_buffer = (size_t)(((unsigned long long)app_config->resources.max_memory_mb * 1024 * 1024) / 4);
    if (app_config->resources.max_buffer_size > max_allowed_buffer) {
        app_config->resources.max_buffer_size = max_allowed_buffer / 2;
    }
    
    // Ensure queue memory doesn't exceed 1/2 of total memory
    if (app_config->resources.max_queue_memory_mb > app_config->resources.max_memory_mb / 2) {
        app_config->resources.max_queue_memory_mb = app_config->resources.max_memory_mb / 4;
    }
    
    // Ensure file size doesn't exceed 2x memory
    if (app_config->resources.max_file_size_mb > app_config->resources.max_memory_mb * 2) {
        app_config->resources.max_file_size_mb = app_config->resources.max_memory_mb;
    }
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 14: Multiple validation failures - buffer size and queue memory
void test_resources_multiple_failures(void) {
    setup_minimal_valid_config();
    
    // Set multiple invalid values
    app_config->resources.max_buffer_size = MIN_RESOURCE_BUFFER_SIZE - 1;
    app_config->resources.max_queue_memory_mb = app_config->resources.max_memory_mb + 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 15: Edge case - exact boundary values (valid)
void test_resources_boundary_values_valid(void) {
    setup_minimal_valid_config();
    
    // Set values at exact boundaries (should be valid)
    app_config->resources.max_memory_mb = MIN_MEMORY_MB;
    // app_config->resources.max_buffer_size = MIN_RESOURCE_BUFFER_SIZE;
    app_config->resources.max_queue_size = MIN_QUEUE_SIZE;
    app_config->resources.min_threads = MIN_THREADS;
    app_config->resources.max_threads = MIN_THREADS;  // min == max is valid
    app_config->resources.thread_stack_size = MIN_STACK_SIZE;
    app_config->resources.max_open_files = MIN_OPEN_FILES;
    app_config->resources.max_log_size_mb = MIN_LOG_SIZE_MB;
    app_config->resources.check_interval_ms = MIN_CHECK_INTERVAL_MS;
    
    // Ensure buffer doesn't exceed 1/4 memory
    app_config->resources.max_buffer_size = (app_config->resources.max_memory_mb * 1024 * 1024) / 5;
    
    // Ensure queue memory doesn't exceed 1/2 memory
    app_config->resources.max_queue_memory_mb = app_config->resources.max_memory_mb / 3;
    
    // Ensure file size doesn't exceed 2x memory
    app_config->resources.max_file_size_mb = app_config->resources.max_memory_mb;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 16: Edge case - maximum boundary values (valid)
void test_resources_boundary_values_max_valid(void) {
    setup_minimal_valid_config();
    
    // Set values at maximum boundaries (should be valid)
    app_config->resources.max_memory_mb = 4096;  // Use reasonable value instead of MAX
    app_config->resources.max_queue_size = MAX_QUEUE_SIZE;
    app_config->resources.max_threads = MAX_THREADS;
    app_config->resources.thread_stack_size = MAX_STACK_SIZE;
    app_config->resources.max_open_files = MAX_OPEN_FILES;
    app_config->resources.max_log_size_mb = MAX_LOG_SIZE_MB;
    app_config->resources.check_interval_ms = MAX_CHECK_INTERVAL_MS;
    
    // Calculate buffer size to be exactly 1/4 of memory (minus 1 KB to be safe)
    size_t calculated_buffer = (size_t)(((unsigned long long)app_config->resources.max_memory_mb * 1024 * 1024) / 4) - 1024;
    
    // Ensure buffer respects MIN/MAX limits
    if (calculated_buffer < MIN_RESOURCE_BUFFER_SIZE) {
        app_config->resources.max_buffer_size = MIN_RESOURCE_BUFFER_SIZE;
    } else if (calculated_buffer > MAX_RESOURCE_BUFFER_SIZE) {
        app_config->resources.max_buffer_size = MAX_RESOURCE_BUFFER_SIZE;
    } else {
        app_config->resources.max_buffer_size = calculated_buffer;
    }
    
    // Ensure queue memory doesn't exceed 1/2 memory
    app_config->resources.max_queue_memory_mb = app_config->resources.max_memory_mb / 2 - 1;
    
    // Ensure file size doesn't exceed 2x memory
    app_config->resources.max_file_size_mb = app_config->resources.max_memory_mb * 2 - 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 17: Memory limits - memory below minimum
void test_resources_memory_below_minimum(void) {
    setup_minimal_valid_config();
    
    // Set memory below minimum
    app_config->resources.max_memory_mb = MIN_MEMORY_MB - 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 18: Memory limits - memory above maximum
void test_resources_memory_above_maximum(void) {
    setup_minimal_valid_config();
    
    // Set memory above maximum
    app_config->resources.max_memory_mb = MAX_MEMORY_MB + 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 19: Queue settings - queue size below minimum
void test_resources_queue_size_below_minimum(void) {
    setup_minimal_valid_config();
    
    // Set queue size below minimum
    app_config->resources.max_queue_size = MIN_QUEUE_SIZE - 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 20: Queue settings - queue size above maximum
void test_resources_queue_size_above_maximum(void) {
    setup_minimal_valid_config();
    
    // Set queue size above maximum
    app_config->resources.max_queue_size = MAX_QUEUE_SIZE + 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 21: Thread limits - min_threads below minimum
void test_resources_min_threads_below_minimum(void) {
    setup_minimal_valid_config();
    
    // Set min_threads below minimum
    app_config->resources.min_threads = MIN_THREADS - 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 22: Thread limits - min_threads exceeds max_threads
void test_resources_min_threads_exceeds_max(void) {
    setup_minimal_valid_config();
    
    // Set min_threads greater than max_threads
    app_config->resources.min_threads = 10;
    app_config->resources.max_threads = 5;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 23: File limits - max_open_files below minimum
void test_resources_open_files_below_minimum(void) {
    setup_minimal_valid_config();
    
    // Set open files below minimum
    app_config->resources.max_open_files = MIN_OPEN_FILES - 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 24: File limits - max_open_files above maximum
void test_resources_open_files_above_maximum(void) {
    setup_minimal_valid_config();
    
    // Set open files above maximum
    app_config->resources.max_open_files = MAX_OPEN_FILES + 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 25: Monitoring settings - check interval below minimum
void test_resources_check_interval_below_minimum(void) {
    setup_minimal_valid_config();
    
    // Set check interval below minimum
    app_config->resources.check_interval_ms = MIN_CHECK_INTERVAL_MS - 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 26: Monitoring settings - check interval above maximum
void test_resources_check_interval_above_maximum(void) {
    setup_minimal_valid_config();
    
    // Set check interval above maximum
    app_config->resources.check_interval_ms = MAX_CHECK_INTERVAL_MS + 1;
    
    LaunchReadiness result = check_resources_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

int main(void) {
    UNITY_BEGIN();
    
    // NULL config tests
    RUN_TEST(test_resources_null_app_config);
    
    // Memory limits validation tests
    RUN_TEST(test_resources_memory_below_minimum);
    RUN_TEST(test_resources_memory_above_maximum);
    RUN_TEST(test_resources_buffer_size_too_small);
    RUN_TEST(test_resources_buffer_size_too_large);
    RUN_TEST(test_resources_buffer_exceeds_quarter_memory);
    
    // Queue settings validation tests
    RUN_TEST(test_resources_queue_size_below_minimum);
    RUN_TEST(test_resources_queue_size_above_maximum);
    RUN_TEST(test_resources_queue_memory_exceeds_half_total);
    
    // Thread limits validation tests
    RUN_TEST(test_resources_min_threads_below_minimum);
    RUN_TEST(test_resources_min_threads_exceeds_max);
    RUN_TEST(test_resources_max_threads_exceeds_limit);
    RUN_TEST(test_resources_stack_size_too_small);
    RUN_TEST(test_resources_stack_size_too_large);
    
    // File limits validation tests
    RUN_TEST(test_resources_open_files_below_minimum);
    RUN_TEST(test_resources_open_files_above_maximum);
    RUN_TEST(test_resources_file_size_exceeds_2x_memory);
    RUN_TEST(test_resources_log_size_too_small);
    RUN_TEST(test_resources_log_size_too_large);
    
    // Monitoring settings validation tests
    RUN_TEST(test_resources_check_interval_below_minimum);
    RUN_TEST(test_resources_check_interval_above_maximum);
    
    // Valid configuration tests
    RUN_TEST(test_resources_valid_configuration);
    RUN_TEST(test_resources_boundary_values_valid);
    RUN_TEST(test_resources_boundary_values_max_valid);
    
    // Multiple failures test
    RUN_TEST(test_resources_multiple_failures);
    
    return UNITY_END();
}