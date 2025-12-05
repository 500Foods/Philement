/*
 * Unity Test File: Comprehensive Database Launch Coverage Tests
 * This file contains tests to achieve maximum coverage of launch_database.c
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>

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
// USE_MOCK_LAUNCH defined by CMake

#include <unity/mocks/mock_libpq.h>
#include <unity/mocks/mock_libmysqlclient.h>
#include <unity/mocks/mock_libsqlite3.h>
#include <unity/mocks/mock_libdb2.h>
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_launch.h>

// Forward declarations for functions being tested
LaunchReadiness check_database_launch_readiness(void);
int launch_database_subsystem(void);

// Test configuration
static AppConfig* test_app_config = NULL;
static DatabaseConfig* test_db_config = NULL;

static void setup_comprehensive_test_config(void) {
    if (test_app_config) {
        cleanup_application_config();
        free(test_app_config);
    }

    test_app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(test_app_config);

    // Initialize with comprehensive database configuration
    test_db_config = &test_app_config->databases;
    test_app_config->databases.connection_count = 3;

    // Initialize connections in the fixed array
    for (int i = 0; i < 3; i++) {
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

    // SQLite connection
    DatabaseConnection* sqlite_conn = &test_app_config->databases.connections[1];
    sqlite_conn->name = strdup("test_sqlite");
    sqlite_conn->type = strdup("sqlite");
    sqlite_conn->enabled = true;
    sqlite_conn->database = strdup("/tmp/test.db");

    // MySQL connection
    DatabaseConnection* mysql_conn = &test_app_config->databases.connections[2];
    mysql_conn->name = strdup("test_mysql");
    mysql_conn->type = strdup("mysql");
    mysql_conn->enabled = true;
    mysql_conn->database = strdup("testdb");
    mysql_conn->host = strdup("localhost");
    mysql_conn->port = strdup("3306");
    mysql_conn->user = strdup("testuser");
    mysql_conn->pass = strdup("testpass");

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

    // Setup comprehensive test configuration
    setup_comprehensive_test_config();

    // Reset global state to allow full function execution
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

// Test database counting and reporting logic
static void test_database_counting_and_reporting(void) {
    // Setup mocks for subsystem operations
    mock_launch_set_get_subsystem_id_result(-1); // Force subsystem registration path

    // Setup library mocks
    mock_system_set_dlopen_result((void*)0x12345678); // Valid library handle
    mock_system_set_access_result(0); // File exists

    // Test
    LaunchReadiness result = check_database_launch_readiness();

    // Should exercise database counting logic
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_EQUAL_STRING(SR_DATABASE, result.subsystem);

    // Verify that multiple database types are counted and reported
    // The function should process all 3 database connections
}

static void test_library_dependency_validation(void) {
    // Setup mocks for library validation
    mock_launch_set_get_subsystem_id_result(-1); // Force registration

    // Test PostgreSQL library validation
    mock_system_set_dlopen_result((void*)0x11111111);

    LaunchReadiness result = check_database_launch_readiness();

    // Should exercise PostgreSQL library loading and validation
    TEST_ASSERT_NOT_NULL(result.messages);
}

static void test_database_connection_validation_loop(void) {
    // Setup mocks for connection validation
    mock_launch_set_get_subsystem_id_result(-1);
    mock_system_set_dlopen_result((void*)0x22222222);
    mock_system_set_access_result(0);

    LaunchReadiness result = check_database_launch_readiness();

    // Should exercise the database connection validation loop
    // This processes all database connections and validates their parameters
    TEST_ASSERT_NOT_NULL(result.messages);
}

static void test_error_message_generation(void) {
    // Setup mocks to trigger various error conditions
    mock_launch_set_get_subsystem_id_result(-1);
    mock_system_set_dlopen_result(NULL); // Library loading fails
    mock_system_set_dlopen_failure(1);
    mock_system_set_dlerror_result("Library not found");

    LaunchReadiness result = check_database_launch_readiness();

    // Should generate comprehensive error messages
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
}

static void test_subsystem_registration_logic(void) {
    // Setup mocks for subsystem registration testing
    mock_launch_set_get_subsystem_id_result(-1); // Not registered

    LaunchReadiness result = check_database_launch_readiness();

    // Should exercise subsystem registration logic
    TEST_ASSERT_NOT_NULL(result.messages);
}

static void test_database_type_specific_validation(void) {
    // Setup configuration with different database types
    test_app_config->databases.connection_count = 2;

    // SQLite connection (file-based)
    test_app_config->databases.connections[0].name = strdup("sqlite_db");
    test_app_config->databases.connections[0].type = strdup("sqlite");
    test_app_config->databases.connections[0].enabled = true;
    test_app_config->databases.connections[0].database = strdup("/tmp/sqlite_test.db");

    // PostgreSQL connection (network-based)
    test_app_config->databases.connections[1].name = strdup("postgres_db");
    test_app_config->databases.connections[1].type = strdup("postgresql");
    test_app_config->databases.connections[1].enabled = true;
    test_app_config->databases.connections[1].database = strdup("postgres_db");
    test_app_config->databases.connections[1].host = strdup("localhost");
    test_app_config->databases.connections[1].port = strdup("5432");
    test_app_config->databases.connections[1].user = strdup("user");
    test_app_config->databases.connections[1].pass = strdup("pass");

    // Setup mocks
    mock_launch_set_get_subsystem_id_result(-1);
    mock_system_set_dlopen_result((void*)0x33333333);
    mock_system_set_access_result(0); // SQLite file exists

    LaunchReadiness result = check_database_launch_readiness();

    // Should exercise type-specific validation logic
    TEST_ASSERT_NOT_NULL(result.messages);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_counting_and_reporting);
    RUN_TEST(test_library_dependency_validation);
    RUN_TEST(test_database_connection_validation_loop);
    RUN_TEST(test_error_message_generation);
    RUN_TEST(test_subsystem_registration_logic);
    RUN_TEST(test_database_type_specific_validation);

    return UNITY_END();
}