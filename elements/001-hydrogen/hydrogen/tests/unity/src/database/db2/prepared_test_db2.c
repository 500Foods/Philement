/*
 * Unity Test File: DB2 Prepared Statement Functions
 * This file contains unit tests for DB2 prepared statement functions
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/db2/prepared.h>
#include <src/database/db2/connection.h>
#include <src/database/db2/types.h>

// Forward declarations for functions being tested
bool db2_add_prepared_statement(PreparedStatementCache* cache, const char* name);
bool db2_remove_prepared_statement(PreparedStatementCache* cache, const char* name);
bool db2_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
bool db2_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// Function prototypes for test functions
void test_db2_add_prepared_statement_null_cache(void);
void test_db2_add_prepared_statement_null_name(void);
void test_db2_add_prepared_statement_success(void);
void test_db2_remove_prepared_statement_null_cache(void);
void test_db2_remove_prepared_statement_null_name(void);
void test_db2_remove_prepared_statement_not_found(void);
void test_db2_prepare_statement_null_connection(void);
void test_db2_prepare_statement_null_name(void);
void test_db2_prepare_statement_null_sql(void);
void test_db2_prepare_statement_null_stmt_ptr(void);
void test_db2_prepare_statement_wrong_engine_type(void);
void test_db2_unprepare_statement_null_connection(void);
void test_db2_unprepare_statement_null_stmt(void);
void test_db2_unprepare_statement_wrong_engine_type(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test db2_add_prepared_statement
void test_db2_add_prepared_statement_null_cache(void) {
    bool result = db2_add_prepared_statement(NULL, "test");
    TEST_ASSERT_FALSE(result);
}

void test_db2_add_prepared_statement_null_name(void) {
    PreparedStatementCache* cache = db2_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    bool result = db2_add_prepared_statement(cache, NULL);
    TEST_ASSERT_FALSE(result);

    db2_destroy_prepared_statement_cache(cache);
}

void test_db2_add_prepared_statement_success(void) {
    PreparedStatementCache* cache = db2_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    bool result = db2_add_prepared_statement(cache, "test_stmt");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, cache->count);

    // Try to add the same name again (should succeed)
    result = db2_add_prepared_statement(cache, "test_stmt");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, cache->count); // Count should not increase

    db2_destroy_prepared_statement_cache(cache);
}

// Test db2_remove_prepared_statement
void test_db2_remove_prepared_statement_null_cache(void) {
    bool result = db2_remove_prepared_statement(NULL, "test");
    TEST_ASSERT_FALSE(result);
}

void test_db2_remove_prepared_statement_null_name(void) {
    PreparedStatementCache* cache = db2_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    bool result = db2_remove_prepared_statement(cache, NULL);
    TEST_ASSERT_FALSE(result);

    db2_destroy_prepared_statement_cache(cache);
}

void test_db2_remove_prepared_statement_not_found(void) {
    PreparedStatementCache* cache = db2_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    bool result = db2_remove_prepared_statement(cache, "nonexistent");
    TEST_ASSERT_FALSE(result);

    db2_destroy_prepared_statement_cache(cache);
}

// Test db2_prepare_statement
void test_db2_prepare_statement_null_connection(void) {
    PreparedStatement* stmt = NULL;
    bool result = db2_prepare_statement(NULL, "test", "SELECT 1", &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

void test_db2_prepare_statement_null_name(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    PreparedStatement* stmt = NULL;
    bool result = db2_prepare_statement(&connection, NULL, "SELECT 1", &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

void test_db2_prepare_statement_null_sql(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    PreparedStatement* stmt = NULL;
    bool result = db2_prepare_statement(&connection, "test", NULL, &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

void test_db2_prepare_statement_null_stmt_ptr(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    bool result = db2_prepare_statement(&connection, "test", "SELECT 1", NULL);
    TEST_ASSERT_FALSE(result);
}

void test_db2_prepare_statement_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    PreparedStatement* stmt = NULL;
    bool result = db2_prepare_statement(&connection, "test", "SELECT 1", &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

// Test db2_unprepare_statement
void test_db2_unprepare_statement_null_connection(void) {
    PreparedStatement stmt = {0};
    bool result = db2_unprepare_statement(NULL, &stmt);
    TEST_ASSERT_FALSE(result);
}

void test_db2_unprepare_statement_null_stmt(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    bool result = db2_unprepare_statement(&connection, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_db2_unprepare_statement_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    PreparedStatement stmt = {0};
    bool result = db2_unprepare_statement(&connection, &stmt);
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    // Test db2_add_prepared_statement
    RUN_TEST(test_db2_add_prepared_statement_null_cache);
    RUN_TEST(test_db2_add_prepared_statement_null_name);
    RUN_TEST(test_db2_add_prepared_statement_success);

    // Test db2_remove_prepared_statement
    RUN_TEST(test_db2_remove_prepared_statement_null_cache);
    RUN_TEST(test_db2_remove_prepared_statement_null_name);
    RUN_TEST(test_db2_remove_prepared_statement_not_found);

    // Test db2_prepare_statement
    RUN_TEST(test_db2_prepare_statement_null_connection);
    RUN_TEST(test_db2_prepare_statement_null_name);
    RUN_TEST(test_db2_prepare_statement_null_sql);
    RUN_TEST(test_db2_prepare_statement_null_stmt_ptr);
    RUN_TEST(test_db2_prepare_statement_wrong_engine_type);

    // Test db2_unprepare_statement
    RUN_TEST(test_db2_unprepare_statement_null_connection);
    RUN_TEST(test_db2_unprepare_statement_null_stmt);
    RUN_TEST(test_db2_unprepare_statement_wrong_engine_type);

    return UNITY_END();
}