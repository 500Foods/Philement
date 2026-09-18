/*
 * Unity tests for firebase_sql_parse_returning().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/sql_parse.h>
#include <src/database/firebase/sql_select.h>

void test_firebase_sql_parse_returning(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_sql_parse_returning(void) {
    FirebaseSqlStatement* stmt = firebase_sql_parse(
        "INSERT INTO t (a) VALUES (1) RETURNING a, b");
    TEST_ASSERT_NULL(stmt->error_message);
    TEST_ASSERT_EQUAL(2, stmt->insert.returning_count);
    TEST_ASSERT_EQUAL_STRING("a", stmt->insert.returning[0]);
    TEST_ASSERT_EQUAL_STRING("b", stmt->insert.returning[1]);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("INSERT INTO t (a) VALUES (1)");
    TEST_ASSERT_EQUAL(0, stmt->insert.returning_count);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("INSERT INTO t (a) VALUES (1) RETURNING");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    const char* sql = "RETURNING id";
    stmt = firebase_sql_statement_alloc();
    TEST_ASSERT_TRUE(firebase_sql_parse_returning(&sql, stmt));
    TEST_ASSERT_EQUAL(1, stmt->insert.returning_count);
    firebase_sql_statement_free(stmt);

    TEST_ASSERT_FALSE(firebase_sql_parse_returning(NULL, NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_sql_parse_returning);
    return UNITY_END();
}
