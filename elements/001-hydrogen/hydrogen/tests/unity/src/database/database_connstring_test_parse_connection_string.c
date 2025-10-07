/*
 * Unity Test File: database_connstring_test_parse_connection_string
 * This file contains unit tests for parse_connection_string and free_connection_config functions
 * from src/database/database_connstring.c
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

#include "../../../../src/database/database.h"
#include "../../../../src/database/database_connstring.h"

// Function prototypes for test functions
void test_parse_connection_string_null_input(void);
void test_parse_connection_string_postgresql_format(void);
void test_parse_connection_string_mysql_format(void);
void test_parse_connection_string_db2_format(void);
void test_parse_connection_string_sqlite_format(void);
void test_parse_connection_string_invalid_format(void);
void test_free_connection_config_null_input(void);
void test_free_connection_config_valid_config(void);

void setUp(void) {
    // Set up test fixtures if needed
}

void tearDown(void) {
    // Clean up test fixtures if needed
}

// Test NULL input
void test_parse_connection_string_null_input(void) {
    ConnectionConfig* config = parse_connection_string(NULL);
    TEST_ASSERT_NULL(config);
}

// Test PostgreSQL connection string format
void test_parse_connection_string_postgresql_format(void) {
    const char* conn_str = "postgresql://user:password@host:5432/database";
    ConnectionConfig* config = parse_connection_string(conn_str);

    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL_STRING("host", config->host);
    TEST_ASSERT_EQUAL(5432, config->port);
    TEST_ASSERT_EQUAL_STRING("database", config->database);
    TEST_ASSERT_EQUAL_STRING("user", config->username);
    TEST_ASSERT_EQUAL_STRING("password", config->password);
    TEST_ASSERT_EQUAL_STRING(conn_str, config->connection_string);

    free_connection_config(config);
}

// Test MySQL connection string format
void test_parse_connection_string_mysql_format(void) {
    const char* conn_str = "mysql://user:password@host:3306/database";
    ConnectionConfig* config = parse_connection_string(conn_str);

    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL_STRING("host", config->host);
    TEST_ASSERT_EQUAL(3306, config->port);
    TEST_ASSERT_EQUAL_STRING("database", config->database);
    TEST_ASSERT_EQUAL_STRING("user", config->username);
    TEST_ASSERT_EQUAL_STRING("password", config->password);
    TEST_ASSERT_EQUAL_STRING(conn_str, config->connection_string);

    free_connection_config(config);
}

// Test DB2 connection string format
void test_parse_connection_string_db2_format(void) {
    const char* conn_str = "DRIVER={IBM DB2 ODBC DRIVER};DATABASE=testdb;HOSTNAME=host;PORT=50000;UID=user;PWD=password";
    ConnectionConfig* config = parse_connection_string(conn_str);

    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL_STRING("host", config->host);
    TEST_ASSERT_EQUAL(50000, config->port);
    TEST_ASSERT_EQUAL_STRING("testdb", config->database);
    TEST_ASSERT_EQUAL_STRING("user", config->username);
    TEST_ASSERT_EQUAL_STRING("password", config->password);
    TEST_ASSERT_EQUAL_STRING(conn_str, config->connection_string);

    free_connection_config(config);
}

// Test SQLite file path format
void test_parse_connection_string_sqlite_format(void) {
    const char* conn_str = "/path/to/database.db";
    ConnectionConfig* config = parse_connection_string(conn_str);

    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL_STRING("localhost", config->host);
    TEST_ASSERT_EQUAL(5432, config->port); // Default port
    TEST_ASSERT_EQUAL_STRING(conn_str, config->database);
    TEST_ASSERT_EQUAL_STRING("", config->username);
    TEST_ASSERT_EQUAL_STRING("", config->password);
    TEST_ASSERT_EQUAL_STRING(conn_str, config->connection_string);

    free_connection_config(config);
}

// Test invalid connection string format (treated as SQLite file path)
void test_parse_connection_string_invalid_format(void) {
    const char* conn_str = "invalid://format";
    ConnectionConfig* config = parse_connection_string(conn_str);

    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL_STRING("localhost", config->host);
    TEST_ASSERT_EQUAL(5432, config->port);
    TEST_ASSERT_EQUAL_STRING(conn_str, config->database); // Treated as SQLite file path
    TEST_ASSERT_EQUAL_STRING("", config->username);
    TEST_ASSERT_EQUAL_STRING("", config->password);
    TEST_ASSERT_EQUAL_STRING(conn_str, config->connection_string);

    free_connection_config(config);
}

// Test free_connection_config with NULL input
void test_free_connection_config_null_input(void) {
    // Should not crash with NULL input
    free_connection_config(NULL);
    // Test passes if no crash occurs
}

// Test free_connection_config with valid config
void test_free_connection_config_valid_config(void) {
    ConnectionConfig* config = parse_connection_string("postgresql://user:pass@host:5432/db");
    TEST_ASSERT_NOT_NULL(config);

    // Should not crash when freeing valid config
    free_connection_config(config);
    // Test passes if no crash occurs
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_connection_string_null_input);
    RUN_TEST(test_parse_connection_string_postgresql_format);
    RUN_TEST(test_parse_connection_string_mysql_format);
    RUN_TEST(test_parse_connection_string_db2_format);
    RUN_TEST(test_parse_connection_string_sqlite_format);
    RUN_TEST(test_parse_connection_string_invalid_format);
    RUN_TEST(test_free_connection_config_null_input);
    RUN_TEST(test_free_connection_config_valid_config);

    return UNITY_END();
}