/*
 * Unity Test File: PostgreSQL Interface Functions
 * This file contains unit tests for PostgreSQL interface functions
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../../src/database/database.h"
#include "../../../../../src/database/postgresql/interface.h"

// Forward declarations for functions being tested
DatabaseEngineInterface* postgresql_get_interface(void);

// Function prototypes for test functions
void test_postgresql_get_interface_not_null(void);
void test_postgresql_get_interface_valid_structure(void);
void test_postgresql_get_interface_function_pointers(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test postgresql_get_interface
void test_postgresql_get_interface_not_null(void) {
    DatabaseEngineInterface* interface = postgresql_get_interface();
    TEST_ASSERT_NOT_NULL(interface);
}

void test_postgresql_get_interface_valid_structure(void) {
    DatabaseEngineInterface* interface = postgresql_get_interface();
    TEST_ASSERT_NOT_NULL(interface);
    TEST_ASSERT_EQUAL(DB_ENGINE_POSTGRESQL, interface->engine_type);
    TEST_ASSERT_NOT_NULL(interface->name);
    TEST_ASSERT_EQUAL_STRING("postgresql", interface->name);
}

void test_postgresql_get_interface_function_pointers(void) {
    DatabaseEngineInterface* interface = postgresql_get_interface();
    TEST_ASSERT_NOT_NULL(interface);

    // Test that essential function pointers are not null
    TEST_ASSERT_NOT_NULL(interface->connect);
    TEST_ASSERT_NOT_NULL(interface->disconnect);
    TEST_ASSERT_NOT_NULL(interface->execute_query);
    TEST_ASSERT_NOT_NULL(interface->begin_transaction);
    TEST_ASSERT_NOT_NULL(interface->commit_transaction);
    TEST_ASSERT_NOT_NULL(interface->rollback_transaction);
    TEST_ASSERT_NOT_NULL(interface->get_connection_string);
    TEST_ASSERT_NOT_NULL(interface->validate_connection_string);
}

int main(void) {
    UNITY_BEGIN();

    // Test postgresql_get_interface
    RUN_TEST(test_postgresql_get_interface_not_null);
    RUN_TEST(test_postgresql_get_interface_valid_structure);
    RUN_TEST(test_postgresql_get_interface_function_pointers);

    return UNITY_END();
}