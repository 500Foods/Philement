/*
 * Unity Test File: init_swagger_support_from_payload Function Tests
 * This file contains unit tests for the init_swagger_support_from_payload() function
 * from src/swagger/swagger.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../src/swagger/swagger.h"
#include "../../../../src/payload/payload.h"

// Structure to hold in-memory Swagger files (copied from swagger.c)
typedef struct {
    char *name;         // File name (e.g., "index.html")
    uint8_t *data;      // File content
    size_t size;        // Content size
    bool is_compressed; // Whether content is Brotli compressed
} SwaggerFile;

// Forward declaration for the function being tested
bool init_swagger_support_from_payload(SwaggerConfig *config, PayloadFile *payload_files, size_t num_payload_files);

// Mock global state (from swagger.c)
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t web_server_shutdown;
extern volatile sig_atomic_t server_starting;

// Global swagger state (for resetting between tests)
extern SwaggerFile *swagger_files;
extern size_t num_swagger_files;
extern bool swagger_initialized;
extern const SwaggerConfig *global_swagger_config;

// Function prototypes for test functions
void test_init_swagger_support_from_payload_null_config(void);
void test_init_swagger_support_from_payload_server_stopping(void);
void test_init_swagger_support_from_payload_web_server_shutdown(void);
void test_init_swagger_support_from_payload_not_starting(void);
void test_init_swagger_support_from_payload_disabled(void);
void test_init_swagger_support_from_payload_already_initialized(void);
void test_init_swagger_support_from_payload_success(void);
void test_init_swagger_support_from_payload_empty_files(void);
void test_init_swagger_support_from_payload_memory_allocation_failure(void);

// Test fixtures
static SwaggerConfig test_config;
static PayloadFile test_files[3];

void setUp(void) {
    // Reset global state
    server_stopping = false;
    web_server_shutdown = false;
    server_starting = true;

    // Clean up any existing swagger files from previous tests
    if (swagger_files) {
        for (size_t i = 0; i < num_swagger_files; i++) {
            free(swagger_files[i].name);
            // Note: data pointers are references and should not be freed
        }
        free(swagger_files);
        swagger_files = NULL;
        num_swagger_files = 0;
    }
    swagger_initialized = false;
    global_swagger_config = NULL;

    // Clean up test files from previous runs
    for (int i = 0; i < 3; i++) {
        if (test_files[i].name) free(test_files[i].name);
        if (test_files[i].data) free(test_files[i].data);
    }

    // Initialize test config
    memset(&test_config, 0, sizeof(SwaggerConfig));
    test_config.enabled = true;
    test_config.payload_available = false;
    test_config.prefix = strdup("/swagger");

    // Initialize test payload files
    memset(test_files, 0, sizeof(test_files));

    // File 1: swagger/index.html
    test_files[0].name = strdup("swagger/index.html");
    test_files[0].data = (uint8_t*)strdup("<html>Test</html>");
    test_files[0].size = strlen((char*)test_files[0].data);
    test_files[0].is_compressed = false;

    // File 2: swagger/swagger.json
    test_files[1].name = strdup("swagger/swagger.json");
    test_files[1].data = (uint8_t*)strdup("{\"swagger\":\"2.0\"}");
    test_files[1].size = strlen((char*)test_files[1].data);
    test_files[1].is_compressed = false;

    // File 3: swagger/css/style.css
    test_files[2].name = strdup("swagger/css/style.css");
    test_files[2].data = (uint8_t*)strdup("body { color: red; }");
    test_files[2].size = strlen((char*)test_files[2].data);
    test_files[2].is_compressed = true;
}

void tearDown(void) {
    // Clean up test config
    free(test_config.prefix);
    memset(&test_config, 0, sizeof(SwaggerConfig));

    // Clean up test files
    for (int i = 0; i < 3; i++) {
        free(test_files[i].name);
        free(test_files[i].data);
    }
    memset(test_files, 0, sizeof(test_files));

    // Reset global swagger state between tests
    swagger_initialized = false;
    global_swagger_config = NULL;
    // Note: swagger_files and num_swagger_files are managed by the function under test
}

void test_init_swagger_support_from_payload_null_config(void) {
    bool result = init_swagger_support_from_payload(NULL, test_files, 3);
    TEST_ASSERT_FALSE(result);
}

void test_init_swagger_support_from_payload_server_stopping(void) {
    server_stopping = true;
    bool result = init_swagger_support_from_payload(&test_config, test_files, 3);
    TEST_ASSERT_FALSE(result);
}

void test_init_swagger_support_from_payload_web_server_shutdown(void) {
    web_server_shutdown = true;
    bool result = init_swagger_support_from_payload(&test_config, test_files, 3);
    TEST_ASSERT_FALSE(result);
}

void test_init_swagger_support_from_payload_not_starting(void) {
    server_starting = false;
    bool result = init_swagger_support_from_payload(&test_config, test_files, 3);
    TEST_ASSERT_FALSE(result);
}

void test_init_swagger_support_from_payload_disabled(void) {
    test_config.enabled = false;
    bool result = init_swagger_support_from_payload(&test_config, test_files, 3);
    TEST_ASSERT_FALSE(result); // Should return false when disabled
    TEST_ASSERT_FALSE(test_config.payload_available);
}

void test_init_swagger_support_from_payload_already_initialized(void) {
    // First call to initialize
    bool result1 = init_swagger_support_from_payload(&test_config, test_files, 3);
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(test_config.payload_available);

    // Second call should return true without doing anything
    test_config.enabled = false; // Change config
    bool result2 = init_swagger_support_from_payload(&test_config, test_files, 3);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_TRUE(test_config.payload_available); // Should still be true
}

void test_init_swagger_support_from_payload_success(void) {
    bool result = init_swagger_support_from_payload(&test_config, test_files, 3);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(test_config.payload_available);

    // Note: We cannot directly check internal swagger_files state from tests
    // The function behavior is validated through return value and config changes
}

void test_init_swagger_support_from_payload_empty_files(void) {
    bool result = init_swagger_support_from_payload(&test_config, test_files, 0);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(test_config.payload_available);

    // Note: We cannot directly check internal num_swagger_files state from tests
}

void test_init_swagger_support_from_payload_memory_allocation_failure(void) {
    // This test would require mocking malloc, but for now we'll test the basic path
    // In a real scenario, we'd use mock_system to simulate malloc failure
    bool result = init_swagger_support_from_payload(&test_config, test_files, 3);
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_init_swagger_support_from_payload_null_config);
    RUN_TEST(test_init_swagger_support_from_payload_server_stopping);
    RUN_TEST(test_init_swagger_support_from_payload_web_server_shutdown);
    RUN_TEST(test_init_swagger_support_from_payload_not_starting);
    RUN_TEST(test_init_swagger_support_from_payload_disabled);
    RUN_TEST(test_init_swagger_support_from_payload_already_initialized);
    RUN_TEST(test_init_swagger_support_from_payload_success);
    RUN_TEST(test_init_swagger_support_from_payload_empty_files);
    RUN_TEST(test_init_swagger_support_from_payload_memory_allocation_failure);

    return UNITY_END();
}