/*
 * Unity Test File: MySQL Prepared Statement Functions
 * This file contains unit tests for MySQL prepared statement functions
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/mysql/types.h>
#include <src/database/mysql/connection.h>
#include <src/database/mysql/prepared.h>

// Forward declarations for functions being tested
bool mysql_add_prepared_statement(PreparedStatementCache* cache, const char* name);
bool mysql_remove_prepared_statement(PreparedStatementCache* cache, const char* name);
bool mysql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
bool mysql_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// Function prototypes for test functions
void test_mysql_add_prepared_statement_null_cache(void);
void test_mysql_add_prepared_statement_null_name(void);
void test_mysql_add_prepared_statement_valid(void);
void test_mysql_remove_prepared_statement_null_cache(void);
void test_mysql_remove_prepared_statement_null_name(void);
void test_mysql_remove_prepared_statement_not_found(void);
void test_mysql_remove_prepared_statement_valid(void);
void test_mysql_prepare_statement_null_connection(void);
void test_mysql_prepare_statement_null_name(void);
void test_mysql_prepare_statement_null_sql(void);
void test_mysql_prepare_statement_null_stmt_ptr(void);
void test_mysql_prepare_statement_wrong_engine_type(void);
void test_mysql_unprepare_statement_null_connection(void);
void test_mysql_unprepare_statement_null_stmt(void);
void test_mysql_unprepare_statement_wrong_engine_type(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test mysql_add_prepared_statement
void test_mysql_add_prepared_statement_null_cache(void) {
    bool result = mysql_add_prepared_statement(NULL, "test");
    TEST_ASSERT_FALSE(result);
}

void test_mysql_add_prepared_statement_null_name(void) {
    PreparedStatementCache cache = {0};
    bool result = mysql_add_prepared_statement(&cache, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_mysql_add_prepared_statement_valid(void) {
    PreparedStatementCache* cache = mysql_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    bool result = mysql_add_prepared_statement(cache, "test_stmt");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, cache->count);

    // Clean up
    mysql_destroy_prepared_statement_cache(cache);
}

// Test mysql_remove_prepared_statement
void test_mysql_remove_prepared_statement_null_cache(void) {
    bool result = mysql_remove_prepared_statement(NULL, "test");
    TEST_ASSERT_FALSE(result);
}

void test_mysql_remove_prepared_statement_null_name(void) {
    PreparedStatementCache cache = {0};
    bool result = mysql_remove_prepared_statement(&cache, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_mysql_remove_prepared_statement_not_found(void) {
    PreparedStatementCache* cache = mysql_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    bool result = mysql_remove_prepared_statement(cache, "nonexistent");
    TEST_ASSERT_FALSE(result);

    // Clean up
    mysql_destroy_prepared_statement_cache(cache);
}

void test_mysql_remove_prepared_statement_valid(void) {
    PreparedStatementCache* cache = mysql_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    // Add a statement first
    bool add_result = mysql_add_prepared_statement(cache, "test_stmt");
    TEST_ASSERT_TRUE(add_result);
    TEST_ASSERT_EQUAL(1, cache->count);

    // Now remove it
    bool remove_result = mysql_remove_prepared_statement(cache, "test_stmt");
    TEST_ASSERT_TRUE(remove_result);
    TEST_ASSERT_EQUAL(0, cache->count);

    // Clean up
    mysql_destroy_prepared_statement_cache(cache);
}

// Test mysql_prepare_statement
void test_mysql_prepare_statement_null_connection(void) {
    PreparedStatement* stmt = NULL;
    bool result = mysql_prepare_statement(NULL, "test", "SELECT 1", &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

void test_mysql_prepare_statement_null_name(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;
    PreparedStatement* stmt = NULL;
    bool result = mysql_prepare_statement(&connection, NULL, "SELECT 1", &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

void test_mysql_prepare_statement_null_sql(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;
    PreparedStatement* stmt = NULL;
    bool result = mysql_prepare_statement(&connection, "test", NULL, &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

void test_mysql_prepare_statement_null_stmt_ptr(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;
    bool result = mysql_prepare_statement(&connection, "test", "SELECT 1", NULL);
    TEST_ASSERT_FALSE(result);
}

void test_mysql_prepare_statement_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    PreparedStatement* stmt = NULL;
    bool result = mysql_prepare_statement(&connection, "test", "SELECT 1", &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

// Test mysql_unprepare_statement
void test_mysql_unprepare_statement_null_connection(void) {
    PreparedStatement stmt = {0};
    bool result = mysql_unprepare_statement(NULL, &stmt);
    TEST_ASSERT_FALSE(result);
}

void test_mysql_unprepare_statement_null_stmt(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL;
    bool result = mysql_unprepare_statement(&connection, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_mysql_unprepare_statement_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE; // Wrong engine type
    PreparedStatement stmt = {0};
    bool result = mysql_unprepare_statement(&connection, &stmt);
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    // Test mysql_add_prepared_statement
    RUN_TEST(test_mysql_add_prepared_statement_null_cache);
    RUN_TEST(test_mysql_add_prepared_statement_null_name);
    RUN_TEST(test_mysql_add_prepared_statement_valid);

    // Test mysql_remove_prepared_statement
    RUN_TEST(test_mysql_remove_prepared_statement_null_cache);
    RUN_TEST(test_mysql_remove_prepared_statement_null_name);
    RUN_TEST(test_mysql_remove_prepared_statement_not_found);
    RUN_TEST(test_mysql_remove_prepared_statement_valid);

    // Test mysql_prepare_statement
    RUN_TEST(test_mysql_prepare_statement_null_connection);
    RUN_TEST(test_mysql_prepare_statement_null_name);
    RUN_TEST(test_mysql_prepare_statement_null_sql);
    RUN_TEST(test_mysql_prepare_statement_null_stmt_ptr);
    RUN_TEST(test_mysql_prepare_statement_wrong_engine_type);

    // Test mysql_unprepare_statement
    RUN_TEST(test_mysql_unprepare_statement_null_connection);
    RUN_TEST(test_mysql_unprepare_statement_null_stmt);
    RUN_TEST(test_mysql_unprepare_statement_wrong_engine_type);

    return UNITY_END();
}