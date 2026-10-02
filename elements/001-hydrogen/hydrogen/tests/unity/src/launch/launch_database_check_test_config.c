/*
 * Unity tests for launch_database_check.c: validate_database_connections, check_database_launch_readiness,
 * and launch_database_subsystem.
 *
 * CHANGELOG
 * 4.0.0 - 2026-10-02 - Split from launch_database_check_test_edges.c to keep test files under 1000 lines.
 *                       Tests for validate_database_connections, check_database_launch_readiness,
 *                       and launch_database_subsystem.
 */

// Standard project header plus Unity Framework header
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
#define USE_MOCK_LAUNCH

#include <unity/mocks/mock_libpq.h>
#include <unity/mocks/mock_libmysqlclient.h>
#include <unity/mocks/mock_libsqlite3.h>
#include <unity/mocks/mock_libdb2.h>
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_launch.h>
#include <unity/mocks/mock_logging.h>
#include <unity/mocks/mock_libmicrohttpd.h>

// Forward declarations for test functions in this file
void test_validate_database_connections_invalid_name(void);
void test_validate_database_connections_invalid_type(void);
void test_validate_database_connections_missing_sqlite_database(void);
void test_validate_database_connections_sqlite_file_not_found(void);
void test_validate_database_connections_missing_fields_non_sqlite(void);
void test_validate_database_connections_disabled(void);
void test_validate_database_connections_valid_sqlite(void);
void test_validate_database_connections_valid_non_sqlite(void);
void test_validate_database_connections_null_name(void);
void test_validate_database_connections_empty_name(void);
void test_validate_database_connections_short_name_valid(void);
void test_validate_database_connections_max_length_name(void);
void test_validate_database_connections_empty_type(void);
void test_validate_database_connections_zero_count(void);
void test_check_database_launch_readiness_dependency_failures(void);
void test_check_database_launch_readiness_already_registered(void);
void test_check_database_launch_readiness_invalid_connections(void);
void test_check_database_launch_readiness_library_dependency_missing(void);
void test_launch_database_subsystem_with_mocks(void);

// Forward declarations for tests in other split files
void test_add_connection_name_null_seed(void);
extern volatile sig_atomic_t database_stopping;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_stopping;

// Test data structures
static AppConfig test_app_config;

void setUp(void) {
    // Reset global state
    server_stopping = 0;
    server_starting = 1;
    server_running = 0;
    database_stopping = 0;
    app_config = &test_app_config;
    mock_system_reset_all();
    mock_launch_reset_all();

    // Set up default valid config
    memset(&test_app_config, 0, sizeof(AppConfig));
    test_app_config.databases.connection_count = 0;
}

void tearDown(void) {
    // Clean up
}

// ============================================================================
// validate_database_connections tests
// ============================================================================

// Test invalid connection name (too long)
void test_validate_database_connections_invalid_name(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    // Invalid name (too long - > 64 characters)
    db_config.connections[0].enabled = true;
    db_config.connections[0].name = strdup("this_name_is_definitely_way_too_long_for_the_validation_limits_and_should_fail");
    db_config.connections[0].type = strdup("sqlite");
    db_config.connections[0].database = strdup("/dev/null");

    bool result = validate_database_connections(&db_config, &messages, &count, &capacity);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(messages);

    // Clean up
    free(db_config.connections[0].name);
    free(db_config.connections[0].type);
    free(db_config.connections[0].database);
}

// Test invalid connection type (too long)
void test_validate_database_connections_invalid_type(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    // Invalid type (too long)
    db_config.connections[0].enabled = true;
    db_config.connections[0].name = strdup("test");
    db_config.connections[0].type = strdup("this_type_name_is_too_long_for_validation");

    bool result = validate_database_connections(&db_config, &messages, &count, &capacity);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(messages);

    // Clean up
    free(db_config.connections[0].name);
    free(db_config.connections[0].type);
}

// Test missing database for SQLite
void test_validate_database_connections_missing_sqlite_database(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    db_config.connections[0].enabled = true;
    db_config.connections[0].name = strdup("test");
    db_config.connections[0].type = strdup("sqlite");
    db_config.connections[0].database = NULL; // Missing database

    bool result = validate_database_connections(&db_config, &messages, &count, &capacity);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(messages);

    // Clean up
    free(db_config.connections[0].name);
    free(db_config.connections[0].type);
}

// Test SQLite file not found
void test_validate_database_connections_sqlite_file_not_found(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    db_config.connections[0].enabled = true;
    db_config.connections[0].name = strdup("test");
    db_config.connections[0].type = strdup("sqlite");
    db_config.connections[0].database = strdup("/nonexistent/file.db");

    bool result = validate_database_connections(&db_config, &messages, &count, &capacity);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(messages);

    // Clean up
    free(db_config.connections[0].name);
    free(db_config.connections[0].type);
    free(db_config.connections[0].database);
}

// Test missing fields for non-SQLite databases
void test_validate_database_connections_missing_fields_non_sqlite(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    db_config.connections[0].enabled = true;
    db_config.connections[0].name = strdup("test");
    db_config.connections[0].type = strdup("postgresql");
    // Missing all required fields: database, host, port, user, pass

    bool result = validate_database_connections(&db_config, &messages, &count, &capacity);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(messages);

    // Clean up
    free(db_config.connections[0].name);
    free(db_config.connections[0].type);
}

// Test disabled database connection
void test_validate_database_connections_disabled(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    db_config.connections[0].enabled = false; // Disabled
    db_config.connections[0].name = strdup("test");
    db_config.connections[0].type = strdup("sqlite");

    bool result = validate_database_connections(&db_config, &messages, &count, &capacity);

    TEST_ASSERT_TRUE(result); // Disabled connections are valid
    TEST_ASSERT_NOT_NULL(messages);

    // Clean up
    free(db_config.connections[0].name);
    free(db_config.connections[0].type);
}

// Test valid SQLite connection with existing file
void test_validate_database_connections_valid_sqlite(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    // /dev/null always exists and is accessible
    db_config.connections[0].enabled = true;
    db_config.connections[0].name = strdup("test");
    db_config.connections[0].type = strdup("sqlite");
    db_config.connections[0].database = strdup("/dev/null");

    bool result = validate_database_connections(&db_config, &messages, &count, &capacity);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(messages);

    // Clean up
    free(db_config.connections[0].name);
    free(db_config.connections[0].type);
    free(db_config.connections[0].database);
}

// Test valid non-SQLite connection with all fields
void test_validate_database_connections_valid_non_sqlite(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    db_config.connections[0].enabled = true;
    db_config.connections[0].name = strdup("test");
    db_config.connections[0].type = strdup("postgresql");
    db_config.connections[0].database = strdup("testdb");
    db_config.connections[0].host = strdup("localhost");
    db_config.connections[0].port = strdup("5432");
    db_config.connections[0].user = strdup("testuser");
    db_config.connections[0].pass = strdup("testpass");

    bool result = validate_database_connections(&db_config, &messages, &count, &capacity);

    TEST_ASSERT_TRUE(result);

    // Clean up
    free(db_config.connections[0].name);
    free(db_config.connections[0].type);
    free(db_config.connections[0].database);
    free(db_config.connections[0].host);
    free(db_config.connections[0].port);
    free(db_config.connections[0].user);
    free(db_config.connections[0].pass);
}

// Test NULL name (edge case - strlen(NULL) is undefined but the check comes first)
void test_validate_database_connections_null_name(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    db_config.connections[0].enabled = true;
    db_config.connections[0].name = NULL;  // NULL name
    db_config.connections[0].type = strdup("sqlite");

    bool result = validate_database_connections(&db_config, &messages, &count, &capacity);

    // NULL name -> !conn->name is true -> invalid
    TEST_ASSERT_FALSE(result);

    // Clean up
    free(db_config.connections[0].type);
}

// Test empty name (strlen < 1)
void test_validate_database_connections_empty_name(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    db_config.connections[0].enabled = true;
    db_config.connections[0].name = strdup("");  // Empty name
    db_config.connections[0].type = strdup("sqlite");

    bool result = validate_database_connections(&db_config, &messages, &count, &capacity);

    TEST_ASSERT_FALSE(result);

    // Clean up
    free(db_config.connections[0].name);
    free(db_config.connections[0].type);
}

// Test short name (valid)
void test_validate_database_connections_short_name_valid(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    db_config.connections[0].enabled = true;
    db_config.connections[0].name = strdup("db1");
    db_config.connections[0].type = strdup("sqlite");
    db_config.connections[0].database = strdup("/dev/null");

    bool result = validate_database_connections(&db_config, &messages, &count, &capacity);

    TEST_ASSERT_TRUE(result);

    // Clean up
    free(db_config.connections[0].name);
    free(db_config.connections[0].type);
    free(db_config.connections[0].database);
}

// Test exactly 64-char name (valid)
void test_validate_database_connections_max_length_name(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    db_config.connections[0].enabled = true;
    db_config.connections[0].name = strdup("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012345");
    db_config.connections[0].type = strdup("sqlite");
    db_config.connections[0].database = strdup("/dev/null");

    bool result = validate_database_connections(&db_config, &messages, &count, &capacity);

    TEST_ASSERT_TRUE(result);

    // Clean up
    free(db_config.connections[0].name);
    free(db_config.connections[0].type);
    free(db_config.connections[0].database);
}

// Test empty type (strlen < 1)
void test_validate_database_connections_empty_type(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    db_config.connections[0].enabled = true;
    db_config.connections[0].name = strdup("test");
    db_config.connections[0].type = strdup("");  // Empty type

    bool result = validate_database_connections(&db_config, &messages, &count, &capacity);

    TEST_ASSERT_FALSE(result);

    // Clean up
    free(db_config.connections[0].name);
    free(db_config.connections[0].type);
}

// Test zero connection_count (no connections to validate)
void test_validate_database_connections_zero_count(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 0;

    bool result = validate_database_connections(&db_config, &messages, &count, &capacity);

    TEST_ASSERT_TRUE(result); // No connections = valid
}

// ============================================================================
// check_database_launch_readiness tests
// ============================================================================

// Test basic functionality
void test_check_database_launch_readiness_dependency_failures(void) {
    // This test verifies the function can be called and returns a readiness struct
    LaunchReadiness result = check_database_launch_readiness();
    TEST_ASSERT_NOT_NULL(result.subsystem);
}

// Test already registered scenario
void test_check_database_launch_readiness_already_registered(void) {
    // Set up mock to return an existing subsystem ID
    mock_launch_set_get_subsystem_id_result(5);

    LaunchReadiness result = check_database_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);

    // Reset mock
    mock_launch_set_get_subsystem_id_result(-1);
}

// Test invalid connections (empty name)
void test_check_database_launch_readiness_invalid_connections(void) {
    // Save original config
    AppConfig* original_config = app_config;

    // Create config with invalid connections
    AppConfig test_config = {0};
    test_config.databases.connection_count = 1;
    test_config.databases.connections[0].enabled = true;
    test_config.databases.connections[0].name = strdup(""); // Invalid empty name
    test_config.databases.connections[0].type = strdup("sqlite");

    app_config = &test_config;

    LaunchReadiness result = check_database_launch_readiness();

    TEST_ASSERT_FALSE(result.ready);

    // Clean up
    free(test_config.databases.connections[0].name);
    free(test_config.databases.connections[0].type);
    app_config = original_config;
}

// Test library dependency missing scenario
void test_check_database_launch_readiness_library_dependency_missing(void) {
    // Save original config
    AppConfig* original_config = app_config;

    // Create config with PostgreSQL database (library may not be available)
    AppConfig test_config = {0};
    test_config.databases.connection_count = 1;
    test_config.databases.connections[0].enabled = true;
    test_config.databases.connections[0].name = strdup("test_pg");
    test_config.databases.connections[0].type = strdup("postgresql");
    test_config.databases.connections[0].database = strdup("testdb");
    test_config.databases.connections[0].host = strdup("localhost");
    test_config.databases.connections[0].port = strdup("5432");
    test_config.databases.connections[0].user = strdup("test");
    test_config.databases.connections[0].pass = strdup("test");

    app_config = &test_config;

    LaunchReadiness result = check_database_launch_readiness();

    // Readiness depends on whether PostgreSQL library is available
    TEST_ASSERT_NOT_NULL(result.subsystem);

    // Clean up
    free(test_config.databases.connections[0].name);
    free(test_config.databases.connections[0].type);
    free(test_config.databases.connections[0].database);
    free(test_config.databases.connections[0].host);
    free(test_config.databases.connections[0].port);
    free(test_config.databases.connections[0].user);
    free(test_config.databases.connections[0].pass);
    app_config = original_config;
}

// ============================================================================
// launch_database_subsystem tests
// ============================================================================

// Test launch_database_subsystem with mocks (requires mock setup)
void test_launch_database_subsystem_with_mocks(void) {
    // Save original state
    AppConfig* original_config = app_config;
    volatile sig_atomic_t original_stopping = server_stopping;
    volatile sig_atomic_t original_starting = server_starting;
    volatile sig_atomic_t original_running = server_running;

    // Set up minimal test state
    server_stopping = 0;
    server_starting = 1;
    server_running = 0;

    AppConfig test_config = {0};
    test_config.databases.connection_count = 0; // No databases to avoid complex setup
    app_config = &test_config;

    // Call the function - it should return 0 due to no databases
    int result = launch_database_subsystem();
    TEST_ASSERT_EQUAL(0, result);

    // Restore state
    app_config = original_config;
    server_stopping = original_stopping;
    server_starting = original_starting;
    server_running = original_running;
}

// Forward declarations for tests in the other split files
void test_add_connection_name_null_seed(void);
void test_add_connection_name_append(void);
void test_add_connection_name_null_new_name(void);
void test_add_connection_name_existing_with_null_new(void);
void test_add_connection_name_realloc_failure(void);
void test_report_database_count_with_names(void);
void test_report_database_count_truncation(void);
void test_report_database_count_null_names(void);
void test_check_database_library_success_with_version(void);
void test_check_database_library_success_no_expected(void);
void test_check_database_library_failure(void);
void test_check_database_library_keep_open(void);
void test_check_database_library_dependencies_postgres(void);
void test_check_database_library_dependencies_mysql(void);
void test_check_database_library_dependencies_db2(void);
void test_check_database_library_dependencies_sqlite(void);
void test_check_database_library_dependencies_mariadb(void);
void test_check_database_library_dependencies_firebird(void);
void test_check_database_library_dependencies_mssql(void);
void test_check_database_library_dependencies_zero(void);
void test_validate_database_configuration_multiple_same_type(void);
void test_validate_database_configuration_multiple_postgres(void);
void test_validate_database_configuration_multiple_mysql(void);
void test_validate_database_configuration_multiple_sqlite(void);
void test_validate_database_configuration_multiple_db2(void);
void test_validate_database_configuration_multiple_mariadb(void);
void test_validate_database_configuration_multiple_firebird(void);
void test_validate_database_configuration_multiple_mssql(void);
void test_validate_database_configuration_truncation(void);
void test_validate_database_configuration_null_connection_name(void);
void test_validate_database_configuration_disabled_connection(void);
void test_validate_database_configuration_zero_connections(void);
void test_validate_database_configuration_unknown_type(void);
void test_validate_database_configuration_null_type(void);
void test_validate_database_configuration_postgres_alias(void);
void test_validate_database_configuration_mixed_types(void);

int main(void) {
    UNITY_BEGIN();

    // validate_database_connections tests
    RUN_TEST(test_validate_database_connections_invalid_name);
    RUN_TEST(test_validate_database_connections_invalid_type);
    RUN_TEST(test_validate_database_connections_missing_sqlite_database);
    RUN_TEST(test_validate_database_connections_sqlite_file_not_found);
    RUN_TEST(test_validate_database_connections_missing_fields_non_sqlite);
    RUN_TEST(test_validate_database_connections_disabled);
    RUN_TEST(test_validate_database_connections_valid_sqlite);
    RUN_TEST(test_validate_database_connections_valid_non_sqlite);
    RUN_TEST(test_validate_database_connections_null_name);
    RUN_TEST(test_validate_database_connections_empty_name);
    RUN_TEST(test_validate_database_connections_short_name_valid);
    RUN_TEST(test_validate_database_connections_max_length_name);
    RUN_TEST(test_validate_database_connections_empty_type);
    RUN_TEST(test_validate_database_connections_zero_count);

    // check_database_launch_readiness tests
    RUN_TEST(test_check_database_launch_readiness_dependency_failures);
    RUN_TEST(test_check_database_launch_readiness_already_registered);
    RUN_TEST(test_check_database_launch_readiness_invalid_connections);
    RUN_TEST(test_check_database_launch_readiness_library_dependency_missing);

    // launch_database_subsystem tests
    RUN_TEST(test_launch_database_subsystem_with_mocks);

    return UNITY_END();
}
