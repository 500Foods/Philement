/*
 * Unity Test File: landing_test_check_all_landing_readiness.c
 * This file contains unit tests for the check_all_landing_readiness function
 * from src/landing/landing.c
 *
 * handle_landing_* and land_registry_subsystem are weak-overridden here so the
 * archive members are not pulled. startup_hydrogen is weak under UNITY_TEST_MODE.
 * Real subsystem_registry and restart_requested globals are used.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/landing/landing.h>
#include <src/state/state_types.h>
#include <src/globals.h>
#include <src/registry/registry.h>

bool check_all_landing_readiness(void);
int startup_hydrogen(const char* config_path);

extern volatile sig_atomic_t restart_requested;

static bool mock_restart_requested = false;
static ReadinessResults mock_readiness_results = {0};
static bool mock_landing_plan_success = true;
static int mock_startup_hydrogen_result = 1;
static int mock_land_registry_result = 1;

__attribute__((weak))
ReadinessResults handle_landing_readiness(void) {
    return mock_readiness_results;
}

__attribute__((weak))
bool handle_landing_plan(const ReadinessResults* results) {
    (void)results;
    return mock_landing_plan_success;
}

__attribute__((weak))
void handle_landing_review(const ReadinessResults* results, time_t start_time) {
    (void)results;
    (void)start_time;
}

__attribute__((weak))
char** get_program_args(void) {
    static char* args[] = {(char*)"program", (char*)"config.json", NULL};
    return args;
}

__attribute__((weak))
int land_registry_subsystem(bool is_restart) {
    (void)is_restart;
    return mock_land_registry_result;
}

__attribute__((weak))
double calculate_shutdown_time(void) {
    return 1.5;
}

/* Strong override — launch.o defines this weak under UNITY_TEST_MODE */
int startup_hydrogen(const char* config_path) {
    (void)config_path;
    return mock_startup_hydrogen_result;
}

__attribute__((weak))
void reset_shutdown_state(void) {
}

__attribute__((weak))
void set_server_start_time(void) {
}

__attribute__((weak))
void record_shutdown_initiate_time(void) {
}

__attribute__((weak))
void record_shutdown_end_time(void) {
}

__attribute__((weak))
double calculate_total_running_time(void) {
    return 3600.0;
}

__attribute__((weak))
double calculate_total_elapsed_time(void) {
    return 3700.0;
}

__attribute__((weak))
void cleanup_application_config(void) {
}

/* Intercept land_approved_subsystems via weak — but it is strong in landing.o.
 * landing.o is always pulled for check_all_landing_readiness, so this weak
 * never wins. Control readiness results so real land_approved gets empty work. */

void test_check_all_landing_readiness_uninitialized_registry(void);
void test_check_all_landing_readiness_no_subsystems_ready(void);
void test_check_all_landing_readiness_landing_plan_fails(void);
void test_check_all_landing_readiness_landing_fails(void);
void test_check_all_landing_readiness_shutdown_success(void);
void test_check_all_landing_readiness_restart_success(void);
void test_check_all_landing_readiness_restart_startup_fails(void);

void setUp(void) {
    mock_restart_requested = false;
    mock_readiness_results = (ReadinessResults){0};
    mock_landing_plan_success = true;
    mock_startup_hydrogen_result = 1;
    mock_land_registry_result = 1;
    restart_requested = 0;
    init_registry();
}

void tearDown(void) {
    restart_requested = 0;
    init_registry();
}

void test_check_all_landing_readiness_uninitialized_registry(void) {
    /* Empty registry (setUp init with no registrations still has capacity;
     * clear to uninitialized shape used by the guard). */
    if (subsystem_registry.subsystems) {
        free(subsystem_registry.subsystems);
        subsystem_registry.subsystems = NULL;
    }
    subsystem_registry.count = 0;
    subsystem_registry.capacity = 0;

    TEST_ASSERT_FALSE(check_all_landing_readiness());
}

void test_check_all_landing_readiness_no_subsystems_ready(void) {
    register_subsystem(SR_PRINT, NULL, NULL, NULL, NULL, NULL);
    mock_readiness_results.any_ready = false;

    TEST_ASSERT_FALSE(check_all_landing_readiness());
}

void test_check_all_landing_readiness_landing_plan_fails(void) {
    register_subsystem(SR_PRINT, NULL, NULL, NULL, NULL, NULL);
    mock_readiness_results.any_ready = true;
    mock_readiness_results.total_checked = 1;
    mock_landing_plan_success = false;

    TEST_ASSERT_FALSE(check_all_landing_readiness());
}

void test_check_all_landing_readiness_landing_fails(void) {
    /* Registry landing failure on shutdown path fails overall success */
    register_subsystem(SR_PRINT, NULL, NULL, NULL, NULL, NULL);
    mock_readiness_results.any_ready = true;
    mock_readiness_results.total_checked = 0;
    mock_landing_plan_success = true;
    mock_land_registry_result = 0;
    restart_requested = 0;

    TEST_ASSERT_FALSE(check_all_landing_readiness());
}

void test_check_all_landing_readiness_shutdown_success(void) {
    register_subsystem(SR_PRINT, NULL, NULL, NULL, NULL, NULL);
    mock_readiness_results.any_ready = true;
    mock_readiness_results.total_checked = 0;
    mock_landing_plan_success = true;
    restart_requested = 0;

    TEST_ASSERT_TRUE(check_all_landing_readiness());
}

void test_check_all_landing_readiness_restart_success(void) {
    register_subsystem(SR_PRINT, NULL, NULL, NULL, NULL, NULL);
    mock_readiness_results.any_ready = true;
    mock_readiness_results.total_checked = 0;
    mock_landing_plan_success = true;
    restart_requested = 1;
    mock_startup_hydrogen_result = 1;

    TEST_ASSERT_TRUE(check_all_landing_readiness());
}

void test_check_all_landing_readiness_restart_startup_fails(void) {
    register_subsystem(SR_PRINT, NULL, NULL, NULL, NULL, NULL);
    mock_readiness_results.any_ready = true;
    mock_readiness_results.total_checked = 0;
    mock_landing_plan_success = true;
    restart_requested = 1;
    mock_startup_hydrogen_result = 0;

    TEST_ASSERT_FALSE(check_all_landing_readiness());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_all_landing_readiness_uninitialized_registry);
    RUN_TEST(test_check_all_landing_readiness_no_subsystems_ready);
    RUN_TEST(test_check_all_landing_readiness_landing_plan_fails);
    RUN_TEST(test_check_all_landing_readiness_landing_fails);
    RUN_TEST(test_check_all_landing_readiness_shutdown_success);
    RUN_TEST(test_check_all_landing_readiness_restart_success);
    RUN_TEST(test_check_all_landing_readiness_restart_startup_fails);

    return UNITY_END();
}
