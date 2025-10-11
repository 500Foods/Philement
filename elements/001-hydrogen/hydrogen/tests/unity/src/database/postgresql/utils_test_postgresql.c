/*
 * Unity Test File: PostgreSQL Utility Functions
 * This file contains unit tests for PostgreSQL utility functions
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/postgresql/utils.h>

// Forward declarations for functions being tested
char* postgresql_get_connection_string(const ConnectionConfig* config);
bool postgresql_validate_connection_string(const char* connection_string);
char* postgresql_escape_string(const DatabaseHandle* connection, const char* input);

// Function prototypes for test functions
void test_postgresql_get_connection_string_null_config(void);
void test_postgresql_get_connection_string_with_config(void);
void test_postgresql_validate_connection_string_null(void);
void test_postgresql_validate_connection_string_empty(void);
void test_postgresql_validate_connection_string_valid(void);
void test_postgresql_validate_connection_string_invalid(void);
void test_postgresql_escape_string_null_connection(void);
void test_postgresql_escape_string_null_input(void);
void test_postgresql_escape_string_wrong_engine_type(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test postgresql_get_connection_string
void test_postgresql_get_connection_string_null_config(void) {
    char* result = postgresql_get_connection_string(NULL);
    TEST_ASSERT_NULL(result);
}

void test_postgresql_get_connection_string_with_config(void) {
    ConnectionConfig config = {0};
    config.database = strdup("testdb");
    config.host = strdup("localhost");
    config.port = 5432;
    config.username = strdup("testuser");
    config.password = strdup("testpass");

    char* result = postgresql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "testdb"));
    TEST_ASSERT_NOT_NULL(strstr(result, "localhost"));
    TEST_ASSERT_NOT_NULL(strstr(result, "5432"));
    TEST_ASSERT_NOT_NULL(strstr(result, "testuser"));
    TEST_ASSERT_NOT_NULL(strstr(result, "testpass"));

    free(result);
    free(config.database);
    free(config.host);
    free(config.username);
    free(config.password);
}

// Test postgresql_validate_connection_string
void test_postgresql_validate_connection_string_null(void) {
    bool result = postgresql_validate_connection_string(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_postgresql_validate_connection_string_empty(void) {
    bool result = postgresql_validate_connection_string("");
    TEST_ASSERT_FALSE(result);
}

void test_postgresql_validate_connection_string_valid(void) {
    bool result = postgresql_validate_connection_string("postgresql://user:pass@host:5432/db");
    TEST_ASSERT_TRUE(result);
}

void test_postgresql_validate_connection_string_invalid(void) {
    bool result = postgresql_validate_connection_string("mysql://user:pass@host:3306/db");
    TEST_ASSERT_FALSE(result);
}

// Test postgresql_escape_string
void test_postgresql_escape_string_null_connection(void) {
    char* result = postgresql_escape_string(NULL, "test");
    TEST_ASSERT_NULL(result);
}

void test_postgresql_escape_string_null_input(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    char* result = postgresql_escape_string(&connection, NULL);
    TEST_ASSERT_NULL(result);
}

void test_postgresql_escape_string_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    char* result = postgresql_escape_string(&connection, "test");
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();

    // Test postgresql_get_connection_string
    RUN_TEST(test_postgresql_get_connection_string_null_config);
    RUN_TEST(test_postgresql_get_connection_string_with_config);

    // Test postgresql_validate_connection_string
    RUN_TEST(test_postgresql_validate_connection_string_null);
    RUN_TEST(test_postgresql_validate_connection_string_empty);
    RUN_TEST(test_postgresql_validate_connection_string_valid);
    RUN_TEST(test_postgresql_validate_connection_string_invalid);

    // Test postgresql_escape_string
    RUN_TEST(test_postgresql_escape_string_null_connection);
    RUN_TEST(test_postgresql_escape_string_null_input);
    RUN_TEST(test_postgresql_escape_string_wrong_engine_type);

    return UNITY_END();
}