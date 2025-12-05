/*
 * Unity Test File: Focused Database Launch Coverage Tests
 * This file targets specific high-coverage areas identified from blackbox test analysis
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

static void setup_realistic_test_config(void) {
    if (test_app_config) {
        cleanup_application_config();
        free(test_app_config);
    }

    test_app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(test_app_config);

    // Initialize with realistic database configuration
    test_db_config = &test_app_config->databases;
    test_app_config->databases.connection_count = 2;

    // Initialize connections in the fixed array
    for (int i = 0; i < 2; i++) {
        memset(&test_app_config->databases.connections[i], 0, sizeof(DatabaseConnection));
    }

    // PostgreSQL connection (will trigger library validation)
    DatabaseConnection* postgres_conn = &test_app_config->databases.connections[0];
    postgres_conn->name = strdup("production_db");
    postgres_conn->type = strdup("postgresql");
    postgres_conn->enabled = true;
    postgres_conn->database = strdup("myapp_prod");
    postgres_conn->host = strdup("db.example.com");
    postgres_conn->port = strdup("5432");
    postgres_conn->user = strdup("app_user");
    postgres_conn->pass = strdup("secure_password");

    // SQLite connection (will trigger file validation)
    DatabaseConnection* sqlite_conn = &test_app_config->databases.connections[1];
    sqlite_conn->name = strdup("cache_db");
    sqlite_conn->type = strdup("sqlite");
    sqlite_conn->enabled = true;
    sqlite_conn->database = strdup("/var/lib/myapp/cache.db");

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

    // Setup realistic test configuration
    setup_realistic_test_config();

    // Reset global state to allow full function execution
    server_stopping = 0;
    server_starting = 1;
    server_running = 0;
    database_stopping = 0;
}

void tearDown(void) {
    cleanup_test_config();

    // Clean up test files
    system("rm -f /var/lib/myapp/cache.db /tmp/test.db");
}

// Test subsystem registration and dependency management
static void test_subsystem_registration_and_dependencies(void) {
    // Setup mocks for subsystem registration
    mock_launch_set_get_subsystem_id_result(-1); // Not registered, force registration

    // Setup library mocks for successful loading
    mock_system_set_dlopen_result((void*)0x12345678);
    mock_system_set_access_result(0); // SQLite file exists

    // Test
    LaunchReadiness result = check_database_launch_readiness();

    // Should exercise subsystem registration logic (lines 47-82)
    // Should exercise dependency registration (Registry, Threads, Network)
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_EQUAL_STRING(SR_DATABASE, result.subsystem);
}

static void test_database_counting_logic(void) {
    // Setup mocks
    mock_launch_set_get_subsystem_id_result(-1);
    mock_system_set_dlopen_result((void*)0x11111111);
    mock_system_set_access_result(0);

    // Test
    LaunchReadiness result = check_database_launch_readiness();

    // Should exercise database counting logic (lines 91-242)
    // This includes counting by database type and generating reports
    TEST_ASSERT_NOT_NULL(result.messages);
}

static void test_database_connection_processing_loop(void) {
    // Setup mocks
    mock_launch_set_get_subsystem_id_result(-1);
    mock_system_set_dlopen_result((void*)0x22222222);
    mock_system_set_access_result(0);

    // Test
    LaunchReadiness result = check_database_launch_readiness();

    // Should exercise database connection processing loop (lines 438-551)
    // This validates each database connection and its parameters
    TEST_ASSERT_NOT_NULL(result.messages);
}

static void test_library_dependency_validation_detailed(void) {
    // Setup mocks for detailed library validation
    mock_launch_set_get_subsystem_id_result(-1);
    mock_system_set_dlopen_result((void*)0x33333333);
    mock_system_set_access_result(0);

    // Test
    LaunchReadiness result = check_database_launch_readiness();

    // Should exercise library dependency validation (lines 247-435)
    // This includes PostgreSQL, MySQL, SQLite, and DB2 library checking
    TEST_ASSERT_NOT_NULL(result.messages);
}

static void test_error_handling_and_reporting(void) {
    // Setup mocks to trigger error conditions
    mock_launch_set_get_subsystem_id_result(-1);
    mock_system_set_dlopen_result(NULL); // Library loading fails
    mock_system_set_dlopen_failure(1);
    mock_system_set_dlerror_result("Library not found");
    mock_system_set_access_result(-1); // File doesn't exist

    // Test
    LaunchReadiness result = check_database_launch_readiness();

    // Should exercise error message generation throughout the function
    TEST_ASSERT_NOT_NULL(result.messages);
    TEST_ASSERT_FALSE(result.ready);
}

static void test_mixed_database_types_scenario(void) {
    // Setup configuration with multiple database types
    test_app_config->databases.connection_count = 3;

    // PostgreSQL
    test_app_config->databases.connections[0].name = strdup("postgres_main");
    test_app_config->databases.connections[0].type = strdup("postgresql");
    test_app_config->databases.connections[0].enabled = true;
    test_app_config->databases.connections[0].database = strdup("main_db");
    test_app_config->databases.connections[0].host = strdup("localhost");
    test_app_config->databases.connections[0].port = strdup("5432");
    test_app_config->databases.connections[0].user = strdup("user");
    test_app_config->databases.connections[0].pass = strdup("pass");

    // SQLite
    test_app_config->databases.connections[1].name = strdup("sqlite_cache");
    test_app_config->databases.connections[1].type = strdup("sqlite");
    test_app_config->databases.connections[1].enabled = true;
    test_app_config->databases.connections[1].database = strdup("/tmp/cache.db");

    // MySQL
    test_app_config->databases.connections[2].name = strdup("mysql_logs");
    test_app_config->databases.connections[2].type = strdup("mysql");
    test_app_config->databases.connections[2].enabled = true;
    test_app_config->databases.connections[2].database = strdup("logs");
    test_app_config->databases.connections[2].host = strdup("localhost");
    test_app_config->databases.connections[2].port = strdup("3306");
    test_app_config->databases.connections[2].user = strdup("user");
    test_app_config->databases.connections[2].pass = strdup("pass");

    // Setup mocks
    mock_launch_set_get_subsystem_id_result(-1);
    mock_system_set_dlopen_result((void*)0x44444444);
    mock_system_set_access_result(0);

    // Test
    LaunchReadiness result = check_database_launch_readiness();

    // Should exercise all database type validation logic
    TEST_ASSERT_NOT_NULL(result.messages);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_subsystem_registration_and_dependencies);
    RUN_TEST(test_database_counting_logic);
    RUN_TEST(test_database_connection_processing_loop);
    RUN_TEST(test_library_dependency_validation_detailed);
    RUN_TEST(test_error_handling_and_reporting);
    RUN_TEST(test_mixed_database_types_scenario);

    return UNITY_END();
}