/*
 * Unity Test File: database_engine_build_connection_string
 * This file contains unit tests for database_engine_build_connection_string functionality
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

#include "../../../../src/database/database.h"

// Forward declarations for functions being tested
char* database_engine_build_connection_string(DatabaseEngine engine_type, ConnectionConfig* config);
bool database_engine_validate_connection_string(DatabaseEngine engine_type, const char* connection_string);

// Function prototypes for test functions
void test_database_engine_build_connection_string_sqlite(void);
void test_database_engine_build_connection_string_db2(void);
void test_database_engine_build_connection_string_null_config(void);
void test_database_engine_build_connection_string_invalid_engine(void);
void test_database_engine_validate_connection_string_sqlite(void);
void test_database_engine_validate_connection_string_db2(void);
void test_database_engine_validate_connection_string_null(void);

void setUp(void) {
    // Initialize engine system for testing
    database_engine_init();
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_database_engine_build_connection_string_sqlite(void) {
    // Test SQLite connection string building
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.db");

    char* result = database_engine_build_connection_string(DB_ENGINE_SQLITE, &config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/tmp/test.db", result);

    free(result);
    free((char*)config.database);
}

void test_database_engine_build_connection_string_db2(void) {
    // Test DB2 connection string building
    ConnectionConfig config = {0};
    config.database = strdup("TESTDB");

    char* result = database_engine_build_connection_string(DB_ENGINE_DB2, &config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("TESTDB", result);

    free(result);
    free((char*)config.database);
}

void test_database_engine_build_connection_string_null_config(void) {
    // Test null config parameter
    char* result = database_engine_build_connection_string(DB_ENGINE_SQLITE, NULL);
    TEST_ASSERT_NULL(result);
}

void test_database_engine_build_connection_string_invalid_engine(void) {
    // Test invalid engine type
    ConnectionConfig config = {0};
    config.database = strdup("test");

    char* result = database_engine_build_connection_string(DB_ENGINE_AI, &config);
    TEST_ASSERT_NULL(result);

    free((char*)config.database);
}

void test_database_engine_validate_connection_string_sqlite(void) {
    // Test SQLite connection string validation
    bool result = database_engine_validate_connection_string(DB_ENGINE_SQLITE, "/tmp/test.db");
    TEST_ASSERT_TRUE(result);

    result = database_engine_validate_connection_string(DB_ENGINE_SQLITE, ":memory:");
    TEST_ASSERT_TRUE(result);
}

void test_database_engine_validate_connection_string_db2(void) {
    // Test DB2 connection string validation
    bool result = database_engine_validate_connection_string(DB_ENGINE_DB2, "TESTDB");
    TEST_ASSERT_TRUE(result);

    result = database_engine_validate_connection_string(DB_ENGINE_DB2, "SAMPLE");
    TEST_ASSERT_TRUE(result);
}

void test_database_engine_validate_connection_string_null(void) {
    // Test null connection string
    bool result = database_engine_validate_connection_string(DB_ENGINE_SQLITE, NULL);
    TEST_ASSERT_FALSE(result);

    result = database_engine_validate_connection_string(DB_ENGINE_SQLITE, "");
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_engine_build_connection_string_sqlite);
    RUN_TEST(test_database_engine_build_connection_string_db2);
    RUN_TEST(test_database_engine_build_connection_string_null_config);
    RUN_TEST(test_database_engine_build_connection_string_invalid_engine);
    RUN_TEST(test_database_engine_validate_connection_string_sqlite);
    RUN_TEST(test_database_engine_validate_connection_string_db2);
    RUN_TEST(test_database_engine_validate_connection_string_null);

    return UNITY_END();
}