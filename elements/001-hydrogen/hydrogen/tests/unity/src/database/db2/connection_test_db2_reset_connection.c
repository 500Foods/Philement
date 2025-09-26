/*
 * Unity Test File: DB2 Connection Reset Function
 * This file contains unit tests for db2_reset_connection functionality
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Enable mocks
#define USE_MOCK_SYSTEM
#include "../../../../../tests/unity/mocks/mock_libdb2.h"
#include "../../../../../tests/unity/mocks/mock_system.h"

// Include necessary headers for the module being tested
#include "../../../../../src/database/database.h"
#include "../../../../../src/database/db2/types.h"
#include "../../../../../src/database/db2/connection.h"

// Forward declarations for functions being tested
bool db2_reset_connection(DatabaseHandle* connection);

// Function prototypes for test functions
static void test_db2_reset_connection_null_connection(void);
static void test_db2_reset_connection_wrong_engine_type(void);
static void test_db2_reset_connection_success(void);

void setUp(void) {
    // Reset all mocks to default state
    mock_libdb2_reset_all();
    mock_system_reset_all();

    // Ensure libdb2 functions are loaded (mocked)
    load_libdb2_functions();
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test db2_reset_connection with NULL connection
void test_db2_reset_connection_null_connection(void) {
    bool result = db2_reset_connection(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test db2_reset_connection with wrong engine type
void test_db2_reset_connection_wrong_engine_type(void) {
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_POSTGRESQL, // Wrong type
        .connection_handle = NULL,
        .status = DB_CONNECTION_ERROR,
        .connected_since = 123456,
        .consecutive_failures = 5
    };
    bool result = db2_reset_connection(&connection);
    TEST_ASSERT_FALSE(result);
    // Status should not change
    TEST_ASSERT_EQUAL(DB_CONNECTION_ERROR, connection.status);
}

// Test db2_reset_connection success case
void test_db2_reset_connection_success(void) {
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = (void*)0x12345678, // Mock handle
        .status = DB_CONNECTION_ERROR,
        .connected_since = 123456,
        .consecutive_failures = 5
    };

    bool result = db2_reset_connection(&connection);
    TEST_ASSERT_TRUE(result);

    // Verify reset behavior
    TEST_ASSERT_EQUAL(DB_CONNECTION_CONNECTED, connection.status);
    TEST_ASSERT_EQUAL(0, connection.consecutive_failures);
    // connected_since should be updated (we can't check exact value, but it should change)
    TEST_ASSERT_NOT_EQUAL(123456, connection.connected_since);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_db2_reset_connection_null_connection);
    RUN_TEST(test_db2_reset_connection_wrong_engine_type);
    RUN_TEST(test_db2_reset_connection_success);

    return UNITY_END();
}