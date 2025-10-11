/*
 * Unity Test File: utils_hash_test_get_stmt_hash
 * This file contains unit tests for the get_stmt_hash() function in utils_hash.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include the header for the function being tested
#include <src/utils/utils_hash.h>

// Forward declarations for functions being tested
void get_stmt_hash(const char* prefix, const char* statement, size_t length, char* output_buf);

// Forward declarations for test functions
void test_get_stmt_hash_basic_functionality(void);
void test_get_stmt_hash_null_prefix(void);
void test_get_stmt_hash_empty_statement(void);
void test_get_stmt_hash_consistency(void);
void test_get_stmt_hash_different_prefixes(void);
void test_get_stmt_hash_different_statements(void);
void test_get_stmt_hash_different_lengths(void);
void test_get_stmt_hash_max_length(void);
void test_get_stmt_hash_null_statement(void);
void test_get_stmt_hash_zero_length(void);
void test_get_stmt_hash_null_output_buffer(void);
void test_get_stmt_hash_complex_sql(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test basic functionality with valid inputs
void test_get_stmt_hash_basic_functionality(void) {
    char hash_buffer[64];

    // Test with prefix and statement
    get_stmt_hash("TEST", "SELECT * FROM users", 16, hash_buffer);
    TEST_ASSERT_NOT_NULL(hash_buffer);
    TEST_ASSERT_EQUAL_STRING_LEN("TEST", hash_buffer, 4);  // Should start with prefix

    // Verify hash is uppercase hexadecimal
    for (size_t i = 7; i < strlen(hash_buffer); i++) {
        TEST_ASSERT_TRUE((hash_buffer[i] >= '0' && hash_buffer[i] <= '9') ||
                        (hash_buffer[i] >= 'A' && hash_buffer[i] <= 'F'));
    }
}

// Test with NULL prefix
void test_get_stmt_hash_null_prefix(void) {
    char hash_buffer[64];

    get_stmt_hash(NULL, "SELECT * FROM users", 16, hash_buffer);
    TEST_ASSERT_NOT_NULL(hash_buffer);
    TEST_ASSERT_TRUE(strlen(hash_buffer) == 16);  // Should be just the 16-character hash
}

// Test with empty statement
void test_get_stmt_hash_empty_statement(void) {
    char hash_buffer[64];

    get_stmt_hash("TEST", "", 16, hash_buffer);
    TEST_ASSERT_NOT_NULL(hash_buffer);
    TEST_ASSERT_EQUAL_STRING_LEN("TEST", hash_buffer, 4);

    // Hash of empty string should be consistent
    char hash_buffer2[64];
    get_stmt_hash("TEST", "", 16, hash_buffer2);
    TEST_ASSERT_EQUAL_STRING(hash_buffer, hash_buffer2);
}

// Test hash consistency - same inputs should produce same hash
void test_get_stmt_hash_consistency(void) {
    char hash_buffer1[64];
    char hash_buffer2[64];

    const char* test_sql = "INSERT INTO logs VALUES (?, ?)";

    get_stmt_hash("MIGRATION", test_sql, 16, hash_buffer1);
    get_stmt_hash("MIGRATION", test_sql, 16, hash_buffer2);

    TEST_ASSERT_EQUAL_STRING(hash_buffer1, hash_buffer2);
}

// Test different prefixes produce different results
void test_get_stmt_hash_different_prefixes(void) {
    char hash_buffer1[64];
    char hash_buffer2[64];

    const char* test_sql = "UPDATE users SET name = ? WHERE id = ?";

    get_stmt_hash("PREFIX1", test_sql, 16, hash_buffer1);
    get_stmt_hash("PREFIX2", test_sql, 16, hash_buffer2);

    TEST_ASSERT_NOT_NULL(hash_buffer1);
    TEST_ASSERT_NOT_NULL(hash_buffer2);
    TEST_ASSERT_TRUE(strlen(hash_buffer1) > 0);
    TEST_ASSERT_TRUE(strlen(hash_buffer2) > 0);
    // Should be different due to different prefixes
    TEST_ASSERT_TRUE(strcmp(hash_buffer1, hash_buffer2) != 0);
}

// Test different statements produce different hashes
void test_get_stmt_hash_different_statements(void) {
    char hash_buffer1[64];
    char hash_buffer2[64];

    get_stmt_hash("TEST", "SELECT * FROM users", 16, hash_buffer1);
    get_stmt_hash("TEST", "SELECT * FROM products", 16, hash_buffer2);

    TEST_ASSERT_NOT_NULL(hash_buffer1);
    TEST_ASSERT_NOT_NULL(hash_buffer2);
    TEST_ASSERT_TRUE(strlen(hash_buffer1) > 0);
    TEST_ASSERT_TRUE(strlen(hash_buffer2) > 0);
    // Should be different due to different statements
    TEST_ASSERT_TRUE(strcmp(hash_buffer1, hash_buffer2) != 0);
}

// Test different hash lengths
void test_get_stmt_hash_different_lengths(void) {
    char hash_buffer1[64];
    char hash_buffer2[64];
    char hash_buffer3[64];

    const char* test_sql = "DELETE FROM sessions WHERE expired < ?";

    get_stmt_hash("TEST", test_sql, 8, hash_buffer1);
    get_stmt_hash("TEST", test_sql, 12, hash_buffer2);
    get_stmt_hash("TEST", test_sql, 16, hash_buffer3);

    // All should start with the same prefix
    TEST_ASSERT_EQUAL_STRING_LEN("TEST", hash_buffer1, 4);
    TEST_ASSERT_EQUAL_STRING_LEN("TEST", hash_buffer2, 4);
    TEST_ASSERT_EQUAL_STRING_LEN("TEST", hash_buffer3, 4);

    // Different lengths should produce different results
    TEST_ASSERT_TRUE(strlen(hash_buffer1) != strlen(hash_buffer2) ||
                    strcmp(hash_buffer1, hash_buffer2) != 0);
    TEST_ASSERT_TRUE(strlen(hash_buffer2) != strlen(hash_buffer3) ||
                    strcmp(hash_buffer2, hash_buffer3) != 0);
}

// Test maximum hash length (16 characters)
void test_get_stmt_hash_max_length(void) {
    char hash_buffer[64];

    get_stmt_hash("TEST", "SELECT * FROM very_long_table_name", 16, hash_buffer);

    // Should start with prefix
    TEST_ASSERT_EQUAL_STRING_LEN("TEST", hash_buffer, 4);

    // Total length should be prefix(4) + hash(16) + null = 21
    // "TEST" = 4, hash = 16, null = 1, total = 21
    TEST_ASSERT_EQUAL(21, strlen(hash_buffer));
}

// Test NULL statement (should not crash)
void test_get_stmt_hash_null_statement(void) {
    char hash_buffer[64];

    // This should handle NULL gracefully (return early)
    get_stmt_hash("TEST", NULL, 16, hash_buffer);

    // Buffer should remain unchanged or be safely handled
    // (The function returns early, so buffer content is undefined but shouldn't crash)
}

// Test zero length (should not crash)
void test_get_stmt_hash_zero_length(void) {
    char hash_buffer[64];

    // This should handle zero length gracefully (return early)
    get_stmt_hash("TEST", "SELECT 1", 0, hash_buffer);

    // Buffer should remain unchanged or be safely handled
    // (The function returns early, so buffer content is undefined but shouldn't crash)
}

// Test NULL output buffer (should not crash)
void test_get_stmt_hash_null_output_buffer(void) {
    // This should handle NULL output buffer gracefully (return early)
    get_stmt_hash("TEST", "SELECT 1", 16, NULL);

    // Should not crash
}

// Test complex SQL statement
void test_get_stmt_hash_complex_sql(void) {
    char hash_buffer[64];

    const char* complex_sql =
        "SELECT u.name, COUNT(o.id) as order_count "
        "FROM users u "
        "LEFT JOIN orders o ON u.id = o.user_id "
        "WHERE u.created_date >= ? AND u.status = ? "
        "GROUP BY u.id, u.name "
        "HAVING COUNT(o.id) > ? "
        "ORDER BY order_count DESC";

    get_stmt_hash("COMPLEX", complex_sql, 16, hash_buffer);

    TEST_ASSERT_NOT_NULL(hash_buffer);
    TEST_ASSERT_EQUAL_STRING_LEN("COMPLEX", hash_buffer, 7);  // Should start with "COMPLEX"
    TEST_ASSERT_TRUE(strlen(hash_buffer) > 7);  // Should have hash appended
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_stmt_hash_basic_functionality);
    RUN_TEST(test_get_stmt_hash_null_prefix);
    RUN_TEST(test_get_stmt_hash_consistency);
    RUN_TEST(test_get_stmt_hash_different_statements);

    return UNITY_END();
}