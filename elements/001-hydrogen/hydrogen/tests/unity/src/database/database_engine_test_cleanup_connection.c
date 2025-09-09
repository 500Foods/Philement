/*
 * Unity Test File: database_engine_cleanup_connection
 * This file contains unit tests for database_engine_cleanup_connection functionality
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

#include "../../../../src/database/database.h"

// Forward declarations for functions being tested
void database_engine_cleanup_connection(DatabaseHandle* connection);

// Function prototypes for test functions
void test_database_engine_cleanup_connection_null(void);
void test_database_engine_cleanup_connection_empty(void);
void test_database_engine_cleanup_connection_with_config(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_database_engine_cleanup_connection_null(void) {
    // Test cleanup with NULL connection - should not crash
    database_engine_cleanup_connection(NULL);
    // Function should handle gracefully
}

void test_database_engine_cleanup_connection_empty(void) {
    // Test cleanup with empty connection structure
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    database_engine_cleanup_connection(connection);
    // Memory should be freed, no crash
}

void test_database_engine_cleanup_connection_with_config(void) {
    // Test cleanup with connection containing config
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    ConnectionConfig* config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(config);

    config->host = strdup("localhost");
    config->database = strdup("testdb");
    config->username = strdup("testuser");

    connection->config = config;
    connection->engine_type = DB_ENGINE_SQLITE;
    connection->status = DB_CONNECTION_CONNECTED;

    database_engine_cleanup_connection(connection);
    // Memory should be freed, no crash
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_engine_cleanup_connection_null);
    RUN_TEST(test_database_engine_cleanup_connection_empty);
    RUN_TEST(test_database_engine_cleanup_connection_with_config);

    return UNITY_END();
}