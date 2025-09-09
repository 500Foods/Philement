/*
 * Unity Test File: database_engine_get
 * This file contains unit tests for database_engine_get functionality
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

#include "../../../../src/database/database.h"

// Forward declarations for functions being tested
DatabaseEngineInterface* database_engine_get(DatabaseEngine engine_type);

// Function prototypes for test functions
void test_database_engine_get_postgresql(void);
void test_database_engine_get_sqlite(void);
void test_database_engine_get_mysql(void);
void test_database_engine_get_db2(void);
void test_database_engine_get_ai(void);
void test_database_engine_get_invalid(void);

void setUp(void) {
    // Initialize engine system for testing
    database_engine_init();
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_database_engine_get_postgresql(void) {
    // Test getting PostgreSQL engine
    DatabaseEngineInterface* engine = database_engine_get(DB_ENGINE_POSTGRESQL);
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL(DB_ENGINE_POSTGRESQL, engine->engine_type);
}

void test_database_engine_get_sqlite(void) {
    // Test getting SQLite engine
    DatabaseEngineInterface* engine = database_engine_get(DB_ENGINE_SQLITE);
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL(DB_ENGINE_SQLITE, engine->engine_type);
}

void test_database_engine_get_mysql(void) {
    // Test getting MySQL engine
    DatabaseEngineInterface* engine = database_engine_get(DB_ENGINE_MYSQL);
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL(DB_ENGINE_MYSQL, engine->engine_type);
}

void test_database_engine_get_db2(void) {
    // Test getting DB2 engine
    DatabaseEngineInterface* engine = database_engine_get(DB_ENGINE_DB2);
    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL(DB_ENGINE_DB2, engine->engine_type);
}

void test_database_engine_get_ai(void) {
    // Test getting AI engine (should be NULL as it's not implemented)
    DatabaseEngineInterface* engine = database_engine_get(DB_ENGINE_AI);
    TEST_ASSERT_NULL(engine);
}

void test_database_engine_get_invalid(void) {
    // Test getting invalid engine type
    DatabaseEngineInterface* engine = database_engine_get(DB_ENGINE_MAX);
    TEST_ASSERT_NULL(engine);

    engine = database_engine_get((DatabaseEngine)-1);
    TEST_ASSERT_NULL(engine);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_engine_get_postgresql);
    RUN_TEST(test_database_engine_get_sqlite);
    RUN_TEST(test_database_engine_get_mysql);
    RUN_TEST(test_database_engine_get_db2);
    RUN_TEST(test_database_engine_get_ai);
    RUN_TEST(test_database_engine_get_invalid);

    return UNITY_END();
}