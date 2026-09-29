/*
 * Unity Test File: MSSQL Interface Functions
 * This file contains unit tests for MSSQL interface functions
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/mssql/interface.h>

// Forward declarations for functions being tested
DatabaseEngineInterface* mssql_get_interface(void);

// Function prototypes for test functions
void test_mssql_get_interface_not_null(void);
void test_mssql_get_interface_valid_structure(void);
void test_mssql_get_interface_function_pointers(void);
void test_mssql_engine_get_version(void);
void test_mssql_engine_get_description(void);
void test_mssql_engine_is_available(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test mssql_get_interface
void test_mssql_get_interface_not_null(void) {
    DatabaseEngineInterface* interface = mssql_get_interface();
    TEST_ASSERT_NOT_NULL(interface);
}

void test_mssql_get_interface_valid_structure(void) {
    DatabaseEngineInterface* interface = mssql_get_interface();
    TEST_ASSERT_NOT_NULL(interface);
    TEST_ASSERT_EQUAL(DB_ENGINE_MSSQL, interface->engine_type);
    TEST_ASSERT_NOT_NULL(interface->name);
    TEST_ASSERT_EQUAL_STRING("mssql", interface->name);
}

void test_mssql_get_interface_function_pointers(void) {
    DatabaseEngineInterface* interface = mssql_get_interface();
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

void test_mssql_engine_get_version(void) {
    const char* version = mssql_engine_get_version();
    TEST_ASSERT_NOT_NULL(version);
}

void test_mssql_engine_get_description(void) {
    const char* description = mssql_engine_get_description();
    TEST_ASSERT_NOT_NULL(description);
}

void test_mssql_engine_is_available(void) {
    bool available = mssql_engine_is_available();
    TEST_ASSERT_TRUE(available);
}

int main(void) {
    UNITY_BEGIN();

    // Test mssql_get_interface
    RUN_TEST(test_mssql_get_interface_not_null);
    RUN_TEST(test_mssql_get_interface_valid_structure);
    RUN_TEST(test_mssql_get_interface_function_pointers);

    // Test engine info functions
    RUN_TEST(test_mssql_engine_get_version);
    RUN_TEST(test_mssql_engine_get_description);
    RUN_TEST(test_mssql_engine_is_available);

    return UNITY_END();
}
