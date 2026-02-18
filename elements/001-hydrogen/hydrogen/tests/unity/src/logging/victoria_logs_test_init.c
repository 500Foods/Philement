/*
 * Unity Test File: init_victoria_logs Tests
 * This file contains unit tests for the init_victoria_logs() function
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
void test_init_victoria_logs_no_url(void);
void test_init_victoria_logs_empty_url(void);
void test_init_victoria_logs_invalid_url(void);
void test_init_victoria_logs_valid_http_url(void);
void test_init_victoria_logs_valid_https_url(void);
void test_init_victoria_logs_with_log_level(void);
void test_init_victoria_logs_default_level(void);
void test_init_victoria_logs_k8s_namespace(void);
void test_init_victoria_logs_k8s_namespace_default(void);
void test_init_victoria_logs_k8s_pod_name(void);
void test_init_victoria_logs_k8s_container_name(void);
void test_init_victoria_logs_k8s_container_name_default(void);
void test_init_victoria_logs_k8s_node_name(void);
void test_init_victoria_logs_thread_state(void);
void test_init_victoria_logs_url_with_port(void);
void test_init_victoria_logs_custom_path(void);
void test_init_victoria_logs_host_equals_node(void);

// Test fixtures
void setUp(void) {
    // Clean up any existing state before each test
    if (victoria_logs_config.enabled) {
        cleanup_victoria_logs();
    }
    memset(&victoria_logs_config, 0, sizeof(victoria_logs_config));
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));
}

void tearDown(void) {
    // Clean up after each test
    if (victoria_logs_config.enabled) {
        cleanup_victoria_logs();
    }
}

// Test when VICTORIALOGS_URL is not set (should disable silently)
void test_init_victoria_logs_no_url(void) {
    // Ensure VICTORIALOGS_URL is not set
    unsetenv("VICTORIALOGS_URL");
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(victoria_logs_config.enabled);
}

// Test when VICTORIALOGS_URL is empty (should disable silently)
void test_init_victoria_logs_empty_url(void) {
    setenv("VICTORIALOGS_URL", "", 1);
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(victoria_logs_config.enabled);
    
    unsetenv("VICTORIALOGS_URL");
}

// Test with invalid URL format (very long hostname should fail)
void test_init_victoria_logs_invalid_url(void) {
    // Create a URL with hostname > 256 chars which will fail parsing
    char long_url[300];
    strcpy(long_url, "http://");
    for (int i = 0; i < 260; i++) {
        long_url[7 + i] = 'a';
    }
    long_url[267] = '\0';
    
    setenv("VICTORIALOGS_URL", long_url, 1);
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(victoria_logs_config.enabled);
    
    unsetenv("VICTORIALOGS_URL");
}

// Test with valid HTTP URL
void test_init_victoria_logs_valid_http_url(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("VICTORIALOGS_LVL");
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(victoria_logs_config.enabled);
    TEST_ASSERT_NOT_NULL(victoria_logs_config.url);
    TEST_ASSERT_EQUAL_STRING("http://localhost:9428/insert/jsonline", victoria_logs_config.url);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test with valid HTTPS URL
void test_init_victoria_logs_valid_https_url(void) {
    setenv("VICTORIALOGS_URL", "https://logs.example.com/insert/jsonline", 1);
    unsetenv("VICTORIALOGS_LVL");
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(victoria_logs_config.enabled);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test log level parsing from environment
void test_init_victoria_logs_with_log_level(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    setenv("VICTORIALOGS_LVL", "ERROR", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(victoria_logs_config.enabled);
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, victoria_logs_config.min_level);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
    unsetenv("VICTORIALOGS_LVL");
}

// Test log level default when not specified
void test_init_victoria_logs_default_level(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("VICTORIALOGS_LVL");
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(victoria_logs_config.enabled);
    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, victoria_logs_config.min_level);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test Kubernetes namespace from environment
void test_init_victoria_logs_k8s_namespace(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    setenv("K8S_NAMESPACE", "test-namespace", 1);
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(victoria_logs_config.k8s_namespace);
    TEST_ASSERT_EQUAL_STRING("test-namespace", victoria_logs_config.k8s_namespace);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
    unsetenv("K8S_NAMESPACE");
}

// Test Kubernetes namespace default
void test_init_victoria_logs_k8s_namespace_default(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(victoria_logs_config.k8s_namespace);
    TEST_ASSERT_EQUAL_STRING("local", victoria_logs_config.k8s_namespace);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test Kubernetes pod name from environment
void test_init_victoria_logs_k8s_pod_name(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    setenv("K8S_POD_NAME", "my-pod-123", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(victoria_logs_config.k8s_pod_name);
    TEST_ASSERT_EQUAL_STRING("my-pod-123", victoria_logs_config.k8s_pod_name);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
    unsetenv("K8S_POD_NAME");
}

// Test Kubernetes container name from environment
void test_init_victoria_logs_k8s_container_name(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    setenv("K8S_CONTAINER_NAME", "my-container", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(victoria_logs_config.k8s_container_name);
    TEST_ASSERT_EQUAL_STRING("my-container", victoria_logs_config.k8s_container_name);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
    unsetenv("K8S_CONTAINER_NAME");
}

// Test Kubernetes container name default
void test_init_victoria_logs_k8s_container_name_default(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(victoria_logs_config.k8s_container_name);
    TEST_ASSERT_EQUAL_STRING("hydrogen", victoria_logs_config.k8s_container_name);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test Kubernetes node name from environment
void test_init_victoria_logs_k8s_node_name(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    setenv("K8S_NODE_NAME", "worker-node-1", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(victoria_logs_config.k8s_node_name);
    TEST_ASSERT_EQUAL_STRING("worker-node-1", victoria_logs_config.k8s_node_name);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
    unsetenv("K8S_NODE_NAME");
}

// Test thread state initialization
void test_init_victoria_logs_thread_state(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(victoria_logs_thread.running);
    TEST_ASSERT_FALSE(victoria_logs_thread.shutdown);
    TEST_ASSERT_NOT_NULL(victoria_logs_thread.batch_buffer);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test URL with port
void test_init_victoria_logs_url_with_port(void) {
    setenv("VICTORIALOGS_URL", "http://victoria:9428/insert/jsonline", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(victoria_logs_config.enabled);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test URL with custom path
void test_init_victoria_logs_custom_path(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/custom/path/here", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    unsetenv("K8S_NODE_NAME");
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(victoria_logs_config.enabled);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
}

// Test host is set to node name
void test_init_victoria_logs_host_equals_node(void) {
    setenv("VICTORIALOGS_URL", "http://localhost:9428/insert/jsonline", 1);
    setenv("K8S_NODE_NAME", "test-node", 1);
    unsetenv("K8S_NAMESPACE");
    unsetenv("K8S_POD_NAME");
    unsetenv("K8S_CONTAINER_NAME");
    
    bool result = init_victoria_logs();
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(victoria_logs_config.host);
    TEST_ASSERT_EQUAL_STRING(victoria_logs_config.k8s_node_name, victoria_logs_config.host);
    
    cleanup_victoria_logs();
    unsetenv("VICTORIALOGS_URL");
    unsetenv("K8S_NODE_NAME");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_init_victoria_logs_no_url);
    RUN_TEST(test_init_victoria_logs_empty_url);
    RUN_TEST(test_init_victoria_logs_invalid_url);
    RUN_TEST(test_init_victoria_logs_valid_http_url);
    RUN_TEST(test_init_victoria_logs_valid_https_url);
    RUN_TEST(test_init_victoria_logs_with_log_level);
    RUN_TEST(test_init_victoria_logs_default_level);
    RUN_TEST(test_init_victoria_logs_k8s_namespace);
    RUN_TEST(test_init_victoria_logs_k8s_namespace_default);
    RUN_TEST(test_init_victoria_logs_k8s_pod_name);
    RUN_TEST(test_init_victoria_logs_k8s_container_name);
    RUN_TEST(test_init_victoria_logs_k8s_container_name_default);
    RUN_TEST(test_init_victoria_logs_k8s_node_name);
    RUN_TEST(test_init_victoria_logs_thread_state);
    RUN_TEST(test_init_victoria_logs_url_with_port);
    RUN_TEST(test_init_victoria_logs_custom_path);
    RUN_TEST(test_init_victoria_logs_host_equals_node);

    return UNITY_END();
}
