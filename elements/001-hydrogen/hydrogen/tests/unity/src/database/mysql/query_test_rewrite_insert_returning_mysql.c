/*
 * Unity Test File: mysql_rewrite_insert_returning
 *
 * Oracle MySQL rejects INSERT ... RETURNING. The rewriter wraps the selected
 * new_<column> token in LAST_INSERT_ID() and drops the RETURNING clause.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <unity/mocks/mock_libmysqlclient.h>

#include <src/database/database.h>
#include <src/database/mysql/connection.h>
#include <src/database/mysql/query.h>

void test_mysql_rewrite_insert_returning_null_sql(void);
void test_mysql_rewrite_insert_returning_leaves_plain_select(void);
void test_mysql_rewrite_insert_returning_wraps_selected_key(void);
void test_mysql_rewrite_insert_returning_drops_semicolon(void);
void test_mysql_rewrite_insert_returning_alias_only_stays(void);
void test_mysql_rewrite_insert_returning_queue_id(void);

void setUp(void) {
    mock_libmysqlclient_reset_all();
    load_libmysql_functions(NULL);
}

void tearDown(void) {
    mock_libmysqlclient_reset_all();
}

void test_mysql_rewrite_insert_returning_null_sql(void) {
    char column[64];
    TEST_ASSERT_NULL(mysql_rewrite_insert_returning(NULL, column, sizeof(column)));
    TEST_ASSERT_NULL(mysql_rewrite_insert_returning("SELECT 1 RETURNING account_id", NULL, 64));
    TEST_ASSERT_NULL(mysql_rewrite_insert_returning("SELECT 1 RETURNING account_id", column, 1));
}

void test_mysql_rewrite_insert_returning_leaves_plain_select(void) {
    char column[64] = "stale";
    const char* sql = "SELECT new_account_id FROM next_account_id";
    TEST_ASSERT_NULL(mysql_rewrite_insert_returning(sql, column, sizeof(column)));
    TEST_ASSERT_EQUAL_STRING("", column);
}

void test_mysql_rewrite_insert_returning_wraps_selected_key(void) {
    char column[64];
    const char* sql =
        "INSERT INTO demo.accounts (account_id, username)\n"
        "WITH next_account_id AS (\n"
        "SELECT COALESCE(MAX(account_id), 0) + 1 AS new_account_id FROM demo.accounts)\n"
        "SELECT new_account_id, ? FROM next_account_id\n"
        "RETURNING account_id";

    char* rewritten = mysql_rewrite_insert_returning(sql, column, sizeof(column));
    TEST_ASSERT_NOT_NULL(rewritten);
    TEST_ASSERT_EQUAL_STRING("account_id", column);
    TEST_ASSERT_NOT_NULL(strstr(rewritten, "AS new_account_id"));
    TEST_ASSERT_NOT_NULL(strstr(rewritten, "SELECT LAST_INSERT_ID(new_account_id), ?"));
    TEST_ASSERT_NULL(strstr(rewritten, "RETURNING"));
    TEST_ASSERT_NULL(strstr(rewritten, ";"));

    char again_column[64] = "stale";
    TEST_ASSERT_NULL(mysql_rewrite_insert_returning(rewritten, again_column, sizeof(again_column)));
    TEST_ASSERT_EQUAL_STRING("", again_column);
    free(rewritten);
}

void test_mysql_rewrite_insert_returning_drops_semicolon(void) {
    char column[64];
    const char* sql =
        "SELECT new_queue_id FROM next_queue\n"
        "RETURNING queue_id ;";
    /* The AS alias is absent, so the only new_queue_id is the selected token. */
    char* rewritten = mysql_rewrite_insert_returning(sql, column, sizeof(column));
    TEST_ASSERT_NOT_NULL(rewritten);
    TEST_ASSERT_EQUAL_STRING("queue_id", column);
    TEST_ASSERT_NOT_NULL(strstr(rewritten, "SELECT LAST_INSERT_ID(new_queue_id) FROM next_queue"));
    TEST_ASSERT_NULL(strchr(rewritten, ';'));
    free(rewritten);
}

void test_mysql_rewrite_insert_returning_alias_only_stays(void) {
    char column[64] = "stale";
    const char* sql = "SELECT COALESCE(MAX(account_id), 0) + 1 AS new_account_id RETURNING account_id";
    TEST_ASSERT_NULL(mysql_rewrite_insert_returning(sql, column, sizeof(column)));
    TEST_ASSERT_EQUAL_STRING("", column);
}

void test_mysql_rewrite_insert_returning_queue_id(void) {
    char column[64];
    const char* sql =
        "INSERT INTO demo.mail_queue (queue_id)\n"
        "WITH next_queue_id AS (SELECT COALESCE(MAX(queue_id), 0) + 1 AS new_queue_id FROM demo.mail_queue)\n"
        "SELECT new_queue_id FROM next_queue_id RETURNING queue_id";
    char* rewritten = mysql_rewrite_insert_returning(sql, column, sizeof(column));
    TEST_ASSERT_NOT_NULL(rewritten);
    TEST_ASSERT_EQUAL_STRING("queue_id", column);
    TEST_ASSERT_NOT_NULL(strstr(rewritten, "AS new_queue_id"));
    TEST_ASSERT_NOT_NULL(strstr(rewritten, "SELECT LAST_INSERT_ID(new_queue_id) FROM next_queue_id"));
    TEST_ASSERT_NULL(strstr(rewritten, "RETURNING"));
    free(rewritten);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mysql_rewrite_insert_returning_null_sql);
    RUN_TEST(test_mysql_rewrite_insert_returning_leaves_plain_select);
    RUN_TEST(test_mysql_rewrite_insert_returning_wraps_selected_key);
    RUN_TEST(test_mysql_rewrite_insert_returning_drops_semicolon);
    RUN_TEST(test_mysql_rewrite_insert_returning_alias_only_stays);
    RUN_TEST(test_mysql_rewrite_insert_returning_queue_id);

    return UNITY_END();
}
