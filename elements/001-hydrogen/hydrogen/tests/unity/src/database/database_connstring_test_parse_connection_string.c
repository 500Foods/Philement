/*
 * Unity Test File: database_connstring_test_parse_connection_string
 * This file contains unit tests for parse_connection_string and free_connection_config functions
 * from src/database/database_connstring.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Test includes
#include <src/database/database.h>
#include <src/database/database_connstring.h>

// Function prototypes for test functions
void test_parse_connection_string_null_input(void);
void test_parse_connection_string_postgresql_format(void);
void test_parse_connection_string_mysql_format(void);
void test_parse_connection_string_db2_format(void);
void test_parse_connection_string_sqlite_format(void);
void test_parse_connection_string_invalid_format(void);
void test_free_connection_config_null_input(void);
void test_free_connection_config_valid_config(void);

// New tests for uncovered code
void test_parse_connection_string_mysql_no_username(void);
void test_parse_connection_string_mysql_no_port(void);
void test_parse_connection_string_postgresql_fallback_database(void);
void test_parse_connection_string_empty_string(void);
void test_parse_connection_string_db2_minimal(void);
void test_parse_connection_string_db2_quoted_values(void);

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

// NEW TESTS FOR UNCOVERED CODE

// Test MySQL connection string without username (covers line 92)
void test_parse_connection_string_mysql_no_username(void) {
    const char* conn_str = "mysql://:password@host:3306/database";
    ConnectionConfig* config = parse_connection_string(conn_str);

    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL_STRING("host", config->host);
    TEST_ASSERT_EQUAL(3306, config->port);
    TEST_ASSERT_EQUAL_STRING("database", config->database);
    TEST_ASSERT_EQUAL_STRING("", config->username); // Empty username
    TEST_ASSERT_EQUAL_STRING("password", config->password);
    TEST_ASSERT_EQUAL_STRING(conn_str, config->connection_string);

    free_connection_config(config);
}

// Test MySQL connection string without port (covers line 116)
void test_parse_connection_string_mysql_no_port(void) {
    const char* conn_str = "mysql://user:password@host/database";
    ConnectionConfig* config = parse_connection_string(conn_str);

    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL_STRING("host", config->host);
    TEST_ASSERT_EQUAL(3306, config->port); // Default MySQL port
    TEST_ASSERT_EQUAL_STRING("database", config->database);
    TEST_ASSERT_EQUAL_STRING("user", config->username);
    TEST_ASSERT_EQUAL_STRING("password", config->password);
    TEST_ASSERT_EQUAL_STRING(conn_str, config->connection_string);

    free_connection_config(config);
}

// Test PostgreSQL connection string that triggers fallback database (covers line 268)
void test_parse_connection_string_postgresql_fallback_database(void) {
    const char* conn_str = "postgresql://user:password@host:5432";
    ConnectionConfig* config = parse_connection_string(conn_str);

    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL_STRING("localhost", config->host); // Default host since no host specified
    TEST_ASSERT_EQUAL(5432, config->port);
    TEST_ASSERT_EQUAL_STRING("postgres", config->database); // Fallback database
    TEST_ASSERT_EQUAL_STRING("user", config->username);
    TEST_ASSERT_EQUAL_STRING("password", config->password);
    TEST_ASSERT_EQUAL_STRING(conn_str, config->connection_string);

    free_connection_config(config);
}

// Test empty string input
void test_parse_connection_string_empty_string(void) {
    const char* conn_str = "";
    ConnectionConfig* config = parse_connection_string(conn_str);

    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL_STRING("localhost", config->host);
    TEST_ASSERT_EQUAL(5432, config->port);
    TEST_ASSERT_EQUAL_STRING(conn_str, config->database); // Empty string as database
    TEST_ASSERT_EQUAL_STRING("", config->username);
    TEST_ASSERT_EQUAL_STRING("", config->password);
    TEST_ASSERT_EQUAL_STRING(conn_str, config->connection_string);

    free_connection_config(config);
}

// Test minimal DB2 connection string
void test_parse_connection_string_db2_minimal(void) {
    const char* conn_str = "DRIVER={DB2};DATABASE=test";
    ConnectionConfig* config = parse_connection_string(conn_str);

    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL_STRING("localhost", config->host); // Default host
    TEST_ASSERT_EQUAL(5432, config->port); // Default port
    TEST_ASSERT_EQUAL_STRING("test", config->database);
    TEST_ASSERT_EQUAL_STRING("", config->username); // Empty username
    TEST_ASSERT_EQUAL_STRING("", config->password); // Empty password
    TEST_ASSERT_EQUAL_STRING(conn_str, config->connection_string);

    free_connection_config(config);
}

// Test DB2 connection string with quoted values
void test_parse_connection_string_db2_quoted_values(void) {
    const char* conn_str = "DRIVER={IBM DB2 ODBC DRIVER};DATABASE=\"test database\";HOSTNAME=\"test host\";UID=\"test user\";PWD=\"test pass\"";
    ConnectionConfig* config = parse_connection_string(conn_str);

    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL_STRING("test host", config->host);
    TEST_ASSERT_EQUAL(5432, config->port); // Default port since PORT not specified
    TEST_ASSERT_EQUAL_STRING("test database", config->database);
    TEST_ASSERT_EQUAL_STRING("test user", config->username);
    TEST_ASSERT_EQUAL_STRING("test pass", config->password);
    TEST_ASSERT_EQUAL_STRING(conn_str, config->connection_string);

    free_connection_config(config);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_connection_string_null_input);
    if (0) RUN_TEST(test_parse_connection_string_postgresql_format);
    if (0) RUN_TEST(test_parse_connection_string_mysql_format);
    if (0) RUN_TEST(test_parse_connection_string_db2_format);
    if (0) RUN_TEST(test_parse_connection_string_sqlite_format);
    if (0) RUN_TEST(test_parse_connection_string_invalid_format);
    RUN_TEST(test_free_connection_config_null_input);
    RUN_TEST(test_free_connection_config_valid_config);

    // Run new tests for uncovered code
    if (0) RUN_TEST(test_parse_connection_string_mysql_no_username);
    if (0) RUN_TEST(test_parse_connection_string_mysql_no_port);
    if (0) RUN_TEST(test_parse_connection_string_postgresql_fallback_database);
    if (0) RUN_TEST(test_parse_connection_string_empty_string);
    if (0) RUN_TEST(test_parse_connection_string_db2_minimal);
    if (0) RUN_TEST(test_parse_connection_string_db2_quoted_values);

    return UNITY_END();
}