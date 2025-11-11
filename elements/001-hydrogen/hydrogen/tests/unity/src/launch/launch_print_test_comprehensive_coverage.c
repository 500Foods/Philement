/*
 * Unity Test File: Comprehensive Print Launch Tests
 * Tests for check_print_launch_readiness and launch_print_subsystem functions
 * with full edge case coverage
 */

// Enable mocks BEFORE including ANY headers
// USE_MOCK_LAUNCH defined by CMake

// Include mock headers immediately
#include <unity/mocks/mock_launch.h>

// Include Unity framework
#include <unity.h>

// Include source headers (functions will now be mocked)
#include <src/hydrogen.h>
#include <src/launch/launch.h>
#include <src/config/config.h>
#include <src/config/config_defaults.h>

// Forward declarations for functions being tested
LaunchReadiness check_print_launch_readiness(void);
int launch_print_subsystem(void);

// External references
extern AppConfig* app_config;
extern volatile sig_atomic_t print_system_shutdown;
// Forward declarations for test functions
void test_print_disabled_configuration(void);
void test_print_null_config(void);
void test_print_max_queued_jobs_below_min(void);
void test_print_max_queued_jobs_above_max(void);
void test_print_max_concurrent_jobs_below_min(void);
void test_print_max_concurrent_jobs_above_max(void);
void test_print_emergency_priority_below_min(void);
void test_print_emergency_priority_above_max(void);
void test_print_default_priority_out_of_range(void);
void test_print_maintenance_priority_out_of_range(void);
void test_print_system_priority_out_of_range(void);
void test_print_insufficient_emergency_system_spread(void);
void test_print_insufficient_system_maintenance_spread(void);
void test_print_insufficient_maintenance_default_spread(void);
void test_print_shutdown_wait_below_min(void);
void test_print_shutdown_wait_above_max(void);
void test_print_job_timeout_below_min(void);
void test_print_job_timeout_above_max(void);
void test_print_job_message_size_below_min(void);
void test_print_job_message_size_above_max(void);
void test_print_status_message_size_below_min(void);
void test_print_status_message_size_above_max(void);
void test_print_max_speed_below_min(void);
void test_print_max_speed_above_max(void);
void test_print_acceleration_below_min(void);
void test_print_acceleration_above_max(void);
void test_print_jerk_below_min(void);
void test_print_jerk_above_max(void);
void test_print_network_dependency_registration_failure(void);
void test_print_network_not_running(void);
void test_print_network_dependency_success(void);
void test_print_valid_configuration_defaults(void);
void test_print_valid_configuration_boundary_values(void);
void test_print_valid_configuration_max_values(void);
void test_launch_print_subsystem_success(void);
extern ServiceThreads print_threads;

// Test helper to create a minimal valid configuration using config_defaults
static void setup_minimal_valid_config(void) {
    if (!app_config) {
        app_config = calloc(1, sizeof(AppConfig));
        if (!app_config) {
            return;  // Let test fail naturally
        }
    }
    
    // Use the standard initialization which sets all defaults properly
    initialize_config_defaults(app_config);
    
    // Enable print subsystem (defaults have it disabled)
    app_config->print.enabled = true;
    // Now we have all required fields from defaults with valid values
}

// Test helper to cleanup configuration
static void cleanup_test_config(void) {
    if (app_config) {
        // Clean up print config strings if any
        free(app_config);
        app_config = NULL;
    }
}

void setUp(void) {
    // Reset mocks
    mock_launch_reset_all();
    
    // Setup default mock behavior for successful case
    mock_launch_set_get_subsystem_id_result(-1);  // Default: no subsystem ID
    mock_launch_set_add_dependency_result(true);
    mock_launch_set_is_subsystem_running_result(true);
    
    // Clean slate for each test
    cleanup_test_config();
}

void tearDown(void) {
    // Clean up after each test
    cleanup_test_config();
    
    // Reset mocks
    mock_launch_reset_all();
}

// ============================================================================
// CONFIGURATION VALIDATION TESTS
// ============================================================================

// Test 1: Print disabled in configuration
void test_print_disabled_configuration(void) {
    setup_minimal_valid_config();
    app_config->print.enabled = false;
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_PRINT, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 2: NULL app_config
void test_print_null_config(void) {
    app_config = NULL;
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// ============================================================================
// JOB LIMITS VALIDATION TESTS
// ============================================================================

// Test 3: Max queued jobs below minimum
void test_print_max_queued_jobs_below_min(void) {
    setup_minimal_valid_config();
    app_config->print.max_queued_jobs = 0;  // Below MIN_QUEUED_JOBS (1)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 4: Max queued jobs above maximum
void test_print_max_queued_jobs_above_max(void) {
    setup_minimal_valid_config();
    app_config->print.max_queued_jobs = 1001;  // Above MAX_QUEUED_JOBS (1000)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 5: Max concurrent jobs below minimum
void test_print_max_concurrent_jobs_below_min(void) {
    setup_minimal_valid_config();
    app_config->print.max_concurrent_jobs = 0;  // Below MIN_CONCURRENT_JOBS (1)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 6: Max concurrent jobs above maximum
void test_print_max_concurrent_jobs_above_max(void) {
    setup_minimal_valid_config();
    app_config->print.max_concurrent_jobs = 17;  // Above MAX_CONCURRENT_JOBS (16)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// ============================================================================
// PRIORITY VALIDATION TESTS
// ============================================================================

// Test 7: Emergency priority below minimum
void test_print_emergency_priority_below_min(void) {
    setup_minimal_valid_config();
    app_config->print.priorities.emergency_priority = -1;  // Below MIN_PRIORITY (0)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 8: Emergency priority above maximum
void test_print_emergency_priority_above_max(void) {
    setup_minimal_valid_config();
    app_config->print.priorities.emergency_priority = 101;  // Above MAX_PRIORITY (100)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 9: Default priority out of range
void test_print_default_priority_out_of_range(void) {
    setup_minimal_valid_config();
    app_config->print.priorities.default_priority = 101;
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 10: Maintenance priority out of range
void test_print_maintenance_priority_out_of_range(void) {
    setup_minimal_valid_config();
    app_config->print.priorities.maintenance_priority = -1;
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 11: System priority out of range
void test_print_system_priority_out_of_range(void) {
    setup_minimal_valid_config();
    app_config->print.priorities.system_priority = 101;
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 12: Insufficient spread between emergency and system
void test_print_insufficient_emergency_system_spread(void) {
    setup_minimal_valid_config();
    app_config->print.priorities.emergency_priority = 50;
    app_config->print.priorities.system_priority = 45;  // Only 5 difference, need 10
    app_config->print.priorities.maintenance_priority = 25;
    app_config->print.priorities.default_priority = 5;
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 13: Insufficient spread between system and maintenance
void test_print_insufficient_system_maintenance_spread(void) {
    setup_minimal_valid_config();
    app_config->print.priorities.emergency_priority = 80;
    app_config->print.priorities.system_priority = 60;
    app_config->print.priorities.maintenance_priority = 55;  // Only 5 difference
    app_config->print.priorities.default_priority = 35;
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 14: Insufficient spread between maintenance and default
void test_print_insufficient_maintenance_default_spread(void) {
    setup_minimal_valid_config();
    app_config->print.priorities.emergency_priority = 80;
    app_config->print.priorities.system_priority = 60;
    app_config->print.priorities.maintenance_priority = 40;
    app_config->print.priorities.default_priority = 35;  // Only 5 difference
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// ============================================================================
// TIMEOUT VALIDATION TESTS
// ============================================================================

// Test 15: Shutdown wait below minimum
void test_print_shutdown_wait_below_min(void) {
    setup_minimal_valid_config();
    app_config->print.timeouts.shutdown_wait_ms = 999;  // Below MIN_SHUTDOWN_WAIT (1000)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 16: Shutdown wait above maximum
void test_print_shutdown_wait_above_max(void) {
    setup_minimal_valid_config();
    app_config->print.timeouts.shutdown_wait_ms = 30001;  // Above MAX_SHUTDOWN_WAIT (30000)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 17: Job processing timeout below minimum
void test_print_job_timeout_below_min(void) {
    setup_minimal_valid_config();
    app_config->print.timeouts.job_processing_timeout_ms = 29999;  // Below MIN_JOB_TIMEOUT (30000)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 18: Job processing timeout above maximum
void test_print_job_timeout_above_max(void) {
    setup_minimal_valid_config();
    app_config->print.timeouts.job_processing_timeout_ms = 3600001;  // Above MAX_JOB_TIMEOUT (3600000)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// ============================================================================
// BUFFER VALIDATION TESTS  
// ============================================================================

// Test 19: Job message size below minimum
void test_print_job_message_size_below_min(void) {
    setup_minimal_valid_config();
    app_config->print.buffers.job_message_size = 127;  // Below MIN_MESSAGE_SIZE (128)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 20: Job message size above maximum
void test_print_job_message_size_above_max(void) {
    setup_minimal_valid_config();
    app_config->print.buffers.job_message_size = 16385;  // Above MAX_MESSAGE_SIZE (16384)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 21: Status message size below minimum
void test_print_status_message_size_below_min(void) {
    setup_minimal_valid_config();
    app_config->print.buffers.status_message_size = 127;  // Below MIN_MESSAGE_SIZE (128)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 22: Status message size above maximum
void test_print_status_message_size_above_max(void) {
    setup_minimal_valid_config();
    app_config->print.buffers.status_message_size = 16385;  // Above MAX_MESSAGE_SIZE (16384)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// ============================================================================
// MOTION CONTROL VALIDATION TESTS
// ============================================================================

// Test 23: Max speed below minimum
void test_print_max_speed_below_min(void) {
    setup_minimal_valid_config();
    app_config->print.motion.max_speed = 0.05;  // Below MIN_SPEED (0.1)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 24: Max speed above maximum
void test_print_max_speed_above_max(void) {
    setup_minimal_valid_config();
    app_config->print.motion.max_speed = 1000.1;  // Above MAX_SPEED (1000.0)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 25: Acceleration below minimum
void test_print_acceleration_below_min(void) {
    setup_minimal_valid_config();
    app_config->print.motion.acceleration = 0.05;  // Below MIN_ACCELERATION (0.1)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 26: Acceleration above maximum
void test_print_acceleration_above_max(void) {
    setup_minimal_valid_config();
    app_config->print.motion.acceleration = 5000.1;  // Above MAX_ACCELERATION (5000.0)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 27: Jerk below minimum
void test_print_jerk_below_min(void) {
    setup_minimal_valid_config();
    app_config->print.motion.jerk = 0.005;  // Below MIN_JERK (0.01)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 28: Jerk above maximum
void test_print_jerk_above_max(void) {
    setup_minimal_valid_config();
    app_config->print.motion.jerk = 100.1;  // Above MAX_JERK (100.0)
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// ============================================================================
// NETWORK DEPENDENCY TESTS
// ============================================================================

// Test 29: Network dependency registration failure
void test_print_network_dependency_registration_failure(void) {
    setup_minimal_valid_config();
    
    // Mock to return valid print ID so we enter the dependency block
    mock_launch_set_get_subsystem_id_result(15);  // Valid subsystem ID for Print
    // Mock dependency registration to fail
    mock_launch_set_add_dependency_result(false);
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 30: Network subsystem not running
void test_print_network_not_running(void) {
    setup_minimal_valid_config();
    
    // Mock to return valid print ID
    mock_launch_set_get_subsystem_id_result(15);
    // Mock add_dependency to succeed
    mock_launch_set_add_dependency_result(true);
    // Mock Network subsystem as not running
    mock_launch_set_is_subsystem_running_result(false);
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 31: Network dependency success path
void test_print_network_dependency_success(void) {
    setup_minimal_valid_config();
    
    // Set valid priority spreads
    app_config->print.priorities.emergency_priority = 60;
    app_config->print.priorities.system_priority = 40;
    app_config->print.priorities.maintenance_priority = 20;
    app_config->print.priorities.default_priority = 0;
    
    // Mock to return valid print ID
    mock_launch_set_get_subsystem_id_result(15);
    // Mock add_dependency to succeed
    mock_launch_set_add_dependency_result(true);
    // Mock Network subsystem as running
    mock_launch_set_is_subsystem_running_result(true);
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_PRINT, result.subsystem);
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// ============================================================================
// VALID CONFIGURATION TESTS
// ============================================================================

// Test 32: Valid configuration with all defaults
void test_print_valid_configuration_defaults(void) {
    setup_minimal_valid_config();
    
    // Set valid priority spreads (defaults may not have adequate spread)
    app_config->print.priorities.emergency_priority = 60;
    app_config->print.priorities.system_priority = 40;
    app_config->print.priorities.maintenance_priority = 20;
    app_config->print.priorities.default_priority = 0;
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_PRINT, result.subsystem);
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 33: Valid configuration with boundary values
void test_print_valid_configuration_boundary_values(void) {
    setup_minimal_valid_config();
    
    // Set all values to their minimum valid values
    app_config->print.max_queued_jobs = MIN_QUEUED_JOBS;
    app_config->print.max_concurrent_jobs = MIN_CONCURRENT_JOBS;
    app_config->print.priorities.emergency_priority = 60;
    app_config->print.priorities.system_priority = 40;
    app_config->print.priorities.maintenance_priority = 20;
    app_config->print.priorities.default_priority = 0;
    app_config->print.timeouts.shutdown_wait_ms = MIN_SHUTDOWN_WAIT;
    app_config->print.timeouts.job_processing_timeout_ms = MIN_JOB_TIMEOUT;
    app_config->print.buffers.job_message_size = MIN_MESSAGE_SIZE;
    app_config->print.buffers.status_message_size = MIN_MESSAGE_SIZE;
    app_config->print.motion.max_speed = MIN_SPEED;
    app_config->print.motion.acceleration = MIN_ACCELERATION;
    app_config->print.motion.jerk = MIN_JERK;
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 34: Valid configuration with maximum values
void test_print_valid_configuration_max_values(void) {
    setup_minimal_valid_config();
    
    // Set all values to their maximum valid values
    app_config->print.max_queued_jobs = MAX_QUEUED_JOBS;
    app_config->print.max_concurrent_jobs = MAX_CONCURRENT_JOBS;
    app_config->print.priorities.emergency_priority = MAX_PRIORITY;
    app_config->print.priorities.system_priority = 70;
    app_config->print.priorities.maintenance_priority = 40;
    app_config->print.priorities.default_priority = 10;
    app_config->print.timeouts.shutdown_wait_ms = MAX_SHUTDOWN_WAIT;
    app_config->print.timeouts.job_processing_timeout_ms = MAX_JOB_TIMEOUT;
    app_config->print.buffers.job_message_size = MAX_MESSAGE_SIZE;
    app_config->print.buffers.status_message_size = MAX_MESSAGE_SIZE;
    app_config->print.motion.max_speed = MAX_SPEED;
    app_config->print.motion.acceleration = MAX_ACCELERATION;
    app_config->print.motion.jerk = MAX_JERK;
    
    LaunchReadiness result = check_print_launch_readiness();
    
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// ============================================================================
// LAUNCH SUBSYSTEM TESTS
// ============================================================================

// Test 35: Launch print subsystem
void test_launch_print_subsystem_success(void) {
    int result = launch_print_subsystem();
    
    TEST_ASSERT_EQUAL(1, result);
}

int main(void) {
    UNITY_BEGIN();
    
    // Configuration validation tests
    RUN_TEST(test_print_disabled_configuration);
    RUN_TEST(test_print_null_config);
    
    // Job limits validation tests
    RUN_TEST(test_print_max_queued_jobs_below_min);
    RUN_TEST(test_print_max_queued_jobs_above_max);
    RUN_TEST(test_print_max_concurrent_jobs_below_min);
    RUN_TEST(test_print_max_concurrent_jobs_above_max);
    
    // Priority validation tests
    RUN_TEST(test_print_emergency_priority_below_min);
    RUN_TEST(test_print_emergency_priority_above_max);
    RUN_TEST(test_print_default_priority_out_of_range);
    RUN_TEST(test_print_maintenance_priority_out_of_range);
    RUN_TEST(test_print_system_priority_out_of_range);
    RUN_TEST(test_print_insufficient_emergency_system_spread);
    RUN_TEST(test_print_insufficient_system_maintenance_spread);
    RUN_TEST(test_print_insufficient_maintenance_default_spread);
    
    // Timeout validation tests
    RUN_TEST(test_print_shutdown_wait_below_min);
    RUN_TEST(test_print_shutdown_wait_above_max);
    RUN_TEST(test_print_job_timeout_below_min);
    RUN_TEST(test_print_job_timeout_above_max);
    
    // Buffer validation tests
    RUN_TEST(test_print_job_message_size_below_min);
    RUN_TEST(test_print_job_message_size_above_max);
    RUN_TEST(test_print_status_message_size_below_min);
    RUN_TEST(test_print_status_message_size_above_max);
    
    // Motion control validation tests
    RUN_TEST(test_print_max_speed_below_min);
    RUN_TEST(test_print_max_speed_above_max);
    RUN_TEST(test_print_acceleration_below_min);
    RUN_TEST(test_print_acceleration_above_max);
    RUN_TEST(test_print_jerk_below_min);
    RUN_TEST(test_print_jerk_above_max);
    
    // Network dependency tests
    RUN_TEST(test_print_network_dependency_registration_failure);
    RUN_TEST(test_print_network_not_running);
    RUN_TEST(test_print_network_dependency_success);
    
    // Valid configuration tests
    RUN_TEST(test_print_valid_configuration_defaults);
    RUN_TEST(test_print_valid_configuration_boundary_values);
    RUN_TEST(test_print_valid_configuration_max_values);
    
    // Launch subsystem tests
    RUN_TEST(test_launch_print_subsystem_success);
    
    return UNITY_END();
}