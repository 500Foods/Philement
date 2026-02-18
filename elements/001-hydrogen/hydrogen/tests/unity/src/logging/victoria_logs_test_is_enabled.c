/*
 * Unity Test File: victoria_logs_is_enabled Tests
 * This file contains unit tests for the victoria_logs_is_enabled() function
 * from src/logging/victoria_logs.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/logging/victoria_logs.h>
#include <src/logging/logging.h>

// External declarations for global state
extern VictoriaLogsConfig victoria_logs_config;
extern VLThreadState victoria_logs_thread;

// Function prototypes for test functions
void test_victoria_logs_is_enabled_not_initialized(void);
void test_victoria_logs_is_enabled_fully_initialized(void);
void test_victoria_logs_is_enabled_after_cleanup(void);

// Test fixtures
void setUp(void) {
    // Clean up any existing state before each test
    if (victoria_logs_config.enabled && victoria_logs_thread.running) {
        cleanup_victoria_logs();
    }
    memset(&victoria_logs_config, 0, sizeof(victoria_logs_config));
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));
}

void tearDown(void) {
    // Clean up after each test
    if (victoria_logs_config.enabled && victoria_logs_thread.running) {
        cleanup_victoria_logs();
    }
}

// Test when not initialized (all zeros)
void test_victoria_logs_is_enabled_not_initialized(void) {
    bool result = victoria_logs_is_enabled();
    TEST_ASSERT_FALSE(result);
}

// Test when properly initialized via init_victoria_logs
void test_victoria_logs_is_enabled_fully_initialized(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool init_result = init_victoria_logs();
    TEST_ASSERT_TRUE(init_result);
    
    bool result = victoria_logs_is_enabled();
    TEST_ASSERT_TRUE(result);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test after cleanup
void test_victoria_logs_is_enabled_after_cleanup(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool init_result = init_victoria_logs();
    TEST_ASSERT_TRUE(init_result);
    TEST_ASSERT_TRUE(victoria_logs_is_enabled());
    
    cleanup_victoria_logs();
    
    bool result = victoria_logs_is_enabled();
    TEST_ASSERT_FALSE(result);
    
    unsetenv("VICTORIALOGS_URL");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_victoria_logs_is_enabled_not_initialized);
    RUN_TEST(test_victoria_logs_is_enabled_fully_initialized);
    RUN_TEST(test_victoria_logs_is_enabled_after_cleanup);

    return UNITY_END();
}
