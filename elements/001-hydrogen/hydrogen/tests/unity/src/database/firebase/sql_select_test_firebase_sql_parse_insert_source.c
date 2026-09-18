/*
 * Unity tests for firebase_sql_parse_insert_source().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/sql_parse.h>
#include <src/database/firebase/sql_expr.h>
#include <src/database/firebase/sql_select.h>

void test_firebase_sql_parse_insert_source(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_sql_parse_insert_source(void) {
    FirebaseSqlStatement* stmt = firebase_sql_parse(
        "-- query_id\n"
        "INSERT INTO queries (\n"
        "  query_id, query_ref, query_type_a28, code\n"
        ")\n"
        "WITH next_query_id AS (\n"
        "  SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id\n"
        "  FROM queries\n"
        ")\n"
        "SELECT\n"
        "  new_query_id AS query_id,\n"
        "  1000 AS query_ref,\n"
        "  1 AS query_type_a28,\n"
        "  'hello' AS code\n"
        "FROM next_query_id\n"
        "RETURNING query_id");
    TEST_ASSERT_NOT_NULL(stmt);
    TEST_ASSERT_NULL(stmt->error_message);
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_INSERT, stmt->kind);
    TEST_ASSERT_EQUAL_STRING("queries", stmt->insert.table);
    TEST_ASSERT_EQUAL(4, stmt->insert.column_count);
    TEST_ASSERT_EQUAL_STRING("next_query_id", stmt->insert.cte.name);
    TEST_ASSERT_EQUAL(1, stmt->insert.cte.query.item_count);
    TEST_ASSERT_EQUAL(FIREBASE_EXPR_ADD, stmt->insert.cte.query.items[0].expr->kind);
    TEST_ASSERT_EQUAL_STRING("queries", stmt->insert.cte.query.from_name);
    TEST_ASSERT_EQUAL(4, stmt->insert.select.item_count);
    TEST_ASSERT_EQUAL_STRING("next_query_id", stmt->insert.select.from_name);
    TEST_ASSERT_EQUAL(1, stmt->insert.returning_count);
    TEST_ASSERT_EQUAL_STRING("query_id", stmt->insert.returning[0]);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("INSERT INTO accounts_new SELECT * FROM accounts");
    TEST_ASSERT_NULL(stmt->error_message);
    TEST_ASSERT_TRUE(stmt->insert.select.star);
    TEST_ASSERT_EQUAL_STRING("accounts", stmt->insert.select.from_name);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse(
        "INSERT INTO queries (query_id) WITH x AS (SELECT 1 AS n FROM queries) "
        "SELECT n FROM y");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("INSERT INTO t (a) WITH x AS SELECT 1 FROM t SELECT 1 FROM x");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("INSERT INTO t SELECT a FROM t");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    const char* errors[] = {
        "INSERT INTO t (a) WITH",
        "INSERT INTO t (a) WITH x",
        "INSERT INTO t (a) WITH x AS y",
        "INSERT INTO t (a) WITH x AS (",
        "INSERT INTO t (a) WITH x AS (SELECT",
        "INSERT INTO t (a) WITH x AS (SELECT 1",
        "INSERT INTO t (a) WITH x AS (SELECT 1 FROM",
        "INSERT INTO t (a) WITH x AS (SELECT 1 FROM t",
        "INSERT INTO t (a) WITH x AS (SELECT 1 FROM t)",
        "INSERT INTO t (a) WITH x AS (SELECT 1 FROM t) SELECT 1",
        "INSERT INTO t (a) WITH x AS (SELECT 1 FROM t) SELECT 1 FROM",
        "INSERT INTO t WITH x AS (SELECT 1 FROM t) SELECT 1 FROM x",
        "INSERT INTO t (a, b) WITH x AS (SELECT 1 FROM t) SELECT 1 FROM x",
        "INSERT INTO t (a) STUFF",
        "INSERT INTO t SELECT *",
        "INSERT INTO t SELECT * FROM",
        "INSERT INTO t (a) WITH x AS (SELECT 1 AS) SELECT 1 FROM x",
        NULL
    };
    for (size_t i = 0; errors[i]; i++) {
        stmt = firebase_sql_parse(errors[i]);
        TEST_ASSERT_NOT_NULL(stmt->error_message);
        firebase_sql_statement_free(stmt);
    }

    TEST_ASSERT_FALSE(firebase_sql_parse_insert_source(NULL, NULL));
    TEST_ASSERT_FALSE(firebase_sql_parse_select_list(NULL, NULL, NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_sql_parse_insert_source);
    return UNITY_END();
}
