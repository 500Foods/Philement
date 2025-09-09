/*
 * Unity Test File: database_engine_register
 * This file contains unit tests for database_engine_register functionality
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

#include "../../../../src/database/database.h"

// Forward declarations for functions being tested
bool database_engine_register(DatabaseEngineInterface* engine);

// Function prototypes for test functions
void test_database_engine_register_null_engine(void);
void test_database_engine_register_invalid_engine_type(void);
void test_database_engine_register_duplicate_engine(void);

void setUp(void) {
    // Initialize engine system for testing
    database_engine_init();
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_database_engine_register_null_engine(void) {
    // Test registering null engine
    bool result = database_engine_register(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_database_engine_register_invalid_engine_type(void) {
    // Test registering engine with invalid type
    DatabaseEngineInterface invalid_engine = {
        .engine_type = DB_ENGINE_MAX,  // Invalid type
        .name = (char*)"invalid",
        .connect = NULL,
        .disconnect = NULL,
        .health_check = NULL,
        .reset_connection = NULL,
        .execute_query = NULL,
        .execute_prepared = NULL,
        .begin_transaction = NULL,
        .commit_transaction = NULL,
        .rollback_transaction = NULL,
        .prepare_statement = NULL,
        .unprepare_statement = NULL,
        .get_connection_string = NULL,
        .validate_connection_string = NULL,
        .escape_string = NULL
    };

    bool result = database_engine_register(&invalid_engine);
    TEST_ASSERT_FALSE(result);
}

void test_database_engine_register_duplicate_engine(void) {
    // Test registering an engine that already exists
    DatabaseEngineInterface* existing_engine = database_engine_get(DB_ENGINE_SQLITE);
    TEST_ASSERT_NOT_NULL(existing_engine);

    // Try to register another SQLite engine (should fail)
    DatabaseEngineInterface duplicate_engine = {
        .engine_type = DB_ENGINE_SQLITE,  // Same type as existing
        .name = (char*)"duplicate_sqlite",
        .connect = NULL,
        .disconnect = NULL,
        .health_check = NULL,
        .reset_connection = NULL,
        .execute_query = NULL,
        .execute_prepared = NULL,
        .begin_transaction = NULL,
        .commit_transaction = NULL,
        .rollback_transaction = NULL,
        .prepare_statement = NULL,
        .unprepare_statement = NULL,
        .get_connection_string = NULL,
        .validate_connection_string = NULL,
        .escape_string = NULL
    };

    bool result = database_engine_register(&duplicate_engine);
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_engine_register_null_engine);
    RUN_TEST(test_database_engine_register_invalid_engine_type);
    RUN_TEST(test_database_engine_register_duplicate_engine);

    return UNITY_END();
}