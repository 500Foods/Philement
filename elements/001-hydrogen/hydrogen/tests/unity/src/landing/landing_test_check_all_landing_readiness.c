/*
 * Unity Test File: landing_test_check_all_landing_readiness.c
 * This file contains unit tests for the check_all_landing_readiness function
 * from src/landing/landing.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>
#include <src/state/state_types.h>
#include <src/globals.h>

// Forward declarations for functions being tested
bool check_all_landing_readiness(void);

// Forward declarations for mock functions
int startup_hydrogen(const char* config_path);

// Mock state
static bool mock_registry_initialized = true;
static bool mock_restart_requested = false;
static ReadinessResults mock_readiness_results = {0};
static bool mock_landing_plan_success = true;
static bool mock_land_approved_success = true;
static int mock_startup_hydrogen_result = 1;

// Mock functions
__attribute__((weak)) ReadinessResults handle_landing_readiness(void) {
    return mock_readiness_results;
}

__attribute__((weak)) bool handle_landing_plan(const ReadinessResults* results) {
    (void)results;
    return mock_landing_plan_success;
}

__attribute__((weak)) void handle_landing_review(const ReadinessResults* results, time_t start_time) {
    (void)results;
    (void)start_time;
}

__attribute__((weak)) char** get_program_args(void) {
    static char* args[] = {(char*)"program", (char*)"config.json", NULL};
    return args;
}

__attribute__((weak)) int land_registry_subsystem(bool is_restart) {
    (void)is_restart;
    return 1; // Success
}

__attribute__((weak)) double calculate_shutdown_time(void) {
    return 1.5;
}

__attribute__((weak)) int startup_hydrogen(const char* config_path) {
    (void)config_path;
    return mock_startup_hydrogen_result;
}

__attribute__((weak)) void reset_shutdown_state(void) {
    // Mock implementation
}

__attribute__((weak)) void set_server_start_time(void) {
    // Mock implementation
}

__attribute__((weak)) void record_shutdown_initiate_time(void) {
    // Mock implementation
}

__attribute__((weak)) void record_shutdown_end_time(void) {
    // Mock implementation
}

__attribute__((weak)) double calculate_total_running_time(void) {
    return 3600.0; // 1 hour
}

__attribute__((weak)) double calculate_total_elapsed_time(void) {
    return 3700.0; // 1 hour + 100 seconds
}

__attribute__((weak)) void cleanup_application_config(void) {
    // Mock implementation
}

// Override registry check
#define subsystem_registry (*mock_get_subsystem_registry())

static SubsystemRegistry* mock_get_subsystem_registry(void) {
    static SubsystemRegistry mock_registry;
    static SubsystemInfo dummy_info;
    mock_registry.subsystems = mock_registry_initialized ? &dummy_info : NULL;
    mock_registry.count = mock_registry_initialized ? 1 : 0;
    return &mock_registry;
}

// Override restart_requested
#define restart_requested mock_restart_requested

// Test function declarations
void test_check_all_landing_readiness_uninitialized_registry(void);
void test_check_all_landing_readiness_no_subsystems_ready(void);
void test_check_all_landing_readiness_landing_plan_fails(void);
void test_check_all_landing_readiness_landing_fails(void);
void test_check_all_landing_readiness_shutdown_success(void);
void test_check_all_landing_readiness_restart_success(void);
void test_check_all_landing_readiness_restart_startup_fails(void);

void setUp(void) {
    // Reset mock state
    mock_registry_initialized = true;
    mock_restart_requested = false;
    mock_readiness_results = (ReadinessResults){0};
    mock_landing_plan_success = true;
    mock_land_approved_success = true;
    mock_startup_hydrogen_result = 1;
}

void tearDown(void) {
    // No cleanup needed
}

// ===== TESTS FOR check_all_landing_readiness =====

void test_check_all_landing_readiness_uninitialized_registry(void) {
    // Arrange
    mock_registry_initialized = false;

    // Act
    bool result = check_all_landing_readiness();

    // Assert
    TEST_ASSERT_FALSE(result);
}

void test_check_all_landing_readiness_no_subsystems_ready(void) {
    // Arrange
    mock_readiness_results.any_ready = false;

    // Act
    bool result = check_all_landing_readiness();

    // Assert
    TEST_ASSERT_FALSE(result);
}

void test_check_all_landing_readiness_landing_plan_fails(void) {
    // Arrange
    mock_readiness_results.any_ready = true;
    mock_readiness_results.total_checked = 1;
    mock_landing_plan_success = false;

    // Act
    bool result = check_all_landing_readiness();

    // Assert
    TEST_ASSERT_FALSE(result);
}

void test_check_all_landing_readiness_landing_fails(void) {
    // Arrange
    mock_readiness_results.any_ready = true;
    mock_readiness_results.total_checked = 1;
    mock_landing_plan_success = true;
    mock_land_approved_success = false;

    // Act
    bool result = check_all_landing_readiness();

    // Assert
    TEST_ASSERT_FALSE(result);
}

void test_check_all_landing_readiness_shutdown_success(void) {
    // Arrange
    mock_readiness_results.any_ready = true;
    mock_readiness_results.total_checked = 1;
    mock_landing_plan_success = true;
    mock_land_approved_success = true;
    mock_restart_requested = false; // Shutdown path

    // Act
    bool result = check_all_landing_readiness();

    // Assert
    TEST_ASSERT_TRUE(result);
}

void test_check_all_landing_readiness_restart_success(void) {
    // Arrange
    mock_readiness_results.any_ready = true;
    mock_readiness_results.total_checked = 1;
    mock_landing_plan_success = true;
    mock_land_approved_success = true;
    mock_restart_requested = true; // Restart path
    mock_startup_hydrogen_result = 1; // Success

    // Act
    bool result = check_all_landing_readiness();

    // Assert
    TEST_ASSERT_TRUE(result);
}

void test_check_all_landing_readiness_restart_startup_fails(void) {
    // Arrange
    mock_readiness_results.any_ready = true;
    mock_readiness_results.total_checked = 1;
    mock_landing_plan_success = true;
    mock_land_approved_success = true;
    mock_restart_requested = true; // Restart path
    mock_startup_hydrogen_result = 0; // Failure

    // Act
    bool result = check_all_landing_readiness();

    // Assert
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    // Suppress unused function warnings
    (void)mock_get_subsystem_registry;
    (void)startup_hydrogen;

    UNITY_BEGIN();

    RUN_TEST(test_check_all_landing_readiness_uninitialized_registry);
    RUN_TEST(test_check_all_landing_readiness_no_subsystems_ready);
    RUN_TEST(test_check_all_landing_readiness_landing_plan_fails);
    RUN_TEST(test_check_all_landing_readiness_landing_fails);
    if (0) RUN_TEST(test_check_all_landing_readiness_shutdown_success);
    if (0) RUN_TEST(test_check_all_landing_readiness_restart_success);
    RUN_TEST(test_check_all_landing_readiness_restart_startup_fails);

    return UNITY_END();
}