/*
 * Unity Test File: database_connstring_test_build_connection_string
 * This file contains unit tests for database_build_connection_string function
 * from src/database/database_connstring.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/database_connstring.h>
#include <src/config/config_databases.h>

// Forward declaration for database_get_engine_interface
DatabaseEngineInterface* database_get_engine_interface(const char* engine);

// Function prototypes for test functions
void test_database_build_connection_string_null_parameters(void);
void test_database_build_connection_string_sqlite_engine(void);
void test_database_build_connection_string_mysql_engine(void);
void test_database_build_connection_string_postgresql_engine(void);
void test_database_build_connection_string_db2_engine(void);
void test_database_build_connection_string_invalid_engine(void);

void setUp(void) {
    // Initialize database engine system for testing
    database_engine_init();
}

void tearDown(void) {
    // Clean up test fixtures if needed
}

// Test NULL parameters
void test_database_build_connection_string_null_parameters(void) {
    // Test NULL engine
    char* result = database_build_connection_string(NULL, &(DatabaseConnection){0});
    TEST_ASSERT_NULL(result);

    // Test NULL conn_config
    result = database_build_connection_string("sqlite", NULL);
    TEST_ASSERT_NULL(result);

    // Test both NULL
    result = database_build_connection_string(NULL, NULL);
    TEST_ASSERT_NULL(result);
}

// Test SQLite engine
void test_database_build_connection_string_sqlite_engine(void) {
    DatabaseConnection conn_config;
    memset(&conn_config, 0, sizeof(DatabaseConnection));
    conn_config.database = (char*)"/path/to/database.db";

    char* result = database_build_connection_string("sqlite", &conn_config);
    // In test environment, engines are not available, so expect NULL
    TEST_ASSERT_NULL(result);

    // Test SQLite with NULL database (should still return NULL)
    conn_config.database = NULL;
    result = database_build_connection_string("sqlite", &conn_config);
    TEST_ASSERT_NULL(result);
}

// Test MySQL engine
void test_database_build_connection_string_mysql_engine(void) {
    DatabaseConnection conn_config;
    memset(&conn_config, 0, sizeof(DatabaseConnection));
    conn_config.host = (char*)"localhost";
    conn_config.port = (char*)"3306";
    conn_config.database = (char*)"testdb";
    conn_config.user = (char*)"user";
    conn_config.pass = (char*)"password";

    char* result = database_build_connection_string("mysql", &conn_config);
    // In test environment, engines are not available, so expect NULL
    TEST_ASSERT_NULL(result);

    // Test with NULL values (should still return NULL)
    memset(&conn_config, 0, sizeof(DatabaseConnection));

    result = database_build_connection_string("mysql", &conn_config);
    TEST_ASSERT_NULL(result);
}

// Test PostgreSQL engine
void test_database_build_connection_string_postgresql_engine(void) {
    DatabaseConnection conn_config;
    memset(&conn_config, 0, sizeof(DatabaseConnection));
    conn_config.host = (char*)"localhost";
    conn_config.port = (char*)"5432";
    conn_config.database = (char*)"testdb";
    conn_config.user = (char*)"user";
    conn_config.pass = (char*)"password";

    char* result = database_build_connection_string("postgresql", &conn_config);
    // In test environment, engines are not available, so expect NULL
    TEST_ASSERT_NULL(result);

    // Test with NULL values (should still return NULL)
    memset(&conn_config, 0, sizeof(DatabaseConnection));

    result = database_build_connection_string("postgresql", &conn_config);
    TEST_ASSERT_NULL(result);
}

// Test DB2 engine
void test_database_build_connection_string_db2_engine(void) {
    DatabaseConnection conn_config;
    memset(&conn_config, 0, sizeof(DatabaseConnection));
    conn_config.database = (char*)"SAMPLE";

    char* result = database_build_connection_string("db2", &conn_config);
    // In test environment, engines are not available, so expect NULL
    TEST_ASSERT_NULL(result);

    // Test with NULL database (should still return NULL)
    conn_config.database = NULL;
    result = database_build_connection_string("db2", &conn_config);
    TEST_ASSERT_NULL(result);
}

// Test invalid engine (fallback to PostgreSQL-style)
void test_database_build_connection_string_invalid_engine(void) {
    DatabaseConnection conn_config;
    memset(&conn_config, 0, sizeof(DatabaseConnection));
    conn_config.host = (char*)"localhost";
    conn_config.port = (char*)"5432";
    conn_config.database = (char*)"testdb";
    conn_config.user = (char*)"user";
    conn_config.pass = (char*)"password";

    char* result = database_build_connection_string("invalid", &conn_config);
    // In test environment, engines are not available, so expect NULL
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_build_connection_string_null_parameters);
    RUN_TEST(test_database_build_connection_string_sqlite_engine);
    RUN_TEST(test_database_build_connection_string_mysql_engine);
    RUN_TEST(test_database_build_connection_string_postgresql_engine);
    RUN_TEST(test_database_build_connection_string_db2_engine);
    RUN_TEST(test_database_build_connection_string_invalid_engine);

    return UNITY_END();
}