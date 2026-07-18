/*
 * Unity Test File: Scripting Configuration Cleanup and Dump Tests
 * This file contains unit tests for cleanup_scripting_config and
 * dump_scripting_config from src/config/config_scripting.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config_scripting.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
void cleanup_scripting_config(ScriptingConfig* config);
void dump_scripting_config(const ScriptingConfig* config);

// Forward declarations for test functions
void test_cleanup_scripting_config_null_pointer(void);
void test_cleanup_scripting_config_empty_config(void);
void test_cleanup_scripting_config_with_data(void);
void test_cleanup_scripting_config_with_source_roots(void);
void test_dump_scripting_config_null_pointer(void);
void test_dump_scripting_config_basic(void);
void test_dump_scripting_config_with_source_roots(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_scripting_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully (line 87-88)
    cleanup_scripting_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_scripting_config_empty_config(void) {
    ScriptingConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_scripting_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_NULL(config.Orchestrator);
    TEST_ASSERT_NULL(config.DefaultDatabase);
    TEST_ASSERT_EQUAL(0, config.SourceRootCount);
}

void test_cleanup_scripting_config_with_data(void) {
    ScriptingConfig config = {0};

    // Initialize with some allocated strings
    config.Enabled = true;
    config.Orchestrator = strdup("Orchestrators.Orchestrator");
    config.DefaultDatabase = strdup("acuranzo");

    cleanup_scripting_config(&config);

    // Allocated strings should be freed and NULLed
    TEST_ASSERT_NULL(config.Orchestrator);
    TEST_ASSERT_NULL(config.DefaultDatabase);
    TEST_ASSERT_EQUAL(0, config.SourceRootCount);
    TEST_ASSERT_FALSE(config.Enabled);
}

void test_cleanup_scripting_config_with_source_roots(void) {
    ScriptingConfig config = {0};

    // Populate source roots with allocated paths
    config.SourceRoots[0] = strdup("/scripts/one");
    config.SourceRoots[1] = strdup("/scripts/two");
    config.SourceRoots[2] = NULL; // unused slot
    config.SourceRootCount = 3;

    cleanup_scripting_config(&config);

    // All populated source roots should be freed and zeroed
    TEST_ASSERT_NULL(config.SourceRoots[0]);
    TEST_ASSERT_NULL(config.SourceRoots[1]);
    TEST_ASSERT_NULL(config.SourceRoots[2]);
    TEST_ASSERT_EQUAL(0, config.SourceRootCount);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_scripting_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully (line 116-118)
    dump_scripting_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_scripting_config_basic(void) {
    ScriptingConfig config = {0};

    // Initialize with basic data
    config.Enabled = true;
    config.Orchestrator = strdup("Orchestrators.Orchestrator");
    config.WorkerCount = 4;
    config.DefaultDatabase = strdup("acuranzo");
    config.SourceRootCount = 0;

    // Dump should not crash and handle the data properly
    dump_scripting_config(&config);

    cleanup_scripting_config(&config);
}

void test_dump_scripting_config_with_source_roots(void) {
    ScriptingConfig config = {0};

    config.Enabled = true;
    config.Orchestrator = strdup("Orchestrators.Orchestrator");
    config.SourceRoots[0] = strdup("/scripts/one");
    config.SourceRoots[1] = strdup("/scripts/two");
    config.SourceRootCount = 2;

    // Dump should not crash and should traverse the source roots branch
    // (lines 142-146)
    dump_scripting_config(&config);

    cleanup_scripting_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Cleanup function tests
    RUN_TEST(test_cleanup_scripting_config_null_pointer);
    RUN_TEST(test_cleanup_scripting_config_empty_config);
    RUN_TEST(test_cleanup_scripting_config_with_data);
    RUN_TEST(test_cleanup_scripting_config_with_source_roots);

    // Dump function tests
    RUN_TEST(test_dump_scripting_config_null_pointer);
    RUN_TEST(test_dump_scripting_config_basic);
    RUN_TEST(test_dump_scripting_config_with_source_roots);

    return UNITY_END();
}
