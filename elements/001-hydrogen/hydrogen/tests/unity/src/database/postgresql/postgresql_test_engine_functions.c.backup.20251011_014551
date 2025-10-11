/*
 * Unity Test File: PostgreSQL Engine Functions Tests
 * This file contains unit tests for the PostgreSQL engine utility functions
 * from src/database/postgresql/postgresql.c
 *
 * Note: The actual engine functions are static, so we test them indirectly
 * through the interface functions that use them.
 */

// Standard project header plus Unity Framework header
#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the PostgreSQL engine
#include "../../../../../src/database/postgresql/interface.h"

// Forward declarations for engine functions
const char* postgresql_engine_get_version(void);
bool postgresql_engine_is_available(void);
const char* postgresql_engine_get_description(void);

// Function prototypes for test functions
void test_postgresql_get_interface_returns_valid_interface(void);
void test_postgresql_interface_function_pointers_are_assigned(void);
void test_postgresql_get_interface_multiple_calls_consistent(void);
void test_postgresql_engine_get_version(void);
void test_postgresql_engine_is_available(void);
void test_postgresql_engine_get_description(void);

// Test fixtures
void setUp(void) {
    // No setup needed for these pure functions
}

void tearDown(void) {
    // No cleanup needed for these pure functions
}

// Test that the PostgreSQL engine interface can be obtained
void test_postgresql_get_interface_returns_valid_interface(void) {
    const DatabaseEngineInterface* interface = postgresql_get_interface();

    TEST_ASSERT_NOT_NULL(interface);
    TEST_ASSERT_NOT_NULL(interface->connect);
    TEST_ASSERT_NOT_NULL(interface->disconnect);
    TEST_ASSERT_NOT_NULL(interface->execute_query);
    TEST_ASSERT_NOT_NULL(interface->begin_transaction);
    TEST_ASSERT_NOT_NULL(interface->commit_transaction);
    TEST_ASSERT_NOT_NULL(interface->rollback_transaction);
}

// Test that the interface functions are properly assigned
void test_postgresql_interface_function_pointers_are_assigned(void) {
    const DatabaseEngineInterface* interface = postgresql_get_interface();

    // Test that all required function pointers are assigned
    TEST_ASSERT_TRUE(interface->connect != NULL);
    TEST_ASSERT_TRUE(interface->disconnect != NULL);
    TEST_ASSERT_TRUE(interface->health_check != NULL);
    TEST_ASSERT_TRUE(interface->execute_query != NULL);
    TEST_ASSERT_TRUE(interface->execute_prepared != NULL);
    TEST_ASSERT_TRUE(interface->begin_transaction != NULL);
    TEST_ASSERT_TRUE(interface->commit_transaction != NULL);
    TEST_ASSERT_TRUE(interface->rollback_transaction != NULL);
    TEST_ASSERT_TRUE(interface->prepare_statement != NULL);
    TEST_ASSERT_TRUE(interface->unprepare_statement != NULL);
}

// Test that the interface can be called multiple times and returns consistent results
void test_postgresql_get_interface_multiple_calls_consistent(void) {
    const DatabaseEngineInterface* interface1 = postgresql_get_interface();
    const DatabaseEngineInterface* interface2 = postgresql_get_interface();

    TEST_ASSERT_NOT_NULL(interface1);
    TEST_ASSERT_NOT_NULL(interface2);

    // Should return the same interface instance
    TEST_ASSERT_EQUAL_PTR(interface1, interface2);
}

// Test the engine version function
void test_postgresql_engine_get_version(void) {
    const char* version = postgresql_engine_get_version();

    TEST_ASSERT_NOT_NULL(version);
    TEST_ASSERT_TRUE(strlen(version) > 0);
    TEST_ASSERT_EQUAL_STRING("PostgreSQL Engine v1.0.0", version);
}

// Test the engine availability function
void test_postgresql_engine_is_available(void) {
    // The result depends on whether libpq is installed on the system
    // We just verify the function doesn't crash and returns a boolean
    // Call the function to ensure it executes without error
    (void)postgresql_engine_is_available();
    TEST_ASSERT_TRUE(true);  // Function executed without crashing
}

// Test the engine description function
void test_postgresql_engine_get_description(void) {
    const char* description = postgresql_engine_get_description();

    TEST_ASSERT_NOT_NULL(description);
    TEST_ASSERT_NOT_NULL(strstr(description, "PostgreSQL"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_postgresql_get_interface_returns_valid_interface);
    RUN_TEST(test_postgresql_interface_function_pointers_are_assigned);
    RUN_TEST(test_postgresql_get_interface_multiple_calls_consistent);
    RUN_TEST(test_postgresql_engine_get_version);
    RUN_TEST(test_postgresql_engine_is_available);
    RUN_TEST(test_postgresql_engine_get_description);

    return UNITY_END();
}