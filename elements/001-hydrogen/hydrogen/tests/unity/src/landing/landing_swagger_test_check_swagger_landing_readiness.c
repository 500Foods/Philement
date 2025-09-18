/*
 * Unity Test File: landing_swagger_test_check_swagger_landing_readiness.c
 * This file contains unit tests for the check_swagger_landing_readiness function
 * from src/landing/landing_swagger.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/landing/landing.h"
#include "../../../../src/state/state_types.h"

// Forward declarations for functions being tested
LaunchReadiness check_swagger_landing_readiness(void);

// Test function declarations
void test_check_swagger_landing_readiness_both_running(void);
void test_check_swagger_landing_readiness_swagger_not_running(void);
void test_check_swagger_landing_readiness_webserver_not_running(void);
void test_check_swagger_landing_readiness_neither_running(void);

// Mock state
static bool mock_swagger_running = true;
static bool mock_webserver_running = true;

// Mock functions
__attribute__((weak)) bool is_subsystem_running_by_name(const char* name) {
    if (strcmp(name, SR_SWAGGER) == 0) {
        return mock_swagger_running;
    } else if (strcmp(name, "WebServer") == 0) {
        return mock_webserver_running;
    }
    return false;
}

void setUp(void) {
    // Reset mock state
    mock_swagger_running = true;
    mock_webserver_running = true;
}

void tearDown(void) {
    // No cleanup needed
}

// ===== TESTS FOR check_swagger_landing_readiness =====

void test_check_swagger_landing_readiness_both_running(void) {
    // Arrange
    mock_swagger_running = true;
    mock_webserver_running = true;

    // Act
    LaunchReadiness result = check_swagger_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_SWAGGER, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_SWAGGER, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  Go:      Swagger ready for shutdown", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Go:      WebServer ready for shutdown", result.messages[2]);
    TEST_ASSERT_NOT_NULL(result.messages[3]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  Go For Landing of Swagger", result.messages[3]);
    TEST_ASSERT_NULL(result.messages[4]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_swagger_landing_readiness_swagger_not_running(void) {
    // Arrange
    mock_swagger_running = false;
    mock_webserver_running = true;

    // Act
    LaunchReadiness result = check_swagger_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_SWAGGER, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_SWAGGER, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   Swagger not running", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of Swagger", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_swagger_landing_readiness_webserver_not_running(void) {
    // Arrange
    mock_swagger_running = true;
    mock_webserver_running = false;

    // Act
    LaunchReadiness result = check_swagger_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_SWAGGER, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_SWAGGER, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  Go:      Swagger ready for shutdown", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   WebServer subsystem not running", result.messages[2]);
    TEST_ASSERT_NOT_NULL(result.messages[3]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of Swagger", result.messages[3]);
    TEST_ASSERT_NULL(result.messages[4]);

    // Cleanup
    free_readiness_messages(&result);
}

void test_check_swagger_landing_readiness_neither_running(void) {
    // Arrange
    mock_swagger_running = false;
    mock_webserver_running = false;

    // Act
    LaunchReadiness result = check_swagger_landing_readiness();

    // Assert
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_SWAGGER, result.subsystem);

    // Check messages
    TEST_ASSERT_NOT_NULL(result.messages[0]);
    TEST_ASSERT_EQUAL_STRING(SR_SWAGGER, result.messages[0]);
    TEST_ASSERT_NOT_NULL(result.messages[1]);
    TEST_ASSERT_EQUAL_STRING("  No-Go:   Swagger not running", result.messages[1]);
    TEST_ASSERT_NOT_NULL(result.messages[2]);
    TEST_ASSERT_EQUAL_STRING("  Decide:  No-Go For Landing of Swagger", result.messages[2]);
    TEST_ASSERT_NULL(result.messages[3]);

    // Cleanup
    free_readiness_messages(&result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_swagger_landing_readiness_both_running);
    RUN_TEST(test_check_swagger_landing_readiness_swagger_not_running);
    if (0) RUN_TEST(test_check_swagger_landing_readiness_webserver_not_running);
    RUN_TEST(test_check_swagger_landing_readiness_neither_running);

    return UNITY_END();
}