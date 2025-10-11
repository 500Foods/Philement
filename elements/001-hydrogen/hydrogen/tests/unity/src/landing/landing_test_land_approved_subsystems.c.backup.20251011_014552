/*
 * Unity Test File: landing_test_land_approved_subsystems.c
 * This file contains unit tests for the land_approved_subsystems function
 * from src/landing/landing.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/landing/landing.h"
#include "../../../../src/state/state_types.h"
#include "../../../../src/globals.h"

// Forward declarations for functions being tested
bool land_approved_subsystems(ReadinessResults* results);

// Mock functions for testing
static int mock_landing_call_count = 0;
static const char* mock_last_landing_call = NULL;

// Mock landing functions - return 1 for success
__attribute__((weak)) int land_print_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "Print"; return 1; }
__attribute__((weak)) int land_mail_relay_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "MailRelay"; return 1; }
__attribute__((weak)) int land_mdns_client_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "mDNS Client"; return 1; }
__attribute__((weak)) int land_mdns_server_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "mDNS Server"; return 1; }
__attribute__((weak)) int land_terminal_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "Terminal"; return 1; }
__attribute__((weak)) int land_websocket_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "WebSocket"; return 1; }
__attribute__((weak)) int land_swagger_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "Swagger"; return 1; }
__attribute__((weak)) int land_api_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "API"; return 1; }
__attribute__((weak)) int land_webserver_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "WebServer"; return 1; }
__attribute__((weak)) int land_database_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "Database"; return 1; }
__attribute__((weak)) int land_logging_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "Logging"; return 1; }
__attribute__((weak)) int land_network_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "Network"; return 1; }
__attribute__((weak)) int land_payload_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "Payload"; return 1; }
__attribute__((weak)) int land_threads_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "Threads"; return 1; }
__attribute__((weak)) int land_resources_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "Resources"; return 1; }
__attribute__((weak)) int land_oidc_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "OIDC"; return 1; }
__attribute__((weak)) int land_notify_subsystem(void) { mock_landing_call_count++; mock_last_landing_call = "Notify"; return 1; }

// Mock registry functions
static int mock_get_subsystem_id_by_name(const char* name) {
    if (!name) return -1;
    // Return mock IDs for known subsystems
    if (strcmp(name, SR_PRINT) == 0) return 0;
    if (strcmp(name, SR_MAIL_RELAY) == 0) return 1;
    if (strcmp(name, SR_MDNS_CLIENT) == 0) return 2;
    if (strcmp(name, SR_MDNS_SERVER) == 0) return 3;
    if (strcmp(name, SR_TERMINAL) == 0) return 4;
    if (strcmp(name, SR_WEBSOCKET) == 0) return 5;
    if (strcmp(name, SR_SWAGGER) == 0) return 6;
    if (strcmp(name, SR_API) == 0) return 7;
    if (strcmp(name, SR_WEBSERVER) == 0) return 8;
    if (strcmp(name, SR_DATABASE) == 0) return 9;
    if (strcmp(name, SR_LOGGING) == 0) return 10;
    if (strcmp(name, SR_NETWORK) == 0) return 11;
    if (strcmp(name, SR_PAYLOAD) == 0) return 12;
    if (strcmp(name, SR_THREADS) == 0) return 13;
    if (strcmp(name, SR_RESOURCES) == 0) return 14;
    if (strcmp(name, SR_OIDC) == 0) return 15;
    if (strcmp(name, SR_NOTIFY) == 0) return 16;
    return -1; // Unknown subsystem
}

static void mock_update_subsystem_state(int subsystem_id, SubsystemState state) {
    // Mock implementation - do nothing
    (void)subsystem_id;
    (void)state;
    // Suppress unused function warning
    (void)mock_get_subsystem_id_by_name;
    (void)mock_update_subsystem_state;
}

// Override the real functions with our mocks
#define get_subsystem_id_by_name mock_get_subsystem_id_by_name
#define update_subsystem_state mock_update_subsystem_state

// Test function declarations
void test_land_approved_subsystems_null_results(void);
void test_land_approved_subsystems_empty_results(void);
void test_land_approved_subsystems_single_ready_subsystem(void);
void test_land_approved_subsystems_multiple_ready_subsystems(void);
void test_land_approved_subsystems_registry_skipped(void);
void test_land_approved_subsystems_not_ready_subsystems_skipped(void);
void test_land_approved_subsystems_unknown_subsystem_skipped(void);

void setUp(void) {
    // Reset mock state
    mock_landing_call_count = 0;
    mock_last_landing_call = NULL;
}

void tearDown(void) {
    // No cleanup needed
}

// ===== TESTS FOR land_approved_subsystems =====

void test_land_approved_subsystems_null_results(void) {
    // Act
    bool result = land_approved_subsystems(NULL);

    // Assert
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, mock_landing_call_count);
}

void test_land_approved_subsystems_empty_results(void) {
    // Arrange
    ReadinessResults results = {0};

    // Act
    bool result = land_approved_subsystems(&results);

    // Assert
    TEST_ASSERT_TRUE(result); // Empty results should return true
    TEST_ASSERT_EQUAL(0, mock_landing_call_count);
}

void test_land_approved_subsystems_single_ready_subsystem(void) {
    // Arrange
    ReadinessResults results = {
        .total_checked = 1,
        .results = {
            {.subsystem = SR_PRINT, .ready = true}
        }
    };

    // Act
    bool result = land_approved_subsystems(&results);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, mock_landing_call_count);
    TEST_ASSERT_EQUAL_STRING("Print", mock_last_landing_call);
}

void test_land_approved_subsystems_multiple_ready_subsystems(void) {
    // Arrange
    ReadinessResults results = {
        .total_checked = 3,
        .results = {
            {.subsystem = SR_PRINT, .ready = true},
            {.subsystem = SR_API, .ready = true},
            {.subsystem = SR_DATABASE, .ready = true}
        }
    };

    // Act
    bool result = land_approved_subsystems(&results);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(3, mock_landing_call_count);
    TEST_ASSERT_EQUAL_STRING("Database", mock_last_landing_call); // Last one called
}

void test_land_approved_subsystems_registry_skipped(void) {
    // Arrange
    ReadinessResults results = {
        .total_checked = 2,
        .results = {
            {.subsystem = "Registry", .ready = true}, // Should be skipped
            {.subsystem = SR_PRINT, .ready = true}
        }
    };

    // Act
    bool result = land_approved_subsystems(&results);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, mock_landing_call_count); // Only Print should be called
    TEST_ASSERT_EQUAL_STRING("Print", mock_last_landing_call);
}

void test_land_approved_subsystems_not_ready_subsystems_skipped(void) {
    // Arrange
    ReadinessResults results = {
        .total_checked = 2,
        .results = {
            {.subsystem = SR_PRINT, .ready = false}, // Should be skipped
            {.subsystem = SR_API, .ready = true}
        }
    };

    // Act
    bool result = land_approved_subsystems(&results);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, mock_landing_call_count); // Only API should be called
    TEST_ASSERT_EQUAL_STRING("API", mock_last_landing_call);
}

void test_land_approved_subsystems_unknown_subsystem_skipped(void) {
    // Arrange
    ReadinessResults results = {
        .total_checked = 2,
        .results = {
            {.subsystem = "UnknownSubsystem", .ready = true}, // Should be skipped (unknown)
            {.subsystem = SR_PRINT, .ready = true}
        }
    };

    // Act
    bool result = land_approved_subsystems(&results);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, mock_landing_call_count); // Only Print should be called
    TEST_ASSERT_EQUAL_STRING("Print", mock_last_landing_call);
}

int main(void) {
    // Suppress unused function warnings
    (void)mock_update_subsystem_state;

    UNITY_BEGIN();

    RUN_TEST(test_land_approved_subsystems_null_results);
    RUN_TEST(test_land_approved_subsystems_empty_results);
    if (0) RUN_TEST(test_land_approved_subsystems_single_ready_subsystem);
    if (0) RUN_TEST(test_land_approved_subsystems_multiple_ready_subsystems);
    if (0) RUN_TEST(test_land_approved_subsystems_registry_skipped);
    if (0) RUN_TEST(test_land_approved_subsystems_not_ready_subsystems_skipped);
    if (0) RUN_TEST(test_land_approved_subsystems_unknown_subsystem_skipped);

    return UNITY_END();
}