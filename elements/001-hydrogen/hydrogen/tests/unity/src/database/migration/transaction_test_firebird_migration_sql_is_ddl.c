/*
 * Unity Test File: Firebird DDL Detection Tests
 * This file contains unit tests for the firebird_migration_sql_is_ddl() function
 * from src/database/migration/transaction.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/migration/migration.h>

void test_firebird_migration_sql_is_ddl_null(void);
void test_firebird_migration_sql_is_ddl_empty(void);
void test_firebird_migration_sql_is_ddl_whitespace_only(void);
void test_firebird_migration_sql_is_ddl_create(void);
void test_firebird_migration_sql_is_ddl_alter(void);
void test_firebird_migration_sql_is_ddl_drop(void);
void test_firebird_migration_sql_is_ddl_recreate(void);
void test_firebird_migration_sql_is_ddl_dml_false(void);
void test_firebird_migration_sql_is_ddl_word_boundary(void);
void test_firebird_migration_sql_is_ddl_leading_whitespace(void);
void test_firebird_migration_sql_is_ddl_line_comment_before(void);
void test_firebird_migration_sql_is_ddl_block_comment_before(void);
void test_firebird_migration_sql_is_ddl_mixed_comments_before(void);
void test_firebird_migration_sql_is_ddl_unterminated_block_comment(void);
void test_firebird_migration_sql_is_ddl_ddl_inside_comment(void);

void setUp(void) {
}

void tearDown(void) {
}

// Test NULL input
void test_firebird_migration_sql_is_ddl_null(void) {
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl(NULL));
}

// Test empty string
void test_firebird_migration_sql_is_ddl_empty(void) {
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl(""));
}

// Test whitespace only
void test_firebird_migration_sql_is_ddl_whitespace_only(void) {
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("   \n\t  \r\n  "));
}

// Test CREATE keyword at start
void test_firebird_migration_sql_is_ddl_create(void) {
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("CREATE TABLE test (id INT)"));
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("CREATE INDEX idx ON test(id)"));
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("CREATE PROCEDURE proc"));
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("create table test (id INT)"));
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("CrEaTe TaBlE test (id INT)"));
}

// Test ALTER keyword
void test_firebird_migration_sql_is_ddl_alter(void) {
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("ALTER TABLE test ADD x INT"));
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("alter table test add x int"));
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("ALTER PROCEDURE proc"));
}

// Test DROP keyword
void test_firebird_migration_sql_is_ddl_drop(void) {
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("DROP TABLE test"));
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("drop table test"));
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("DROP INDEX idx"));
}

// Test RECREATE keyword
void test_firebird_migration_sql_is_ddl_recreate(void) {
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("RECREATE TABLE test (id INT)"));
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("recreate table test (id INT)"));
}

// Test non-DDL SQL (DML) should return false
void test_firebird_migration_sql_is_ddl_dml_false(void) {
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("SELECT 1"));
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("INSERT INTO test VALUES (1)"));
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("UPDATE test SET x = 1"));
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("DELETE FROM test"));
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("MERGE INTO test"));
}

// Test that word boundary detection is correct
void test_firebird_migration_sql_is_ddl_word_boundary(void) {
    // Underscore is treated as part of a word (like alnum), so CREATE_TABLE is NOT DDL
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("CREATE_TABLE test"));
    // But keywords followed by non-alnum/non-underscore characters are DDL
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("CREATE TABLE test"));
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("CREATE( TABLE test"));
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("CREATE\nTABLE test"));
    // Keywords with extra letters after are NOT DDL
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("CREATEX TABLE test"));
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("CREATED TABLE test"));
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("ALTERED TABLE test"));
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("DROPS TABLE test"));
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("RECREATION test"));
}

// Test leading whitespace is skipped
void test_firebird_migration_sql_is_ddl_leading_whitespace(void) {
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("   CREATE TABLE test (id INT)"));
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("\n\t  CREATE TABLE test (id INT)"));
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("   SELECT 1"));
}

// Test leading line comments are skipped
void test_firebird_migration_sql_is_ddl_line_comment_before(void) {
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("-- comment line\nCREATE TABLE test (id INT)"));
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("-- comment line\nSELECT 1"));
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("-- first\n-- second\nCREATE TABLE test"));
}

// Test leading block comments are skipped
void test_firebird_migration_sql_is_ddl_block_comment_before(void) {
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("/* block comment */CREATE TABLE test (id INT)"));
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("/* block\ncomment\nmulti-line */ALTER TABLE test ADD x INT"));
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("/* block comment */SELECT 1"));
}

// Test multiple comments before keyword
void test_firebird_migration_sql_is_ddl_mixed_comments_before(void) {
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("-- line comment\n/* block comment */\nCREATE TABLE test"));
    TEST_ASSERT_TRUE(firebird_migration_sql_is_ddl("/* block */-- line\nCREATE TABLE test"));
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("-- line\n/* block */SELECT 1"));
}

// Test unterminated block comment (should consume to end, no DDL found)
void test_firebird_migration_sql_is_ddl_unterminated_block_comment(void) {
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("/* never closed CREATE TABLE test"));
}

// Test comment-only content with DDL-like text inside
void test_firebird_migration_sql_is_ddl_ddl_inside_comment(void) {
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("-- CREATE TABLE test"));
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("/* CREATE TABLE test */"));
    TEST_ASSERT_FALSE(firebird_migration_sql_is_ddl("/* CREATE\nTABLE test */"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_firebird_migration_sql_is_ddl_null);
    RUN_TEST(test_firebird_migration_sql_is_ddl_empty);
    RUN_TEST(test_firebird_migration_sql_is_ddl_whitespace_only);
    RUN_TEST(test_firebird_migration_sql_is_ddl_create);
    RUN_TEST(test_firebird_migration_sql_is_ddl_alter);
    RUN_TEST(test_firebird_migration_sql_is_ddl_drop);
    RUN_TEST(test_firebird_migration_sql_is_ddl_recreate);
    RUN_TEST(test_firebird_migration_sql_is_ddl_dml_false);
    RUN_TEST(test_firebird_migration_sql_is_ddl_word_boundary);
    RUN_TEST(test_firebird_migration_sql_is_ddl_leading_whitespace);
    RUN_TEST(test_firebird_migration_sql_is_ddl_line_comment_before);
    RUN_TEST(test_firebird_migration_sql_is_ddl_block_comment_before);
    RUN_TEST(test_firebird_migration_sql_is_ddl_mixed_comments_before);
    RUN_TEST(test_firebird_migration_sql_is_ddl_unterminated_block_comment);
    RUN_TEST(test_firebird_migration_sql_is_ddl_ddl_inside_comment);

    return UNITY_END();
}
