/*
 * Unity Test File: cleanup_victoria_logs Tests
 * This file contains unit tests for the cleanup_victoria_logs() function
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
void test_cleanup_victoria_logs_not_initialized(void);
void test_cleanup_victoria_logs_after_init(void);
void test_cleanup_victoria_logs_double_cleanup(void);
void test_cleanup_victoria_logs_with_k8s_metadata(void);

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

// Test cleanup when not initialized (should not crash)
void test_cleanup_victoria_logs_not_initialized(void) {
    // Should not crash even if never initialized
    cleanup_victoria_logs();
    
    // Verify state is still zeroed/cleared
    TEST_ASSERT_FALSE(victoria_logs_config.enabled);
    TEST_ASSERT_NULL(victoria_logs_config.url);
}

// Test cleanup after successful initialization
void test_cleanup_victoria_logs_after_init(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(victoria_logs_config.enabled);
    
    cleanup_victoria_logs();
    
    // Verify state is cleared
    TEST_ASSERT_FALSE(victoria_logs_config.enabled);
    TEST_ASSERT_NULL(victoria_logs_config.url);
    
    unsetenv("VICTORIALOGS_URL");
}

// Test double cleanup (should not crash)
void test_cleanup_victoria_logs_double_cleanup(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    TEST_ASSERT_TRUE(result);
    
    cleanup_victoria_logs();
    cleanup_victoria_logs();  // Second cleanup should not crash
    
    TEST_ASSERT_FALSE(victoria_logs_config.enabled);
    
    unsetenv("VICTORIALOGS_URL");
}

// Test cleanup with all K8s metadata set
void test_cleanup_victoria_logs_with_k8s_metadata(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    setenv("K8S_NAMESPACE", "test-ns", 1);
    setenv("K8S_POD_NAME", "test-pod", 1);
    setenv("K8S_CONTAINER_NAME", "test-container", 1);
    setenv("K8S_NODE_NAME", "test-node", 1);
    
    bool result = init_victoria_logs();
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(victoria_logs_config.k8s_namespace);
    TEST_ASSERT_NOT_NULL(victoria_logs_config.k8s_pod_name);
    TEST_ASSERT_NOT_NULL(victoria_logs_config.k8s_container_name);
    TEST_ASSERT_NOT_NULL(victoria_logs_config.k8s_node_name);
    TEST_ASSERT_NOT_NULL(victoria_logs_config.host);
    
    cleanup_victoria_logs();
    
    // All strings should be freed and set to NULL
    TEST_ASSERT_NULL(victoria_logs_config.k8s_namespace);
    TEST_ASSERT_NULL(victoria_logs_config.k8s_pod_name);
    TEST_ASSERT_NULL(victoria_logs_config.k8s_container_name);
    TEST_ASSERT_NULL(victoria_logs_config.k8s_node_name);
    TEST_ASSERT_NULL(victoria_logs_config.host);
    
    unsetenv("VICTORIALOGS_URL");
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_cleanup_victoria_logs_not_initialized);
    RUN_TEST(test_cleanup_victoria_logs_after_init);
    RUN_TEST(test_cleanup_victoria_logs_double_cleanup);
    RUN_TEST(test_cleanup_victoria_logs_with_k8s_metadata);

    return UNITY_END();
}
