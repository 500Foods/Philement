/*
 * Unity Test File: execute helper functions
 * This file contains unit tests for helper functions in execute.c
 * Tests: copy_sql_from_lua, count_sql_lines, execute_migration_sql
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>

// Forward declarations for functions being tested
char* copy_sql_from_lua(const char* sql_result, size_t sql_length, const char* dqm_label);
int count_sql_lines(const char* sql, size_t sql_length);
bool execute_migration_sql(DatabaseHandle* connection, const char* sql_copy, size_t sql_length,
                           const char* migration_file, const char* dqm_label);

// Forward declarations for test functions
void test_copy_sql_from_lua_null_sql(void);
void test_copy_sql_from_lua_zero_length(void);
void test_copy_sql_from_lua_simple_sql(void);
void test_copy_sql_from_lua_multiline_sql(void);
void test_copy_sql_from_lua_long_sql(void);
void test_copy_sql_from_lua_with_special_chars(void);
void test_count_sql_lines_null_sql(void);
void test_count_sql_lines_zero_length(void);
void test_count_sql_lines_single_line(void);
void test_count_sql_lines_multiple_lines(void);
void test_count_sql_lines_empty_lines(void);
void test_count_sql_lines_trailing_newline(void);
void test_count_sql_lines_many_lines(void);
void test_execute_migration_sql_null_sql(void);
void test_execute_migration_sql_zero_length(void);
void test_execute_migration_sql_empty_string(void);
void test_execute_migration_sql_valid_sql(void);
void test_copy_and_count_integration(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== copy_sql_from_lua TESTS =====

void test_copy_sql_from_lua_null_sql(void) {
    char* result = copy_sql_from_lua(NULL, 100, "test-label");
    TEST_ASSERT_NULL(result);
}

void test_copy_sql_from_lua_zero_length(void) {
    const char* sql = "SELECT * FROM test";
    char* result = copy_sql_from_lua(sql, 0, "test-label");
    TEST_ASSERT_NULL(result);
}

void test_copy_sql_from_lua_simple_sql(void) {
    const char* sql = "SELECT * FROM test;";
    size_t length = strlen(sql);
    
    char* result = copy_sql_from_lua(sql, length, "test-label");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(sql, result);
    TEST_ASSERT_EQUAL(length, strlen(result));
    
    free(result);
}

void test_copy_sql_from_lua_multiline_sql(void) {
    const char* sql = "SELECT *\nFROM test\nWHERE id = 1;";
    size_t length = strlen(sql);
    
    char* result = copy_sql_from_lua(sql, length, "test-label");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(sql, result);
    TEST_ASSERT_EQUAL(length, strlen(result));
    
    free(result);
}

void test_copy_sql_from_lua_long_sql(void) {
    // Create a long SQL statement (1000 characters)
    char sql[1001];
    memset(sql, 'X', 1000);
    sql[1000] = '\0';
    
    char* result = copy_sql_from_lua(sql, 1000, "test-label");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1000, strlen(result));
    TEST_ASSERT_EQUAL_MEMORY(sql, result, 1000);
    
    free(result);
}

void test_copy_sql_from_lua_with_special_chars(void) {
    const char* sql = "INSERT INTO test VALUES ('test\\nvalue', 'quote''s', NULL);";
    size_t length = strlen(sql);
    
    char* result = copy_sql_from_lua(sql, length, "test-label");
   
TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(sql, result);
    
    free(result);
}

// ===== count_sql_lines TESTS =====

void test_count_sql_lines_null_sql(void) {
    int count = count_sql_lines(NULL, 100);
    TEST_ASSERT_EQUAL(0, count);
}

void test_count_sql_lines_zero_length(void) {
    const char* sql = "SELECT * FROM test";
    int count = count_sql_lines(sql, 0);
    TEST_ASSERT_EQUAL(0, count);
}

void test_count_sql_lines_single_line(void) {
    const char* sql = "SELECT * FROM test;";
    int count = count_sql_lines(sql, strlen(sql));
    TEST_ASSERT_EQUAL(1, count);
}

void test_count_sql_lines_multiple_lines(void) {
    const char* sql = "SELECT *\nFROM test\nWHERE id = 1;";
    int count = count_sql_lines(sql, strlen(sql));
    TEST_ASSERT_EQUAL(3, count);  // 3 lines (2 newlines + 1)
}

void test_count_sql_lines_empty_lines(void) {
    const char* sql = "SELECT *\n\n\nFROM test;";
    int count = count_sql_lines(sql, strlen(sql));
    TEST_ASSERT_EQUAL(4, count);  // 4 lines (3 newlines + 1)
}

void test_count_sql_lines_trailing_newline(void) {
    const char* sql = "SELECT * FROM test;\n";
    int count = count_sql_lines(sql, strlen(sql));
    TEST_ASSERT_EQUAL(2, count);  // 2 lines (1 newline + 1)
}

void test_count_sql_lines_many_lines(void) {
    // Test with 100 lines
    char sql[500];
    memset(sql, 'X', 399);
    for (int i = 0; i < 99; i++) {
        sql[i * 4] = 'S';
        sql[i * 4 + 1] = 'Q';
        sql[i * 4 + 2] = 'L';
        sql[i * 4 + 3] = '\n';
    }
    sql[396] = 'E';
    sql[397] = 'N';
    sql[398] = 'D';
    sql[399] = '\0';
    
    int count = count_sql_lines(sql, strlen(sql));
    TEST_ASSERT_EQUAL(100, count);  // 100 lines (99 newlines + 1)
}

// ===== execute_migration_sql TESTS =====

void test_execute_migration_sql_null_sql(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    bool result = execute_migration_sql(&connection, NULL, 100, "test.lua", "test-label");
    TEST_ASSERT_FALSE(result);  // Should return false for NULL SQL
}

void test_execute_migration_sql_zero_length(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    const char* sql = "SELECT * FROM test;";
    
    bool result = execute_migration_sql(&connection, sql, 0, "test.lua", "test-label");
    TEST_ASSERT_FALSE(result);  // Should return false for zero length
}

void test_execute_migration_sql_empty_string(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    bool result = execute_migration_sql(&connection, "", 0, "test.lua", "test-label");
    TEST_ASSERT_FALSE(result);  // Should return false for empty string
}

void test_execute_migration_sql_valid_sql(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    const char* sql = "CREATE TABLE test (id INTEGER);";
    
    // This will use mocked execute_transaction which returns true by default
    bool result = execute_migration_sql(&connection, sql, strlen(sql), "test.lua", "test-label");
    
    // With mock, transaction succeeds
    TEST_ASSERT_TRUE(result);
}

// ===== INTEGRATION TESTS =====

void test_copy_and_count_integration(void) {
    const char* original_sql = "SELECT *\nFROM test\nWHERE id = 1;";
    size_t length = strlen(original_sql);
    
    // Copy the SQL
    char* copied_sql = copy_sql_from_lua(original_sql, length, "test-label");
    TEST_ASSERT_NOT_NULL(copied_sql);
    
    // Count lines in copied SQL
    int line_count = count_sql_lines(copied_sql, length);
    TEST_ASSERT_EQUAL(3, line_count);
    
    // Verify content
    TEST_ASSERT_EQUAL_STRING(original_sql, copied_sql);
    
    free(copied_sql);
}

int main(void) {
    UNITY_BEGIN();
    
    // copy_sql_from_lua tests
    RUN_TEST(test_copy_sql_from_lua_null_sql);
    RUN_TEST(test_copy_sql_from_lua_zero_length);
    RUN_TEST(test_copy_sql_from_lua_simple_sql);
    RUN_TEST(test_copy_sql_from_lua_multiline_sql);
    RUN_TEST(test_copy_sql_from_lua_long_sql);
    RUN_TEST(test_copy_sql_from_lua_with_special_chars);
    
    // count_sql_lines tests
    RUN_TEST(test_count_sql_lines_null_sql);
    RUN_TEST(test_count_sql_lines_zero_length);
    RUN_TEST(test_count_sql_lines_single_line);
    RUN_TEST(test_count_sql_lines_multiple_lines);
    RUN_TEST(test_count_sql_lines_empty_lines);
    RUN_TEST(test_count_sql_lines_trailing_newline);
    RUN_TEST(test_count_sql_lines_many_lines);
    
    // execute_migration_sql tests
    RUN_TEST(test_execute_migration_sql_null_sql);
    RUN_TEST(test_execute_migration_sql_zero_length);
    RUN_TEST(test_execute_migration_sql_empty_string);
    RUN_TEST(test_execute_migration_sql_valid_sql);
    
    // Integration tests
    RUN_TEST(test_copy_and_count_integration);
    
    return UNITY_END();
}