/*
 * Unity Test File: Beryllium beryllium_create_config Function Tests
 * This file contains comprehensive unit tests for the beryllium_create_config() function
 * from src/print/beryllium.c
 *
 * Coverage Goals:
 * - Test configuration creation from AppConfig
 * - Test default values when AppConfig is NULL
 * - Test proper value extraction from AppConfig
 * - Test all configuration parameters
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the beryllium module
#include <src/print/beryllium.h>

// Forward declaration for the function being tested
BerylliumConfig beryllium_create_config(void);

// Mock AppConfig for testing
extern AppConfig *app_config;

// Function prototypes for test functions
void test_beryllium_create_config_with_null_app_config(void);
void test_beryllium_create_config_with_valid_app_config(void);
void test_beryllium_create_config_default_values(void);
void test_beryllium_create_config_motion_parameters(void);
void test_beryllium_create_config_filament_parameters(void);
void test_beryllium_create_config_speed_parameters(void);
void test_beryllium_create_config_all_parameters(void);

// Test fixtures
static AppConfig test_app_config;

void setUp(void) {
    // Initialize test AppConfig with known values for all tests
    memset(&test_app_config, 0, sizeof(AppConfig));

    // Set default motion parameters that most tests will use
    test_app_config.print.motion.acceleration = 1000.0;
    test_app_config.print.motion.z_acceleration = 200.0;
    test_app_config.print.motion.e_acceleration = 300.0;
    test_app_config.print.motion.max_speed_xy = 150.0;
    test_app_config.print.motion.max_speed_travel = 200.0;
    test_app_config.print.motion.max_speed_z = 30.0;

    // Set the global app_config pointer
    app_config = &test_app_config;
}

void tearDown(void) {
    // Reset the global app_config pointer
    app_config = NULL;
}

//=============================================================================
// Basic Configuration Creation Tests
//=============================================================================

void test_beryllium_create_config_with_null_app_config(void) {
    // Save original app_config
    AppConfig *original_app_config = app_config;
    app_config = NULL;

    BerylliumConfig config = beryllium_create_config();

    // Test that default values are used when app_config is NULL
    TEST_ASSERT_TRUE(config.acceleration == 500.0);
    TEST_ASSERT_TRUE(config.z_acceleration == 100.0);
    TEST_ASSERT_TRUE(config.extruder_acceleration == 250.0);
    TEST_ASSERT_TRUE(config.max_speed_xy == 100.0);
    TEST_ASSERT_TRUE(config.max_speed_travel == 150.0);
    TEST_ASSERT_TRUE(config.max_speed_z == 20.0);
    TEST_ASSERT_TRUE(config.default_feedrate == 3000.0);
    TEST_ASSERT_TRUE(config.filament_diameter == 1.75);
    TEST_ASSERT_TRUE(config.filament_density == 1.24);
    (void)config; // Suppress unused variable warning

    // Restore original app_config
    app_config = original_app_config;
}

void test_beryllium_create_config_with_valid_app_config(void) {
    // Initialize test AppConfig with known values
    memset(&test_app_config, 0, sizeof(AppConfig));
    test_app_config.print.motion.acceleration = 1000.0;
    test_app_config.print.motion.z_acceleration = 200.0;
    test_app_config.print.motion.e_acceleration = 300.0;
    test_app_config.print.motion.max_speed_xy = 150.0;
    test_app_config.print.motion.max_speed_travel = 200.0;
    test_app_config.print.motion.max_speed_z = 30.0;

    // Set test app_config
    app_config = &test_app_config;

    BerylliumConfig config = beryllium_create_config();

    // Test that values are extracted from app_config
    TEST_ASSERT_TRUE(config.acceleration == 1000.0);
    TEST_ASSERT_TRUE(config.z_acceleration == 200.0);
    TEST_ASSERT_TRUE(config.extruder_acceleration == 300.0);
    TEST_ASSERT_TRUE(config.max_speed_xy == 150.0);
    TEST_ASSERT_TRUE(config.max_speed_travel == 200.0);
    TEST_ASSERT_TRUE(config.max_speed_z == 30.0);

    // Test that hardcoded values are set correctly
    TEST_ASSERT_TRUE(config.default_feedrate == 3000.0);
    TEST_ASSERT_TRUE(config.filament_diameter == 1.75);
    TEST_ASSERT_TRUE(config.filament_density == 1.24);
    (void)config; // Suppress unused variable warning
}

void test_beryllium_create_config_default_values(void) {
    // Save original app_config and create a fresh zero-initialized one
    AppConfig *original_app_config = app_config;
    AppConfig zero_config;
    memset(&zero_config, 0, sizeof(AppConfig));
    app_config = &zero_config;

    BerylliumConfig config = beryllium_create_config();

    // Test that function uses zero values from app_config (not defaults)
    // The function uses: app_config ? app_config->field : default_value
    // So when app_config exists but fields are 0, it uses the 0 values
    TEST_ASSERT_TRUE(config.acceleration == 0.0);
    TEST_ASSERT_TRUE(config.z_acceleration == 0.0);
    TEST_ASSERT_TRUE(config.extruder_acceleration == 0.0);
    TEST_ASSERT_TRUE(config.max_speed_xy == 0.0);
    TEST_ASSERT_TRUE(config.max_speed_travel == 0.0);
    TEST_ASSERT_TRUE(config.max_speed_z == 0.0);
    // Hardcoded values should still be set correctly
    TEST_ASSERT_TRUE(config.default_feedrate == 3000.0);
    TEST_ASSERT_TRUE(config.filament_diameter == 1.75);
    TEST_ASSERT_TRUE(config.filament_density == 1.24);
    (void)config; // Suppress unused variable warning

    // Restore original app_config
    app_config = original_app_config;
}

//=============================================================================
// Motion Parameter Tests
//=============================================================================

void test_beryllium_create_config_motion_parameters(void) {
    // Initialize test AppConfig with base values
    memset(&test_app_config, 0, sizeof(AppConfig));
    test_app_config.print.motion.max_speed_xy = 150.0;
    test_app_config.print.motion.max_speed_travel = 200.0;
    test_app_config.print.motion.max_speed_z = 30.0;

    // Set specific motion parameters
    test_app_config.print.motion.acceleration = 750.0;
    test_app_config.print.motion.z_acceleration = 150.0;
    test_app_config.print.motion.e_acceleration = 400.0;
    app_config = &test_app_config;

    BerylliumConfig config = beryllium_create_config();

    // Test motion parameters are extracted correctly
    TEST_ASSERT_TRUE(config.acceleration == 750.0);
    TEST_ASSERT_TRUE(config.z_acceleration == 150.0);
    TEST_ASSERT_TRUE(config.extruder_acceleration == 400.0);
    (void)config; // Suppress unused variable warning
}

void test_beryllium_create_config_speed_parameters(void) {
    // Initialize test AppConfig with base values
    memset(&test_app_config, 0, sizeof(AppConfig));
    test_app_config.print.motion.acceleration = 1000.0;
    test_app_config.print.motion.z_acceleration = 200.0;
    test_app_config.print.motion.e_acceleration = 300.0;

    // Set specific speed parameters
    test_app_config.print.motion.max_speed_xy = 120.0;
    test_app_config.print.motion.max_speed_travel = 180.0;
    test_app_config.print.motion.max_speed_z = 25.0;
    app_config = &test_app_config;

    BerylliumConfig config = beryllium_create_config();

    // Test speed parameters are extracted correctly
    TEST_ASSERT_TRUE(config.max_speed_xy == 120.0);
    TEST_ASSERT_TRUE(config.max_speed_travel == 180.0);
    TEST_ASSERT_TRUE(config.max_speed_z == 25.0);
    (void)config; // Suppress unused variable warning
}

void test_beryllium_create_config_filament_parameters(void) {
    // Initialize test AppConfig with some values
    memset(&test_app_config, 0, sizeof(AppConfig));
    test_app_config.print.motion.acceleration = 1000.0;
    test_app_config.print.motion.z_acceleration = 200.0;
    test_app_config.print.motion.e_acceleration = 300.0;
    test_app_config.print.motion.max_speed_xy = 150.0;
    test_app_config.print.motion.max_speed_travel = 200.0;
    test_app_config.print.motion.max_speed_z = 30.0;

    // Test that filament parameters are always set to hardcoded values
    app_config = &test_app_config;

    BerylliumConfig config = beryllium_create_config();

    // Filament parameters should always be the hardcoded values
    TEST_ASSERT_TRUE(config.filament_diameter == 1.75);
    TEST_ASSERT_TRUE(config.filament_density == 1.24);
    (void)config; // Suppress unused variable warning
}

//=============================================================================
// Comprehensive Configuration Tests
//=============================================================================

void test_beryllium_create_config_all_parameters(void) {
    // Set all parameters to non-default values
    test_app_config.print.motion.acceleration = 800.0;
    test_app_config.print.motion.z_acceleration = 120.0;
    test_app_config.print.motion.e_acceleration = 350.0;
    test_app_config.print.motion.max_speed_xy = 140.0;
    test_app_config.print.motion.max_speed_travel = 160.0;
    test_app_config.print.motion.max_speed_z = 35.0;
    app_config = &test_app_config;

    BerylliumConfig config = beryllium_create_config();

    // Test that all parameters are set correctly
    TEST_ASSERT_TRUE(config.acceleration == 800.0);
    TEST_ASSERT_TRUE(config.z_acceleration == 120.0);
    TEST_ASSERT_TRUE(config.extruder_acceleration == 350.0);
    TEST_ASSERT_TRUE(config.max_speed_xy == 140.0);
    TEST_ASSERT_TRUE(config.max_speed_travel == 160.0);
    TEST_ASSERT_TRUE(config.max_speed_z == 35.0);
    TEST_ASSERT_TRUE(config.default_feedrate == 3000.0);
    TEST_ASSERT_TRUE(config.filament_diameter == 1.75);
    TEST_ASSERT_TRUE(config.filament_density == 1.24);
    (void)config; // Suppress unused variable warning
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic configuration creation tests
    RUN_TEST(test_beryllium_create_config_with_null_app_config);
    RUN_TEST(test_beryllium_create_config_with_valid_app_config);
    RUN_TEST(test_beryllium_create_config_default_values);

    // Motion parameter tests
    RUN_TEST(test_beryllium_create_config_motion_parameters);
    RUN_TEST(test_beryllium_create_config_speed_parameters);
    RUN_TEST(test_beryllium_create_config_filament_parameters);

    // Comprehensive configuration tests
    RUN_TEST(test_beryllium_create_config_all_parameters);

    return UNITY_END();
}
