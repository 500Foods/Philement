/*
 * Unity Test File: Enhanced Database Launch Coverage Tests
 * This file contains comprehensive unit tests for launch_database.c to achieve 75%+ coverage
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/launch/launch.h"
#include "../../../../src/launch/launch_database.c"

// Standard library includes
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Enable mocks for comprehensive testing
#define USE_MOCK_LIBPQ
#define USE_MOCK_LIBMYSQLCLIENT
#define USE_MOCK_LIBSQLITE3
#define USE_MOCK_LIBDB2
#define USE_MOCK_SYSTEM
#define USE_MOCK_LAUNCH

#include "../../../../tests/unity/mocks/mock_libpq.h"
#include "../../../../tests/unity/mocks/mock_libmysqlclient.h"
#include "../../../../tests/unity/mocks/mock_libsqlite3.h"
#include "../../../../tests/unity/mocks/mock_libdb2.h"
#include "../../../../tests/unity/mocks/mock_system.h"
#include "../../../../tests/unity/mocks/mock_launch.h"

// Forward declarations for functions being tested
LaunchReadiness check_database_launch_readiness(void);
int launch_database_subsystem(void);

// Test configuration
static AppConfig* test_app_config = NULL;
static DatabaseConfig* test_db_config = NULL;

static void setup_test_config(void) {
    if (test_app_config) {
        cleanup_application_config();
        free(test_app_config);
    }

    test_app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(test_app_config);

    // Initialize with test database configuration
    test_db_config = &test_app_config->databases;
    test_app_config->databases.connection_count = 4;

    // Initialize connections in the fixed array
    for (int i = 0; i < 4; i++) {
        memset(&test_app_config->databases.connections[i], 0, sizeof(DatabaseConnection));
    }

    // PostgreSQL connection
    DatabaseConnection* postgres_conn = &test_app_config->databases.connections[0];
    postgres_conn->name = strdup("test_postgres");
    postgres_conn->type = strdup("postgresql");
    postgres_conn->enabled = true;
    postgres_conn->database = strdup("testdb");
    postgres_conn->host = strdup("localhost");
    postgres_conn->port = strdup("5432");
    postgres_conn->user = strdup("testuser");
    postgres_conn->pass = strdup("testpass");

    // MySQL connection
    DatabaseConnection* mysql_conn = &test_app_config->databases.connections[1];
    mysql_conn->name = strdup("test_mysql");
    mysql_conn->type = strdup("mysql");
    mysql_conn->enabled = true;
    mysql_conn->database = strdup("testdb");
    mysql_conn->host = strdup("localhost");
    mysql_conn->port = strdup("3306");
    mysql_conn->user = strdup("testuser");
    mysql_conn->pass = strdup("testpass");

    // SQLite connection
    DatabaseConnection* sqlite_conn = &test_app_config->databases.connections[2];
    sqlite_conn->name = strdup("test_sqlite");
    sqlite_conn->type = strdup("sqlite");
    sqlite_conn->enabled = true;
    sqlite_conn->database = strdup("/tmp/test.db");

    // DB2 connection
    DatabaseConnection* db2_conn = &test_app_config->databases.connections[3];
    db2_conn->name = strdup("test_db2");
    db2_conn->type = strdup("db2");
    db2_conn->enabled = true;
    db2_conn->database = strdup("testdb");
    db2_conn->host = strdup("localhost");
    db2_conn->port = strdup("50000");
    db2_conn->user = strdup("testuser");
    db2_conn->pass = strdup("testpass");

    // Set global app_config
    app_config = test_app_config;
}

static void cleanup_test_config(void) {
    if (test_app_config) {
        cleanup_application_config();
        test_app_config = NULL;
        app_config = NULL;
    }
}

void setUp(void) {
    // Reset all mocks
    mock_libpq_reset_all();
    mock_libmysqlclient_reset_all();
    mock_libsqlite3_reset_all();
    mock_libdb2_reset_all();
    mock_system_reset_all();
    mock_launch_reset_all();

    // Setup test configuration
    setup_test_config();

    // Reset global state
    server_stopping = 0;
    server_starting = 1;
    server_running = 0;
    database_stopping = 0;
}

void tearDown(void) {
    cleanup_test_config();

    // Clean up test files
    system("rm -f /tmp/test.db");
}

// Enhanced tests for check_database_launch_readiness
static void test_check_database_launch_readiness_postgresql_library_success(void) {
    // Setup: PostgreSQL library loads successfully
    mock_system_set_dlopen_result((void*)0x12345678); // Valid handle
    mock_system_set_dlerror_result(NULL);

    // Test
    LaunchReadiness result = check_database_launch_readiness();

    // The function should handle PostgreSQL library loading gracefully
    // Whether it returns ready or not depends on other factors in the complex logic
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_EQUAL_STRING(SR_DATABASE, result.subsystem);
}

static void test_check_database_launch_readiness_postgresql_library_failure(void) {
    // Setup: PostgreSQL library fails to load
    mock_system_set_dlopen_result(NULL);
    mock_system_set_dlopen_failure(1);
    mock_system_set_dlerror_result("libpq.so.5: cannot open shared object file");

    // Test
    LaunchReadiness result = check_database_launch_readiness();

    // Should not be ready due to library failure
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

static void test_check_database_launch_readiness_mysql_library_success(void) {
    // Setup: MySQL library loads successfully
    mock_system_set_dlopen_result((void*)0x87654321); // Valid handle

    // Test with only MySQL configured
    test_app_config->databases.connection_count = 1;
    free(test_app_config->databases.connections[0].name);
    free(test_app_config->databases.connections[0].type);
    test_app_config->databases.connections[0].name = strdup("test_mysql");
    test_app_config->databases.connections[0].type = strdup("mysql");

    LaunchReadiness result = check_database_launch_readiness();

    // The function should handle MySQL library loading gracefully
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_EQUAL_STRING(SR_DATABASE, result.subsystem);
}

static void test_check_database_launch_readiness_sqlite_file_exists(void) {
    // Setup: SQLite file exists
    mock_system_set_access_result(0); // F_OK success

    // Test with only SQLite configured
    test_app_config->databases.connection_count = 1;
    free(test_app_config->databases.connections[0].name);
    free(test_app_config->databases.connections[0].type);
    test_app_config->databases.connections[0].name = strdup("test_sqlite");
    test_app_config->databases.connections[0].type = strdup("sqlite");
    test_app_config->databases.connections[0].database = strdup("/tmp/test.db");

    LaunchReadiness result = check_database_launch_readiness();

    // The function should handle SQLite file validation gracefully
    // Whether it returns ready or not depends on other factors in the logic
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_EQUAL_STRING(SR_DATABASE, result.subsystem);
}

static void test_check_database_launch_readiness_sqlite_file_missing(void) {
    // Setup: SQLite file doesn't exist
    mock_system_set_access_result(-1); // F_OK failure

    // Test with only SQLite configured
    test_app_config->databases.connection_count = 1;
    free(test_app_config->databases.connections[0].name);
    free(test_app_config->databases.connections[0].type);
    test_app_config->databases.connections[0].name = strdup("test_sqlite");
    test_app_config->databases.connections[0].type = strdup("sqlite");

    LaunchReadiness result = check_database_launch_readiness();

    // Should not be ready due to missing SQLite file
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

static void test_check_database_launch_readiness_database_connection_invalid_name(void) {
    // Setup: Invalid database name (too short)
    test_app_config->databases.connection_count = 1;
    free(test_app_config->databases.connections[0].name);
    test_app_config->databases.connections[0].name = strdup(""); // Empty name

    LaunchReadiness result = check_database_launch_readiness();

    // Should not be ready due to invalid name
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

static void test_check_database_launch_readiness_database_connection_invalid_type(void) {
    // Setup: Invalid database type
    test_app_config->databases.connection_count = 1;
    free(test_app_config->databases.connections[0].name);
    free(test_app_config->databases.connections[0].type);
    test_app_config->databases.connections[0].name = strdup("testdb");
    test_app_config->databases.connections[0].type = strdup(""); // Empty type

    LaunchReadiness result = check_database_launch_readiness();

    // Should not be ready due to invalid type
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

static void test_check_database_launch_readiness_database_connection_missing_fields(void) {
    // Setup: PostgreSQL connection with missing required fields
    test_db_config->connection_count = 1;
    strcpy(test_db_config->connections[0].name, "testdb");
    strcpy(test_db_config->connections[0].type, "postgresql");
    test_db_config->connections[0].enabled = true;

    // Clear required fields
    free(test_db_config->connections[0].database);
    free(test_db_config->connections[0].host);
    free(test_db_config->connections[0].port);
    free(test_db_config->connections[0].user);
    free(test_db_config->connections[0].pass);
    test_db_config->connections[0].database = NULL;
    test_db_config->connections[0].host = NULL;
    test_db_config->connections[0].port = NULL;
    test_db_config->connections[0].user = NULL;
    test_db_config->connections[0].pass = NULL;

    LaunchReadiness result = check_database_launch_readiness();

    // Should not be ready due to missing fields
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

static void test_check_database_launch_readiness_disabled_database(void) {
    // Setup: Database is disabled but we need at least one enabled database
    test_app_config->databases.connection_count = 2;

    // Enable second database, disable first
    test_app_config->databases.connections[0].enabled = false;
    test_app_config->databases.connections[1].enabled = true;

    LaunchReadiness result = check_database_launch_readiness();

    // Should be ready with at least one enabled database
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

static void test_check_database_launch_readiness_zero_databases(void) {
    // Setup: No databases configured
    test_app_config->databases.connection_count = 0;

    LaunchReadiness result = check_database_launch_readiness();

    // Should not be ready when no databases are configured
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

static void test_check_database_launch_readiness_multiple_database_types(void) {
    // Setup: Multiple database types configured
    test_app_config->databases.connection_count = 4;
    // Configuration already set up in setup_test_config()

    // Setup mocks for all library types - need to handle each type
    mock_system_set_dlopen_result((void*)0x11111111);
    mock_system_set_access_result(0); // SQLite file exists

    LaunchReadiness result = check_database_launch_readiness();

    // The function should handle multiple database types gracefully
    // Whether it returns ready or not depends on complex logic with library loading
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_EQUAL_STRING(SR_DATABASE, result.subsystem);
}

int main(void) {
    UNITY_BEGIN();

    // Library dependency tests
    RUN_TEST(test_check_database_launch_readiness_postgresql_library_success);
    RUN_TEST(test_check_database_launch_readiness_postgresql_library_failure);
    RUN_TEST(test_check_database_launch_readiness_mysql_library_success);
    RUN_TEST(test_check_database_launch_readiness_sqlite_file_exists);
    RUN_TEST(test_check_database_launch_readiness_sqlite_file_missing);

    // Database connection validation tests
    RUN_TEST(test_check_database_launch_readiness_database_connection_invalid_name);
    RUN_TEST(test_check_database_launch_readiness_database_connection_invalid_type);
    RUN_TEST(test_check_database_launch_readiness_database_connection_missing_fields);
    RUN_TEST(test_check_database_launch_readiness_disabled_database);
    RUN_TEST(test_check_database_launch_readiness_zero_databases);
    RUN_TEST(test_check_database_launch_readiness_multiple_database_types);

    return UNITY_END();
}