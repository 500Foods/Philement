/*
 * Unity tests for INSERT / UPDATE / DELETE parsing.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/sql_parse.h>
#include <src/database/firebase/sql_expr.h>

void test_firebase_sql_parse_insert_values(void);
void test_firebase_sql_parse_update_and_delete(void);
void test_firebase_sql_parse_dml_errors(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_sql_parse_insert_values(void) {
    FirebaseSqlStatement* stmt = firebase_sql_parse(
        "INSERT INTO lookups (lookup_id, key_idx, value_txt, collection, created_at)\n"
        "VALUES (\n"
        "  030, -- lookup_id\n"
        "  6,\n"
        "  'Firebase',\n"
        "  FB_JSON_INGEST('{\"icon\":\"fb\"}'),\n"
        "  FB_NOW()\n"
        ")");
    TEST_ASSERT_NOT_NULL(stmt);
    TEST_ASSERT_NULL(stmt->error_message);
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_INSERT, stmt->kind);
    TEST_ASSERT_EQUAL_STRING("lookups", stmt->insert.table);
    TEST_ASSERT_EQUAL(5, stmt->insert.column_count);
    TEST_ASSERT_EQUAL(1, stmt->insert.row_count);
    TEST_ASSERT_EQUAL(FIREBASE_EXPR_INTEGER, stmt->insert.rows[0].values[0]->kind);
    TEST_ASSERT_EQUAL_STRING("030", stmt->insert.rows[0].values[0]->text);
    TEST_ASSERT_EQUAL(FIREBASE_EXPR_CALL, stmt->insert.rows[0].values[3]->kind);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("INSERT INTO t (a, b) VALUES (1, 2), (3, 4)");
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_INSERT, stmt->kind);
    TEST_ASSERT_EQUAL(2, stmt->insert.row_count);
    firebase_sql_statement_free(stmt);
}

void test_firebase_sql_parse_update_and_delete(void) {
    FirebaseSqlStatement* stmt = firebase_sql_parse(
        "UPDATE queries SET query_type_a28 = 2, updated_at = FB_NOW()\n"
        "WHERE query_ref = 1001 and query_type_a28 = 1");
    TEST_ASSERT_NULL(stmt->error_message);
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_UPDATE, stmt->kind);
    TEST_ASSERT_EQUAL_STRING("queries", stmt->update.table);
    TEST_ASSERT_EQUAL(2, stmt->update.set_count);
    TEST_ASSERT_EQUAL(2, stmt->update.where.count);
    TEST_ASSERT_EQUAL(FIREBASE_PRED_EQ, stmt->update.where.predicates[0].op);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse(
        "DELETE FROM lookups WHERE lookup_id = 30 AND key_idx IN (0, 1, 6)");
    TEST_ASSERT_NULL(stmt->error_message);
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_DELETE, stmt->kind);
    TEST_ASSERT_EQUAL(2, stmt->del.where.count);
    TEST_ASSERT_EQUAL(FIREBASE_PRED_IN, stmt->del.where.predicates[1].op);
    TEST_ASSERT_EQUAL(3, stmt->del.where.predicates[1].in_count);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("DELETE FROM lookups WHERE valid_after IS NULL");
    TEST_ASSERT_EQUAL(FIREBASE_PRED_IS_NULL, stmt->del.where.predicates[0].op);
    firebase_sql_statement_free(stmt);
}

void test_firebase_sql_parse_dml_errors(void) {
    FirebaseSqlStatement* stmt = firebase_sql_parse("INSERT INTO t (a) VALUES (1, 2)");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("UPDATE t SET a = 1");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("DELETE FROM t");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("INSERT t (a) VALUES (1)");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("INSERT INTO (a) VALUES (1)");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("INSERT INTO t () VALUES (1)");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("INSERT INTO t (a) VALUES (");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("INSERT INTO t (a) VALUES (1");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("UPDATE SET a = 1 WHERE a = 1");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("UPDATE t a = 1 WHERE a = 1");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("UPDATE t SET = 1 WHERE a = 1");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("UPDATE t SET a 1 WHERE a = 1");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("UPDATE t SET a = WHERE a = 1");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("DELETE t WHERE a = 1");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("DELETE FROM WHERE a = 1");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("DELETE FROM t WHERE");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("DELETE FROM t WHERE a IS");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("DELETE FROM t WHERE a IN");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("DELETE FROM t WHERE a IN (");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("DELETE FROM t WHERE a IN (1");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("DELETE FROM t WHERE a LIKE 1");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("DELETE FROM t WHERE a = ");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    const char* sql = "(42";
    char* err = NULL;
    FirebaseExpr* expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_NULL(expr);
    free(err);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_sql_parse_insert_values);
    RUN_TEST(test_firebase_sql_parse_update_and_delete);
    RUN_TEST(test_firebase_sql_parse_dml_errors);
    return UNITY_END();
}
