/*
 * Unity tests for launch_database_check.c: check_database_library_dependencies and validate_database_configuration.
 *
 * CHANGELOG
 * 4.0.0 - 2026-10-02 - Split from launch_database_check_test_edges.c to keep test files under 1000 lines.
 *                       Tests for check_database_library_dependencies (all 7 engines) and
 *                       validate_database_configuration (all 7 engine types).
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
// check_database_library_dependencies tests - all 7 engines
// ============================================================================

// Test library dependency checks for PostgreSQL
void test_check_database_library_dependencies_postgres(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    void* fake_handle = (void*)0xDEADBEEF;
    mock_system_set_dlopen_result(fake_handle);

    // PostgreSQL count > 0
    check_database_library_dependencies(&messages, &count, &capacity, &overall_readiness,
                                       1, 0, 0, 0, 0, 0, 0);

    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_GREATER_THAN(0, count);

    // Clean up
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test library dependency checks for MySQL
void test_check_database_library_dependencies_mysql(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    void* fake_handle = (void*)0x12345678;
    mock_system_set_dlopen_result(fake_handle);

    check_database_library_dependencies(&messages, &count, &capacity, &overall_readiness,
                                       0, 1, 0, 0, 0, 0, 0);

    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_GREATER_THAN(0, count);

    // Clean up
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test library dependency checks for DB2
void test_check_database_library_dependencies_db2(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    void* fake_handle = (void*)0xABCDEF00;
    mock_system_set_dlopen_result(fake_handle);

    check_database_library_dependencies(&messages, &count, &capacity, &overall_readiness,
                                       0, 0, 0, 1, 0, 0, 0);

    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_GREATER_THAN(0, count);

    // Clean up
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test library dependency checks for SQLite
void test_check_database_library_dependencies_sqlite(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    void* fake_handle = (void*)0xCAFEBABE;
    mock_system_set_dlopen_result(fake_handle);

    check_database_library_dependencies(&messages, &count, &capacity, &overall_readiness,
                                       0, 0, 1, 0, 0, 0, 0);

    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_GREATER_THAN(0, count);

    // Clean up
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test library dependency checks for MariaDB
void test_check_database_library_dependencies_mariadb(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    void* fake_handle = (void*)0x11111111;
    mock_system_set_dlopen_result(fake_handle);

    check_database_library_dependencies(&messages, &count, &capacity, &overall_readiness,
                                       0, 0, 0, 0, 0, 1, 0);

    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_GREATER_THAN(0, count);

    // Clean up
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test library dependency checks for Firebird
void test_check_database_library_dependencies_firebird(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    void* fake_handle = (void*)0x22222222;
    mock_system_set_dlopen_result(fake_handle);

    check_database_library_dependencies(&messages, &count, &capacity, &overall_readiness,
                                       0, 0, 0, 0, 1, 0, 0);

    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_GREATER_THAN(0, count);

    // Clean up
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test library dependency checks for MSSQL/ODBC
void test_check_database_library_dependencies_mssql(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    void* fake_handle = (void*)0x33333333;
    mock_system_set_dlopen_result(fake_handle);

    check_database_library_dependencies(&messages, &count, &capacity, &overall_readiness,
                                       0, 0, 0, 0, 0, 0, 1);

    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_GREATER_THAN(0, count);

    // Clean up
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test library dependency checks with all counts zero (no messages added)
void test_check_database_library_dependencies_zero(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    check_database_library_dependencies(&messages, &count, &capacity, &overall_readiness,
                                       0, 0, 0, 0, 0, 0, 0);

    TEST_ASSERT_EQUAL(0, count);
    TEST_ASSERT_NULL(messages);
}

// ============================================================================
// validate_database_configuration tests - all 7 engine types
// ============================================================================

// Test multiple PostgreSQL connections to trigger realloc
void test_validate_database_configuration_multiple_same_type(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 4;

    // Two PostgreSQL connections to trigger realloc
    db_config.connections[0].enabled = true;
    db_config.connections[0].type = strdup("postgresql");
    db_config.connections[0].connection_name = strdup("pg1");

    db_config.connections[1].enabled = true;
    db_config.connections[1].type = strdup("postgresql");
    db_config.connections[1].connection_name = strdup("pg2");

    // Two MySQL connections
    db_config.connections[2].enabled = true;
    db_config.connections[2].type = strdup("mysql");
    db_config.connections[2].connection_name = strdup("mysql1");

    db_config.connections[3].enabled = true;
    db_config.connections[3].type = strdup("mysql");
    db_config.connections[3].connection_name = strdup("mysql2");

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    TEST_ASSERT_EQUAL(2, postgres_count);
    TEST_ASSERT_EQUAL(2, mysql_count);
    TEST_ASSERT_EQUAL(0, sqlite_count);
    TEST_ASSERT_EQUAL(0, db2_count);
    TEST_ASSERT_EQUAL(0, firebird_count);
    TEST_ASSERT_EQUAL(0, mariadb_count);
    TEST_ASSERT_EQUAL(0, mssql_count);
    TEST_ASSERT_TRUE(overall_readiness);

    // Clean up
    free(db_config.connections[0].type);
    free(db_config.connections[0].connection_name);
    free(db_config.connections[1].type);
    free(db_config.connections[1].connection_name);
    free(db_config.connections[2].type);
    free(db_config.connections[2].connection_name);
    free(db_config.connections[3].type);
    free(db_config.connections[3].connection_name);
}

// Test three PostgreSQL connections
void test_validate_database_configuration_multiple_postgres(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 3;

    db_config.connections[0].enabled = true;
    db_config.connections[0].type = strdup("postgresql");
    db_config.connections[0].connection_name = strdup("pg1");

    db_config.connections[1].enabled = true;
    db_config.connections[1].type = strdup("postgresql");
    db_config.connections[1].connection_name = strdup("pg2");

    db_config.connections[2].enabled = true;
    db_config.connections[2].type = strdup("postgresql");
    db_config.connections[2].connection_name = strdup("pg3");

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    TEST_ASSERT_EQUAL(3, postgres_count);
    TEST_ASSERT_EQUAL(0, mysql_count);
    TEST_ASSERT_EQUAL(0, sqlite_count);
    TEST_ASSERT_EQUAL(0, db2_count);
    TEST_ASSERT_EQUAL(0, firebird_count);
    TEST_ASSERT_EQUAL(0, mariadb_count);
    TEST_ASSERT_EQUAL(0, mssql_count);
    TEST_ASSERT_TRUE(overall_readiness);

    // Clean up
    for (int i = 0; i < 3; i++) {
        free(db_config.connections[i].type);
        free(db_config.connections[i].connection_name);
    }
}

// Test three MySQL connections
void test_validate_database_configuration_multiple_mysql(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 3;

    db_config.connections[0].enabled = true;
    db_config.connections[0].type = strdup("mysql");
    db_config.connections[0].connection_name = strdup("mysql1");

    db_config.connections[1].enabled = true;
    db_config.connections[1].type = strdup("mysql");
    db_config.connections[1].connection_name = strdup("mysql2");

    db_config.connections[2].enabled = true;
    db_config.connections[2].type = strdup("mysql");
    db_config.connections[2].connection_name = strdup("mysql3");

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    TEST_ASSERT_EQUAL(0, postgres_count);
    TEST_ASSERT_EQUAL(3, mysql_count);
    TEST_ASSERT_EQUAL(0, sqlite_count);
    TEST_ASSERT_EQUAL(0, db2_count);
    TEST_ASSERT_EQUAL(0, firebird_count);
    TEST_ASSERT_EQUAL(0, mariadb_count);
    TEST_ASSERT_EQUAL(0, mssql_count);
    TEST_ASSERT_TRUE(overall_readiness);

    // Clean up
    for (int i = 0; i < 3; i++) {
        free(db_config.connections[i].type);
        free(db_config.connections[i].connection_name);
    }
}

// Test three SQLite connections
void test_validate_database_configuration_multiple_sqlite(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 3;

    db_config.connections[0].enabled = true;
    db_config.connections[0].type = strdup("sqlite");
    db_config.connections[0].connection_name = strdup("sqlite1");

    db_config.connections[1].enabled = true;
    db_config.connections[1].type = strdup("sqlite");
    db_config.connections[1].connection_name = strdup("sqlite2");

    db_config.connections[2].enabled = true;
    db_config.connections[2].type = strdup("sqlite");
    db_config.connections[2].connection_name = strdup("sqlite3");

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    TEST_ASSERT_EQUAL(0, postgres_count);
    TEST_ASSERT_EQUAL(0, mysql_count);
    TEST_ASSERT_EQUAL(3, sqlite_count);
    TEST_ASSERT_EQUAL(0, db2_count);
    TEST_ASSERT_EQUAL(0, firebird_count);
    TEST_ASSERT_EQUAL(0, mariadb_count);
    TEST_ASSERT_EQUAL(0, mssql_count);
    TEST_ASSERT_TRUE(overall_readiness);

    // Clean up
    for (int i = 0; i < 3; i++) {
        free(db_config.connections[i].type);
        free(db_config.connections[i].connection_name);
    }
}

// Test three DB2 connections
void test_validate_database_configuration_multiple_db2(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 3;

    db_config.connections[0].enabled = true;
    db_config.connections[0].type = strdup("db2");
    db_config.connections[0].connection_name = strdup("db2_1");

    db_config.connections[1].enabled = true;
    db_config.connections[1].type = strdup("db2");
    db_config.connections[1].connection_name = strdup("db2_2");

    db_config.connections[2].enabled = true;
    db_config.connections[2].type = strdup("db2");
    db_config.connections[2].connection_name = strdup("db2_3");

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    TEST_ASSERT_EQUAL(0, postgres_count);
    TEST_ASSERT_EQUAL(0, mysql_count);
    TEST_ASSERT_EQUAL(0, sqlite_count);
    TEST_ASSERT_EQUAL(3, db2_count);
    TEST_ASSERT_EQUAL(0, firebird_count);
    TEST_ASSERT_EQUAL(0, mariadb_count);
    TEST_ASSERT_EQUAL(0, mssql_count);
    TEST_ASSERT_TRUE(overall_readiness);

    // Clean up
    for (int i = 0; i < 3; i++) {
        free(db_config.connections[i].type);
        free(db_config.connections[i].connection_name);
    }
}

// Test three MariaDB connections (previously uncovered)
void test_validate_database_configuration_multiple_mariadb(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 3;

    db_config.connections[0].enabled = true;
    db_config.connections[0].type = strdup("mariadb");
    db_config.connections[0].connection_name = strdup("maria1");

    db_config.connections[1].enabled = true;
    db_config.connections[1].type = strdup("mariadb");
    db_config.connections[1].connection_name = strdup("maria2");

    db_config.connections[2].enabled = true;
    db_config.connections[2].type = strdup("mariadb");
    db_config.connections[2].connection_name = strdup("maria3");

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    TEST_ASSERT_EQUAL(0, postgres_count);
    TEST_ASSERT_EQUAL(0, mysql_count);
    TEST_ASSERT_EQUAL(0, sqlite_count);
    TEST_ASSERT_EQUAL(0, db2_count);
    TEST_ASSERT_EQUAL(0, firebird_count);
    TEST_ASSERT_EQUAL(3, mariadb_count);
    TEST_ASSERT_EQUAL(0, mssql_count);
    TEST_ASSERT_TRUE(overall_readiness);

    // Clean up
    for (int i = 0; i < 3; i++) {
        free(db_config.connections[i].type);
        free(db_config.connections[i].connection_name);
    }
}

// Test three Firebird connections (previously uncovered)
void test_validate_database_configuration_multiple_firebird(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 3;

    db_config.connections[0].enabled = true;
    db_config.connections[0].type = strdup("firebird");
    db_config.connections[0].connection_name = strdup("fb1");

    db_config.connections[1].enabled = true;
    db_config.connections[1].type = strdup("firebird");
    db_config.connections[1].connection_name = strdup("fb2");

    db_config.connections[2].enabled = true;
    db_config.connections[2].type = strdup("firebird");
    db_config.connections[2].connection_name = strdup("fb3");

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    TEST_ASSERT_EQUAL(0, postgres_count);
    TEST_ASSERT_EQUAL(0, mysql_count);
    TEST_ASSERT_EQUAL(0, sqlite_count);
    TEST_ASSERT_EQUAL(0, db2_count);
    TEST_ASSERT_EQUAL(3, firebird_count);
    TEST_ASSERT_EQUAL(0, mariadb_count);
    TEST_ASSERT_EQUAL(0, mssql_count);
    TEST_ASSERT_TRUE(overall_readiness);

    // Clean up
    for (int i = 0; i < 3; i++) {
        free(db_config.connections[i].type);
        free(db_config.connections[i].connection_name);
    }
}

// Test three MSSQL connections (previously uncovered)
void test_validate_database_configuration_multiple_mssql(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 3;

    db_config.connections[0].enabled = true;
    db_config.connections[0].type = strdup("mssql");
    db_config.connections[0].connection_name = strdup("mssql1");

    db_config.connections[1].enabled = true;
    db_config.connections[1].type = strdup("mssql");
    db_config.connections[1].connection_name = strdup("mssql2");

    db_config.connections[2].enabled = true;
    db_config.connections[2].type = strdup("mssql");
    db_config.connections[2].connection_name = strdup("mssql3");

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    TEST_ASSERT_EQUAL(0, postgres_count);
    TEST_ASSERT_EQUAL(0, mysql_count);
    TEST_ASSERT_EQUAL(0, sqlite_count);
    TEST_ASSERT_EQUAL(0, db2_count);
    TEST_ASSERT_EQUAL(0, firebird_count);
    TEST_ASSERT_EQUAL(0, mariadb_count);
    TEST_ASSERT_EQUAL(3, mssql_count);
    TEST_ASSERT_TRUE(overall_readiness);

    // Clean up
    for (int i = 0; i < 3; i++) {
        free(db_config.connections[i].type);
        free(db_config.connections[i].connection_name);
    }
}

// Test truncation when more than 3 databases of same type
void test_validate_database_configuration_truncation(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 5;

    // Four PostgreSQL connections to trigger truncation (count > 3)
    for (int i = 0; i < 4; i++) {
        db_config.connections[i].enabled = true;
        db_config.connections[i].type = strdup("postgresql");
        char name[10];
        sprintf(name, "pg%d", i);
        db_config.connections[i].connection_name = strdup(name);
    }

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    TEST_ASSERT_EQUAL(4, postgres_count);
    TEST_ASSERT_TRUE(overall_readiness);

    // Clean up
    for (int i = 0; i < 4; i++) {
        free(db_config.connections[i].type);
        free(db_config.connections[i].connection_name);
    }
}

// Test connection_name NULL (uses "Unknown")
void test_validate_database_configuration_null_connection_name(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    db_config.connections[0].enabled = true;
    db_config.connections[0].type = strdup("postgresql");
    db_config.connections[0].connection_name = NULL;  // NULL connection name

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    TEST_ASSERT_EQUAL(1, postgres_count);
    TEST_ASSERT_TRUE(overall_readiness);

    // Clean up
    free(db_config.connections[0].type);
}

// Test disabled connection (should be skipped)
void test_validate_database_configuration_disabled_connection(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    db_config.connections[0].enabled = false;  // Disabled
    db_config.connections[0].type = strdup("postgresql");
    db_config.connections[0].connection_name = strdup("test");

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    // Disabled connections should not be counted; total is 0, so overall_readiness = false
    TEST_ASSERT_EQUAL(0, postgres_count);
    TEST_ASSERT_FALSE(overall_readiness);

    // Clean up
    free(db_config.connections[0].type);
    free(db_config.connections[0].connection_name);
}

// Test with zero connections (all counts should be 0, total is 0, overall_readiness false)
void test_validate_database_configuration_zero_connections(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 0;

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    TEST_ASSERT_EQUAL(0, postgres_count);
    TEST_ASSERT_EQUAL(0, mysql_count);
    TEST_ASSERT_EQUAL(0, sqlite_count);
    TEST_ASSERT_EQUAL(0, db2_count);
    TEST_ASSERT_EQUAL(0, firebird_count);
    TEST_ASSERT_EQUAL(0, mariadb_count);
    TEST_ASSERT_EQUAL(0, mssql_count);
    TEST_ASSERT_FALSE(overall_readiness);  // No databases -> No-Go
    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_GREATER_THAN(0, count);

    // Clean up
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test with unknown engine type (should be ignored)
void test_validate_database_configuration_unknown_type(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    db_config.connections[0].enabled = true;
    db_config.connections[0].type = strdup("unknown_type");
    db_config.connections[0].connection_name = strdup("unknown");

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    // Unknown type should not be counted by any engine; total is 0, so overall_readiness = false
    TEST_ASSERT_EQUAL(0, postgres_count);
    TEST_ASSERT_EQUAL(0, mysql_count);
    TEST_ASSERT_EQUAL(0, sqlite_count);
    TEST_ASSERT_EQUAL(0, db2_count);
    TEST_ASSERT_EQUAL(0, firebird_count);
    TEST_ASSERT_EQUAL(0, mariadb_count);
    TEST_ASSERT_EQUAL(0, mssql_count);
    TEST_ASSERT_FALSE(overall_readiness);

    // Clean up
    free(db_config.connections[0].type);
    free(db_config.connections[0].connection_name);
}

// Test with NULL type (should be skipped)
void test_validate_database_configuration_null_type(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    db_config.connections[0].enabled = true;
    db_config.connections[0].type = NULL;
    db_config.connections[0].connection_name = strdup("test");

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    TEST_ASSERT_EQUAL(0, postgres_count);
    TEST_ASSERT_FALSE(overall_readiness);  // No known databases -> total is 0 -> No-Go

    // Clean up
    free(db_config.connections[0].connection_name);
}

// Test "postgres" type (alias for postgresql)
void test_validate_database_configuration_postgres_alias(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 1;

    db_config.connections[0].enabled = true;
    db_config.connections[0].type = strdup("postgres");
    db_config.connections[0].connection_name = strdup("pg_alias");

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    TEST_ASSERT_EQUAL(1, postgres_count);

    // Clean up
    free(db_config.connections[0].type);
    free(db_config.connections[0].connection_name);
}

// Test mixed engine types
void test_validate_database_configuration_mixed_types(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    DatabaseConfig db_config = {0};
    db_config.connection_count = 5;

    db_config.connections[0].enabled = true;
    db_config.connections[0].type = strdup("postgresql");
    db_config.connections[0].connection_name = strdup("pg1");

    db_config.connections[1].enabled = true;
    db_config.connections[1].type = strdup("mysql");
    db_config.connections[1].connection_name = strdup("mysql1");

    db_config.connections[2].enabled = true;
    db_config.connections[2].type = strdup("mariadb");
    db_config.connections[2].connection_name = strdup("maria1");

    db_config.connections[3].enabled = true;
    db_config.connections[3].type = strdup("sqlite");
    db_config.connections[3].connection_name = strdup("sqlite1");

    db_config.connections[4].enabled = true;
    db_config.connections[4].type = strdup("mssql");
    db_config.connections[4].connection_name = strdup("mssql1");

    int postgres_count, mysql_count, sqlite_count, db2_count, firebird_count, mariadb_count, mssql_count;

    validate_database_configuration(&db_config, &messages, &count, &capacity, &overall_readiness,
                                   &postgres_count, &mysql_count, &sqlite_count, &db2_count, &firebird_count, &mariadb_count, &mssql_count);

    TEST_ASSERT_EQUAL(1, postgres_count);
    TEST_ASSERT_EQUAL(1, mysql_count);
    TEST_ASSERT_EQUAL(1, mariadb_count);
    TEST_ASSERT_EQUAL(1, sqlite_count);
    TEST_ASSERT_EQUAL(0, db2_count);
    TEST_ASSERT_EQUAL(0, firebird_count);
    TEST_ASSERT_EQUAL(1, mssql_count);
    TEST_ASSERT_TRUE(overall_readiness);

    // Clean up
    for (int i = 0; i < 5; i++) {
        free(db_config.connections[i].type);
        free(db_config.connections[i].connection_name);
    }
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

int main(void) {
    UNITY_BEGIN();

    // check_database_library_dependencies tests
    RUN_TEST(test_check_database_library_dependencies_postgres);
    RUN_TEST(test_check_database_library_dependencies_mysql);
    RUN_TEST(test_check_database_library_dependencies_db2);
    RUN_TEST(test_check_database_library_dependencies_sqlite);
    RUN_TEST(test_check_database_library_dependencies_mariadb);
    RUN_TEST(test_check_database_library_dependencies_firebird);
    RUN_TEST(test_check_database_library_dependencies_mssql);
    RUN_TEST(test_check_database_library_dependencies_zero);

    // validate_database_configuration tests
    RUN_TEST(test_validate_database_configuration_multiple_same_type);
    RUN_TEST(test_validate_database_configuration_multiple_postgres);
    RUN_TEST(test_validate_database_configuration_multiple_mysql);
    RUN_TEST(test_validate_database_configuration_multiple_sqlite);
    RUN_TEST(test_validate_database_configuration_multiple_db2);
    RUN_TEST(test_validate_database_configuration_multiple_mariadb);
    RUN_TEST(test_validate_database_configuration_multiple_firebird);
    RUN_TEST(test_validate_database_configuration_multiple_mssql);
    RUN_TEST(test_validate_database_configuration_truncation);
    RUN_TEST(test_validate_database_configuration_null_connection_name);
    RUN_TEST(test_validate_database_configuration_disabled_connection);
    RUN_TEST(test_validate_database_configuration_zero_connections);
    RUN_TEST(test_validate_database_configuration_unknown_type);
    RUN_TEST(test_validate_database_configuration_null_type);
    RUN_TEST(test_validate_database_configuration_postgres_alias);
    RUN_TEST(test_validate_database_configuration_mixed_types);

    return UNITY_END();
}
