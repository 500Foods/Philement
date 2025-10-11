/*
 * Unity Test File: landing_test_get_landing_function.c
 * This file contains unit tests for the get_landing_function from src/landing/landing.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>

// Forward declarations for functions being tested
LandingFunction get_landing_function(const char* subsystem_name);

// Test function declarations
void test_get_landing_function_registry_returns_null(void);
void test_get_landing_function_print_returns_function(void);
void test_get_landing_function_mail_relay_returns_function(void);
void test_get_landing_function_mdns_client_returns_function(void);
void test_get_landing_function_mdns_server_returns_function(void);
void test_get_landing_function_terminal_returns_function(void);
void test_get_landing_function_websocket_returns_function(void);
void test_get_landing_function_swagger_returns_function(void);
void test_get_landing_function_api_returns_function(void);
void test_get_landing_function_webserver_returns_function(void);
void test_get_landing_function_database_returns_function(void);
void test_get_landing_function_logging_returns_function(void);
void test_get_landing_function_network_returns_function(void);
void test_get_landing_function_payload_returns_function(void);
void test_get_landing_function_threads_returns_function(void);
void test_get_landing_function_resources_returns_function(void);
void test_get_landing_function_oidc_returns_function(void);
void test_get_landing_function_notify_returns_function(void);
void test_get_landing_function_unknown_returns_null(void);
void test_get_landing_function_null_input_returns_null(void);

void setUp(void) {
    // No setup needed for this function
}

void tearDown(void) {
    // No cleanup needed
}

// ===== TESTS FOR get_landing_function =====

void test_get_landing_function_registry_returns_null(void) {
    // Act
    LandingFunction result = get_landing_function("Registry");

    // Assert
    TEST_ASSERT_NULL(result);
}

void test_get_landing_function_print_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_PRINT);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_print_subsystem, result);
}

void test_get_landing_function_mail_relay_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_MAIL_RELAY);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_mail_relay_subsystem, result);
}

void test_get_landing_function_mdns_client_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_MDNS_CLIENT);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_mdns_client_subsystem, result);
}

void test_get_landing_function_mdns_server_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_MDNS_SERVER);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_mdns_server_subsystem, result);
}

void test_get_landing_function_terminal_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_TERMINAL);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_terminal_subsystem, result);
}

void test_get_landing_function_websocket_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_WEBSOCKET);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_websocket_subsystem, result);
}

void test_get_landing_function_swagger_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_SWAGGER);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_swagger_subsystem, result);
}

void test_get_landing_function_api_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_API);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_api_subsystem, result);
}

void test_get_landing_function_webserver_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_WEBSERVER);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_webserver_subsystem, result);
}

void test_get_landing_function_database_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_DATABASE);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_database_subsystem, result);
}

void test_get_landing_function_logging_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_LOGGING);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_logging_subsystem, result);
}

void test_get_landing_function_network_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_NETWORK);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_network_subsystem, result);
}

void test_get_landing_function_payload_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_PAYLOAD);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_payload_subsystem, result);
}

void test_get_landing_function_threads_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_THREADS);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_threads_subsystem, result);
}

void test_get_landing_function_resources_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_RESOURCES);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_resources_subsystem, result);
}

void test_get_landing_function_oidc_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_OIDC);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_oidc_subsystem, result);
}

void test_get_landing_function_notify_returns_function(void) {
    // Act
    LandingFunction result = get_landing_function(SR_NOTIFY);

    // Assert
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(land_notify_subsystem, result);
}

void test_get_landing_function_unknown_returns_null(void) {
    // Act
    LandingFunction result = get_landing_function("UnknownSubsystem");

    // Assert
    TEST_ASSERT_NULL(result);
}

void test_get_landing_function_null_input_returns_null(void) {
    // Act
    LandingFunction result = get_landing_function(NULL);

    // Assert
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_landing_function_registry_returns_null);
    RUN_TEST(test_get_landing_function_print_returns_function);
    RUN_TEST(test_get_landing_function_mail_relay_returns_function);
    RUN_TEST(test_get_landing_function_mdns_client_returns_function);
    RUN_TEST(test_get_landing_function_mdns_server_returns_function);
    RUN_TEST(test_get_landing_function_terminal_returns_function);
    RUN_TEST(test_get_landing_function_websocket_returns_function);
    RUN_TEST(test_get_landing_function_swagger_returns_function);
    RUN_TEST(test_get_landing_function_api_returns_function);
    RUN_TEST(test_get_landing_function_webserver_returns_function);
    RUN_TEST(test_get_landing_function_database_returns_function);
    RUN_TEST(test_get_landing_function_logging_returns_function);
    RUN_TEST(test_get_landing_function_network_returns_function);
    RUN_TEST(test_get_landing_function_payload_returns_function);
    RUN_TEST(test_get_landing_function_threads_returns_function);
    RUN_TEST(test_get_landing_function_resources_returns_function);
    RUN_TEST(test_get_landing_function_oidc_returns_function);
    RUN_TEST(test_get_landing_function_notify_returns_function);
    RUN_TEST(test_get_landing_function_unknown_returns_null);
    RUN_TEST(test_get_landing_function_null_input_returns_null);

    return UNITY_END();
}