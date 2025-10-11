/*
 * Unity Test File: DB2 Connection Health Check Function
 * This file contains unit tests for db2_health_check functionality
 */

#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libdb2.h>
#include <unity/mocks/mock_system.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/db2/types.h>
#include <src/database/db2/connection.h>

// Forward declarations for functions being tested
bool db2_health_check(DatabaseHandle* connection);

// Function prototypes for test functions
static void test_db2_health_check_null_connection(void);
static void test_db2_health_check_wrong_engine_type(void);
static void test_db2_health_check_null_db2_handle(void);
static void test_db2_health_check_null_connection_ptr(void);
static void test_db2_health_check_stmt_alloc_failure(void);
static void test_db2_health_check_exec_failure(void);
static void test_db2_health_check_success(void);

void setUp(void) {
    // Reset all mocks to default state
    mock_libdb2_reset_all();
    mock_system_reset_all();

    // Ensure libdb2 functions are loaded (mocked)
    load_libdb2_functions(NULL);
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test db2_health_check with NULL connection
void test_db2_health_check_null_connection(void) {
    bool result = db2_health_check(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test db2_health_check with wrong engine type
void test_db2_health_check_wrong_engine_type(void) {
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_POSTGRESQL, // Wrong type
        .connection_handle = NULL
    };
    bool result = db2_health_check(&connection);
    TEST_ASSERT_FALSE(result);
}

// Test db2_health_check with NULL DB2 connection handle
void test_db2_health_check_null_db2_handle(void) {
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = NULL
    };
    bool result = db2_health_check(&connection);
    TEST_ASSERT_FALSE(result);
}

// Test db2_health_check with NULL connection pointer in DB2 handle
void test_db2_health_check_null_connection_ptr(void) {
    DB2Connection db2_conn = {
        .connection = NULL
    };
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn
    };
    bool result = db2_health_check(&connection);
    TEST_ASSERT_FALSE(result);
}

// Test db2_health_check with statement handle allocation failure
void test_db2_health_check_stmt_alloc_failure(void) {
    DB2Connection db2_conn = {
        .connection = (void*)0x12345678 // Mock connection handle
    };
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn,
        .last_health_check = 0,
        .consecutive_failures = 0
    };

    // Mock SQLAllocHandle to fail for statement handle
    mock_libdb2_set_SQLAllocHandle_result(-1); // SQL_ERROR

    bool result = db2_health_check(&connection);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(1, connection.consecutive_failures);
}

// Test db2_health_check with SQLExecDirect failure
void test_db2_health_check_exec_failure(void) {
    DB2Connection db2_conn = {
        .connection = (void*)0x12345678 // Mock connection handle
    };
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn,
        .last_health_check = 0,
        .consecutive_failures = 0
    };

    // Mock SQLExecDirect to fail
    mock_libdb2_set_SQLExecDirect_result(-1); // SQL_ERROR

    bool result = db2_health_check(&connection);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(1, connection.consecutive_failures);
}

// Test db2_health_check success case
void test_db2_health_check_success(void) {
    DB2Connection db2_conn = {
        .connection = (void*)0x12345678 // Mock connection handle
    };
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = &db2_conn,
        .last_health_check = 0,
        .consecutive_failures = 5 // Should be reset to 0
    };

    bool result = db2_health_check(&connection);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, connection.consecutive_failures);
    TEST_ASSERT_NOT_EQUAL(0, connection.last_health_check);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_db2_health_check_null_connection);
    RUN_TEST(test_db2_health_check_wrong_engine_type);
    RUN_TEST(test_db2_health_check_null_db2_handle);
    RUN_TEST(test_db2_health_check_null_connection_ptr);
    RUN_TEST(test_db2_health_check_stmt_alloc_failure);
    RUN_TEST(test_db2_health_check_exec_failure);
    RUN_TEST(test_db2_health_check_success);

    return UNITY_END();
}