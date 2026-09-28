/*
 * Unity Test File: Reporting Configuration Tests
 * This file contains unit tests for the load_reporting_config, dump_reporting_config,
 * and cleanup_reporting_config functions from src/config/config_reporting.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config_reporting.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
bool load_reporting_config(json_t* root, AppConfig* config);
void dump_reporting_config(const ReportingConfig* config);
void cleanup_reporting_config(ReportingConfig* config);

// Forward declarations for test functions
void test_load_reporting_config_null_root(void);
void test_load_reporting_config_empty_json(void);
void test_load_reporting_config_enabled(void);
void test_load_reporting_config_all_fields(void);
void test_load_reporting_config_max_image_size(void);
void test_load_reporting_config_max_input_bytes(void);
void test_load_reporting_config_default_dpi(void);
void test_load_reporting_config_allowed_formats(void);
void test_load_reporting_config_defaults_without_section(void);
void test_dump_reporting_config_null_pointer(void);
void test_dump_reporting_config_basic(void);
void test_dump_reporting_config_with_formats(void);
void test_cleanup_reporting_config_null_pointer(void);
void test_cleanup_reporting_config_empty_config(void);
void test_cleanup_reporting_config_with_data(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== LOAD CONFIG TESTS =====

void test_load_reporting_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_reporting_config(NULL, &config);

    // Should succeed with defaults when root is NULL
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.reporting.Enabled);
    TEST_ASSERT_EQUAL(8192, config.reporting.MaxImageSize);
    TEST_ASSERT_EQUAL(52428800, config.reporting.MaxInputBytes);
    TEST_ASSERT_EQUAL(52428800, config.reporting.MaxOutputBytes);
    TEST_ASSERT_EQUAL(72, config.reporting.DefaultDPI);
    TEST_ASSERT_NULL(config.reporting.AllowedFormats);

    cleanup_reporting_config(&config.reporting);
}

void test_load_reporting_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_reporting_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.reporting.Enabled);
    TEST_ASSERT_EQUAL(8192, config.reporting.MaxImageSize);
    TEST_ASSERT_EQUAL(52428800, config.reporting.MaxInputBytes);
    TEST_ASSERT_EQUAL(52428800, config.reporting.MaxOutputBytes);
    TEST_ASSERT_EQUAL(72, config.reporting.DefaultDPI);
    TEST_ASSERT_NULL(config.reporting.AllowedFormats);

    json_decref(root);
    cleanup_reporting_config(&config.reporting);
}

void test_load_reporting_config_enabled(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* reporting_section = json_object();

    json_object_set(reporting_section, "Enabled", json_true());
    json_object_set(root, "Reporting", reporting_section);

    bool result = load_reporting_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.reporting.Enabled);

    json_decref(root);
    cleanup_reporting_config(&config.reporting);
}

void test_load_reporting_config_all_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* reporting_section = json_object();

    json_object_set(reporting_section, "Enabled", json_true());
    json_object_set(reporting_section, "MaxImageSize", json_integer(4096));
    json_object_set(reporting_section, "MaxInputBytes", json_integer(10485760));
    json_object_set(reporting_section, "MaxOutputBytes", json_integer(20971520));
    json_object_set(reporting_section, "DefaultDPI", json_integer(150));
    json_object_set(reporting_section, "AllowedFormats", json_string("png,jpg,gif"));

    json_object_set(root, "Reporting", reporting_section);

    bool result = load_reporting_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.reporting.Enabled);
    TEST_ASSERT_EQUAL(4096, config.reporting.MaxImageSize);
    TEST_ASSERT_EQUAL(10485760, config.reporting.MaxInputBytes);
    TEST_ASSERT_EQUAL(20971520, config.reporting.MaxOutputBytes);
    TEST_ASSERT_EQUAL(150, config.reporting.DefaultDPI);
    TEST_ASSERT_NOT_NULL(config.reporting.AllowedFormats);
    TEST_ASSERT_EQUAL_STRING("png,jpg,gif", config.reporting.AllowedFormats);

    json_decref(root);
    cleanup_reporting_config(&config.reporting);
}

void test_load_reporting_config_max_image_size(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* reporting_section = json_object();

    json_object_set(reporting_section, "MaxImageSize", json_integer(2048));
    json_object_set(root, "Reporting", reporting_section);

    bool result = load_reporting_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2048, config.reporting.MaxImageSize);
    TEST_ASSERT_FALSE(config.reporting.Enabled);

    json_decref(root);
    cleanup_reporting_config(&config.reporting);
}

void test_load_reporting_config_max_input_bytes(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* reporting_section = json_object();

    json_object_set(reporting_section, "MaxInputBytes", json_integer(20971520));
    json_object_set(root, "Reporting", reporting_section);

    bool result = load_reporting_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(20971520, config.reporting.MaxInputBytes);

    json_decref(root);
    cleanup_reporting_config(&config.reporting);
}

void test_load_reporting_config_default_dpi(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* reporting_section = json_object();

    json_object_set(reporting_section, "DefaultDPI", json_integer(300));
    json_object_set(root, "Reporting", reporting_section);

    bool result = load_reporting_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(300, config.reporting.DefaultDPI);

    json_decref(root);
    cleanup_reporting_config(&config.reporting);
}

void test_load_reporting_config_allowed_formats(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* reporting_section = json_object();

    json_object_set(reporting_section, "AllowedFormats", json_string("webp,bmp,tiff"));
    json_object_set(root, "Reporting", reporting_section);

    bool result = load_reporting_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(config.reporting.AllowedFormats);
    TEST_ASSERT_EQUAL_STRING("webp,bmp,tiff", config.reporting.AllowedFormats);

    json_decref(root);
    cleanup_reporting_config(&config.reporting);
}

void test_load_reporting_config_defaults_without_section(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    // No Reporting section in JSON - should use defaults
    bool result = load_reporting_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.reporting.Enabled);
    TEST_ASSERT_EQUAL(8192, config.reporting.MaxImageSize);
    TEST_ASSERT_EQUAL(52428800, config.reporting.MaxInputBytes);
    TEST_ASSERT_EQUAL(52428800, config.reporting.MaxOutputBytes);
    TEST_ASSERT_EQUAL(72, config.reporting.DefaultDPI);
    TEST_ASSERT_NULL(config.reporting.AllowedFormats);

    json_decref(root);
    cleanup_reporting_config(&config.reporting);
}

// ===== DUMP TESTS =====

void test_dump_reporting_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_reporting_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_reporting_config_basic(void) {
    ReportingConfig config = {0};

    config.Enabled = true;
    config.MaxImageSize = 4096;
    config.MaxInputBytes = 10485760;
    config.MaxOutputBytes = 20971520;
    config.DefaultDPI = 150;
    config.AllowedFormats = NULL;

    // Dump should not crash
    dump_reporting_config(&config);
}

void test_dump_reporting_config_with_formats(void) {
    ReportingConfig config = {0};

    config.Enabled = false;
    config.MaxImageSize = 8192;
    config.MaxInputBytes = 52428800;
    config.MaxOutputBytes = 52428800;
    config.DefaultDPI = 72;
    config.AllowedFormats = strdup("png,jpg,gif");

    // Dump should not crash and handle the formats string
    dump_reporting_config(&config);

    free(config.AllowedFormats);
}

// ===== CLEANUP TESTS =====

void test_cleanup_reporting_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_reporting_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_reporting_config_empty_config(void) {
    ReportingConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_reporting_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.Enabled);
    TEST_ASSERT_EQUAL(0, config.MaxImageSize);
    TEST_ASSERT_EQUAL(0, config.MaxInputBytes);
    TEST_ASSERT_EQUAL(0, config.MaxOutputBytes);
    TEST_ASSERT_EQUAL(0, config.DefaultDPI);
    TEST_ASSERT_NULL(config.AllowedFormats);
}

void test_cleanup_reporting_config_with_data(void) {
    ReportingConfig config = {0};

    config.Enabled = true;
    config.MaxImageSize = 4096;
    config.MaxInputBytes = 10485760;
    config.MaxOutputBytes = 20971520;
    config.DefaultDPI = 150;
    config.AllowedFormats = strdup("png,jpg,gif");

    // Cleanup should free all allocated memory
    cleanup_reporting_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.Enabled);
    TEST_ASSERT_EQUAL(0, config.MaxImageSize);
    TEST_ASSERT_EQUAL(0, config.MaxInputBytes);
    TEST_ASSERT_EQUAL(0, config.MaxOutputBytes);
    TEST_ASSERT_EQUAL(0, config.DefaultDPI);
    TEST_ASSERT_NULL(config.AllowedFormats);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Load config tests
    RUN_TEST(test_load_reporting_config_null_root);
    RUN_TEST(test_load_reporting_config_empty_json);
    RUN_TEST(test_load_reporting_config_enabled);
    RUN_TEST(test_load_reporting_config_all_fields);
    RUN_TEST(test_load_reporting_config_max_image_size);
    RUN_TEST(test_load_reporting_config_max_input_bytes);
    RUN_TEST(test_load_reporting_config_default_dpi);
    RUN_TEST(test_load_reporting_config_allowed_formats);
    RUN_TEST(test_load_reporting_config_defaults_without_section);

    // Dump function tests
    RUN_TEST(test_dump_reporting_config_null_pointer);
    RUN_TEST(test_dump_reporting_config_basic);
    RUN_TEST(test_dump_reporting_config_with_formats);

    // Cleanup function tests
    RUN_TEST(test_cleanup_reporting_config_null_pointer);
    RUN_TEST(test_cleanup_reporting_config_empty_config);
    RUN_TEST(test_cleanup_reporting_config_with_data);

    return UNITY_END();
}
