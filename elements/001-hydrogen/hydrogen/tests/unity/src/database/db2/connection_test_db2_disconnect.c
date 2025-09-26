/*
 * Unity Test File: DB2 Connection Disconnect Function
 * This file contains unit tests for db2_disconnect functionality
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
bool db2_disconnect(DatabaseHandle* connection);

// Function prototypes for test functions
static void test_db2_disconnect_null_connection(void);
static void test_db2_disconnect_wrong_engine_type(void);
static void test_db2_disconnect_success(void);
static void test_db2_disconnect_null_db2_handle(void);

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

// Test db2_disconnect with NULL connection
void test_db2_disconnect_null_connection(void) {
    bool result = db2_disconnect(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test db2_disconnect with wrong engine type
void test_db2_disconnect_wrong_engine_type(void) {
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_POSTGRESQL, // Wrong type
        .connection_handle = NULL
    };
    bool result = db2_disconnect(&connection);
    TEST_ASSERT_FALSE(result);
}

// Test db2_disconnect success case
void test_db2_disconnect_success(void) {
    // Create a mock DatabaseHandle with DB2 connection
    DB2Connection* db2_conn = calloc(1, sizeof(DB2Connection));
    TEST_ASSERT_NOT_NULL(db2_conn);

    // Create prepared statement cache
    db2_conn->prepared_statements = calloc(1, sizeof(PreparedStatementCache));
    TEST_ASSERT_NOT_NULL(db2_conn->prepared_statements);
    db2_conn->prepared_statements->capacity = 16;
    db2_conn->prepared_statements->names = calloc(16, sizeof(char*));
    TEST_ASSERT_NOT_NULL(db2_conn->prepared_statements->names);
    pthread_mutex_init(&db2_conn->prepared_statements->lock, NULL);

    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = db2_conn,
        .designator = strdup("test")
    };
    pthread_mutex_init(&connection.connection_lock, NULL);

    bool result = db2_disconnect(&connection);
    TEST_ASSERT_TRUE(result);

    // Verify cleanup
    TEST_ASSERT_EQUAL(DB_CONNECTION_DISCONNECTED, connection.status);

    // Note: The function frees the designator and DB2Connection,
    // so we don't need to free them again
}

// Test db2_disconnect with NULL DB2 connection handle
void test_db2_disconnect_null_db2_handle(void) {
    DatabaseHandle connection = {
        .engine_type = DB_ENGINE_DB2,
        .connection_handle = NULL,
        .designator = strdup("test")
    };
    pthread_mutex_init(&connection.connection_lock, NULL);

    bool result = db2_disconnect(&connection);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(DB_CONNECTION_DISCONNECTED, connection.status);

    // Clean up
    free(connection.designator);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_db2_disconnect_null_connection);
    RUN_TEST(test_db2_disconnect_wrong_engine_type);
    RUN_TEST(test_db2_disconnect_success);
    RUN_TEST(test_db2_disconnect_null_db2_handle);

    return UNITY_END();
}