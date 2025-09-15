/*
 * Unity Test File: SQLite Connection Functions
 * This file contains unit tests for SQLite connection functions
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../../src/database/database.h"
#include "../../../../../src/database/sqlite/types.h"
#include "../../../../../src/database/sqlite/connection.h"

// Forward declarations for functions being tested
bool load_libsqlite_functions(void);
PreparedStatementCache* sqlite_create_prepared_statement_cache(void);
void sqlite_destroy_prepared_statement_cache(PreparedStatementCache* cache);
bool sqlite_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool sqlite_disconnect(DatabaseHandle* connection);
bool sqlite_health_check(DatabaseHandle* connection);
bool sqlite_reset_connection(DatabaseHandle* connection);

// Function prototypes for test functions
void test_load_libsqlite_functions(void);
void test_sqlite_create_prepared_statement_cache(void);
void test_sqlite_destroy_prepared_statement_cache_null(void);
void test_sqlite_destroy_prepared_statement_cache_valid(void);
void test_sqlite_connect_null_config(void);
void test_sqlite_connect_null_connection_ptr(void);
void test_sqlite_disconnect_null_connection(void);
void test_sqlite_disconnect_wrong_engine_type(void);
void test_sqlite_health_check_null_connection(void);
void test_sqlite_health_check_wrong_engine_type(void);
void test_sqlite_reset_connection_null_connection(void);
void test_sqlite_reset_connection_wrong_engine_type(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test load_libsqlite_functions
void test_load_libsqlite_functions(void) {
    // This function can be called multiple times safely
    // Result depends on whether SQLite library is available
    // We just test that it doesn't crash
    load_libsqlite_functions();
    TEST_PASS();
}

// Test sqlite_create_prepared_statement_cache
void test_sqlite_create_prepared_statement_cache(void) {
    PreparedStatementCache* cache = sqlite_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_NOT_NULL(cache->names);
    TEST_ASSERT_EQUAL(16, cache->capacity);
    TEST_ASSERT_EQUAL(0, cache->count);

    // Clean up
    sqlite_destroy_prepared_statement_cache(cache);
}

// Test sqlite_destroy_prepared_statement_cache
void test_sqlite_destroy_prepared_statement_cache_null(void) {
    // Should handle NULL gracefully
    sqlite_destroy_prepared_statement_cache(NULL);
    TEST_PASS(); // If we reach here, it didn't crash
}

void test_sqlite_destroy_prepared_statement_cache_valid(void) {
    PreparedStatementCache* cache = sqlite_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);

    // Should handle valid cache gracefully
    sqlite_destroy_prepared_statement_cache(cache);
    TEST_PASS(); // If we reach here, it didn't crash
}

// Test sqlite_connect
void test_sqlite_connect_null_config(void) {
    DatabaseHandle* connection = NULL;
    bool result = sqlite_connect(NULL, &connection, "test");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(connection);
}

void test_sqlite_connect_null_connection_ptr(void) {
    ConnectionConfig config = {0};
    bool result = sqlite_connect(&config, NULL, "test");
    TEST_ASSERT_FALSE(result);
}

// Test sqlite_disconnect
void test_sqlite_disconnect_null_connection(void) {
    bool result = sqlite_disconnect(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_sqlite_disconnect_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL; // Wrong engine type
    bool result = sqlite_disconnect(&connection);
    TEST_ASSERT_FALSE(result);
}

// Test sqlite_health_check
void test_sqlite_health_check_null_connection(void) {
    bool result = sqlite_health_check(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_sqlite_health_check_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL; // Wrong engine type
    bool result = sqlite_health_check(&connection);
    TEST_ASSERT_FALSE(result);
}

// Test sqlite_reset_connection
void test_sqlite_reset_connection_null_connection(void) {
    bool result = sqlite_reset_connection(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_sqlite_reset_connection_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MYSQL; // Wrong engine type
    bool result = sqlite_reset_connection(&connection);
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    // Test load_libsqlite_functions
    RUN_TEST(test_load_libsqlite_functions);

    // Test sqlite_create_prepared_statement_cache
    RUN_TEST(test_sqlite_create_prepared_statement_cache);

    // Test sqlite_destroy_prepared_statement_cache
    RUN_TEST(test_sqlite_destroy_prepared_statement_cache_null);
    RUN_TEST(test_sqlite_destroy_prepared_statement_cache_valid);

    // Test sqlite_connect
    RUN_TEST(test_sqlite_connect_null_config);
    RUN_TEST(test_sqlite_connect_null_connection_ptr);

    // Test sqlite_disconnect
    RUN_TEST(test_sqlite_disconnect_null_connection);
    RUN_TEST(test_sqlite_disconnect_wrong_engine_type);

    // Test sqlite_health_check
    RUN_TEST(test_sqlite_health_check_null_connection);
    RUN_TEST(test_sqlite_health_check_wrong_engine_type);

    // Test sqlite_reset_connection
    RUN_TEST(test_sqlite_reset_connection_null_connection);
    RUN_TEST(test_sqlite_reset_connection_wrong_engine_type);

    return UNITY_END();
}