/*
 * Unity Test File: database_engine_get_by_name
 * This file contains unit tests for database_engine_get_by_name functionality
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

#include "../../../../src/database/database.h"

// Forward declarations for functions being tested
DatabaseEngineInterface* database_engine_get_by_name(const char* name);

// Function prototypes for test functions
void test_database_engine_get_by_name_postgresql(void);
void test_database_engine_get_by_name_sqlite(void);
void test_database_engine_get_by_name_mysql(void);
void test_database_engine_get_by_name_db2(void);
void test_database_engine_get_by_name_null(void);
void test_database_engine_get_by_name_empty(void);
void test_database_engine_get_by_name_invalid(void);
void test_database_engine_get_by_name_case_variations(void);

void setUp(void) {
    // Initialize engine system for testing
    database_engine_init();
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_database_engine_get_by_name_postgresql(void) {
    // Test getting PostgreSQL engine by name
    DatabaseEngineInterface* engine = database_engine_get_by_name("postgresql");
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL(DB_ENGINE_POSTGRESQL, engine->engine_type);
}

void test_database_engine_get_by_name_sqlite(void) {
    // Test getting SQLite engine by name
    DatabaseEngineInterface* engine = database_engine_get_by_name("sqlite");
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL(DB_ENGINE_SQLITE, engine->engine_type);
}

void test_database_engine_get_by_name_mysql(void) {
    // Test getting MySQL engine by name
    DatabaseEngineInterface* engine = database_engine_get_by_name("mysql");
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL(DB_ENGINE_MYSQL, engine->engine_type);
}

void test_database_engine_get_by_name_db2(void) {
    // Test getting DB2 engine by name
    DatabaseEngineInterface* engine = database_engine_get_by_name("db2");
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL(DB_ENGINE_DB2, engine->engine_type);
}

void test_database_engine_get_by_name_null(void) {
    // Test null name parameter
    DatabaseEngineInterface* engine = database_engine_get_by_name(NULL);
    TEST_ASSERT_NULL(engine);
}

void test_database_engine_get_by_name_empty(void) {
    // Test empty string
    DatabaseEngineInterface* engine = database_engine_get_by_name("");
    TEST_ASSERT_NULL(engine);
}

void test_database_engine_get_by_name_invalid(void) {
    // Test invalid engine name
    DatabaseEngineInterface* engine = database_engine_get_by_name("invalid_engine");
    TEST_ASSERT_NULL(engine);
}

void test_database_engine_get_by_name_case_variations(void) {
    // Test case variations - should be case sensitive
    DatabaseEngineInterface* engine = database_engine_get_by_name("POSTGRESQL");
    TEST_ASSERT_NULL(engine); // Should not match due to case

    engine = database_engine_get_by_name("Sqlite");
    TEST_ASSERT_NULL(engine); // Should not match due to case
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_engine_get_by_name_postgresql);
    RUN_TEST(test_database_engine_get_by_name_sqlite);
    RUN_TEST(test_database_engine_get_by_name_mysql);
    RUN_TEST(test_database_engine_get_by_name_db2);
    RUN_TEST(test_database_engine_get_by_name_null);
    RUN_TEST(test_database_engine_get_by_name_empty);
    RUN_TEST(test_database_engine_get_by_name_invalid);
    RUN_TEST(test_database_engine_get_by_name_case_variations);

    return UNITY_END();
}