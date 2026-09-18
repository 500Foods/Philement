/*
 * Unity tests for firebase_dml_insert_select().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebase/types.h>
#include <src/database/firebase/http.h>
#include <src/database/firebase/connection.h>
#include <src/database/firebase/query.h>
#include <src/database/firebase/sql_parse.h>
#include <src/database/firebase/sql_ddl.h>
#include <src/database/firebase/sql_dml.h>
#include <src/database/firebase/sql_select.h>
#include <src/database/firebase/fns_tz.h>

static ConnectionConfig g_config;

void destroy_handle(DatabaseHandle* handle);
void release_query_result(QueryResult* result);
DatabaseHandle* connect_testfb(void);
char* queries_catalog_json(void);
void test_firebase_dml_insert_select(void);

void setUp(void) {
    firebase_http_test_clear_responses();
    firebase_now_test_clear();
}

void tearDown(void) {
    firebase_http_test_clear_responses();
    firebase_now_test_clear();
}

void destroy_handle(DatabaseHandle* handle) {
    if (!handle) {
        return;
    }
    firebase_disconnect(handle);
    free(handle->designator);
    pthread_mutex_destroy(&handle->connection_lock);
    free(handle);
}

void release_query_result(QueryResult* result) {
    database_engine_cleanup_result(result);
}

DatabaseHandle* connect_testfb(void) {
    firebase_http_test_set_response("documents/", 200, "{}");
    memset(&g_config, 0, sizeof(g_config));
    g_config.username = (char*)"hydrodemo";
    g_config.host = (char*)"127.0.0.1";
    g_config.port = 8080;
    g_config.database = (char*)"(default)";
    g_config.schema = (char*)"testfb";
    DatabaseHandle* conn = NULL;
    if (!firebase_connect(&g_config, &conn, "t")) {
        return NULL;
    }
    return conn;
}

char* queries_catalog_json(void) {
    FirebaseSqlStatement* stmt = firebase_sql_parse(
        "CREATE TABLE queries ("
        " query_id integer NOT NULL,"
        " query_ref integer NOT NULL,"
        " query_type_a28 integer NOT NULL,"
        " code text NOT NULL,"
        " PRIMARY KEY(query_id),"
        " UNIQUE(query_ref, query_type_a28)"
        ")");
    char* json = firebase_catalog_document_json(&stmt->table, "testfb_queries", "[]");
    firebase_sql_statement_free(stmt);
    return json;
}

void test_firebase_dml_insert_select(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    char* catalog = queries_catalog_json();

    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200, "{}");
    firebase_http_test_set_method_response("GET", "testfb_queries/1", 404, "{}");
    firebase_http_test_set_method_response("PATCH", "testfb_queries/1", 200, "{}");

    QueryRequest request = {0};
    request.sql_template = (char*)
        "INSERT INTO queries (query_id, query_ref, query_type_a28, code)\n"
        "WITH next_query_id AS (\n"
        "  SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id FROM queries\n"
        ")\n"
        "SELECT new_query_id AS query_id, 1000 AS query_ref, 1 AS query_type_a28, 'hello' AS code\n"
        "FROM next_query_id\n"
        "RETURNING query_id";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(1, result->affected_rows);
    TEST_ASSERT_EQUAL(1, result->row_count);
    TEST_ASSERT_NOT_NULL(strstr(result->data_json, "\"query_id\":1"));
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_url(), "testfb_queries/1"));
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200,
        "{\"documents\":[{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_queries/1\","
        "\"fields\":{\"query_id\":{\"integerValue\":\"1\"},"
        "\"query_ref\":{\"integerValue\":\"1000\"},"
        "\"query_type_a28\":{\"integerValue\":\"1\"}}}]}");
    firebase_http_test_set_method_response("GET", "testfb_queries/2", 404, "{}");
    firebase_http_test_set_method_response("PATCH", "testfb_queries/2", 200, "{}");
    request.sql_template = (char*)
        "INSERT INTO queries (query_id, query_ref, query_type_a28, code)\n"
        "WITH next_query_id AS (\n"
        "  SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id FROM queries\n"
        ")\n"
        "SELECT new_query_id AS query_id, 1001 AS query_ref, 1 AS query_type_a28, 'world' AS code\n"
        "FROM next_query_id";
    result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_url(), "testfb_queries/2"));
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "documents/testfb_accounts", 200,
        "{\"documents\":[{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_accounts/9\","
        "\"fields\":{\"account_id\":{\"integerValue\":\"9\"},\"name\":{\"stringValue\":\"Ada\"}}}]}");
    firebase_http_test_set_method_response("PATCH", "testfb_accounts_new/9", 200, "{}");
    request.sql_template = (char*)"INSERT INTO accounts_new SELECT * FROM accounts";
    result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(1, result->affected_rows);
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_url(), "testfb_accounts_new/9"));
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "Ada"));
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200, "{}");
    request.sql_template = (char*)
        "INSERT INTO queries (query_id, nope)\n"
        "WITH next_query_id AS (SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id FROM queries)\n"
        "SELECT new_query_id AS query_id, 1 AS nope FROM next_query_id";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "unknown column"));
    release_query_result(result);

    TEST_ASSERT_FALSE(firebase_dml_insert_select(NULL, NULL, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "invalid connection"));
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 404, "{}");
    request.sql_template = (char*)
        "INSERT INTO queries (query_id, query_ref, query_type_a28, code)\n"
        "WITH next_query_id AS (SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id FROM queries)\n"
        "SELECT new_query_id AS query_id, 1 AS query_ref, 1 AS query_type_a28, 'x' AS code FROM next_query_id";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "does not exist"));
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200, "{}");
    firebase_http_test_set_method_response("GET", "testfb_queries/1", 200, "{}");
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "duplicate primary key"));
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200,
        "{\"documents\":[{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_queries/9\","
        "\"fields\":{\"query_id\":{\"integerValue\":\"9\"},"
        "\"query_ref\":{\"integerValue\":\"7\"},"
        "\"query_type_a28\":{\"integerValue\":\"1\"}}}]}");
    firebase_http_test_set_method_response("GET", "testfb_queries/10", 404, "{}");
    request.sql_template = (char*)
        "INSERT INTO queries (query_id, query_ref, query_type_a28, code)\n"
        "WITH next_query_id AS (SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id FROM queries)\n"
        "SELECT new_query_id AS query_id, 7 AS query_ref, 1 AS query_type_a28, 'x' AS code FROM next_query_id";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "UNIQUE"));
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200, "{}");
    firebase_http_test_set_method_response("GET", "testfb_queries/1", 404, "{}");
    firebase_http_test_set_method_response("PATCH", "testfb_queries/1", 500, "{}");
    request.sql_template = (char*)
        "INSERT INTO queries (query_id, query_ref, query_type_a28, code)\n"
        "WITH next_query_id AS (SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id FROM queries)\n"
        "SELECT new_query_id AS query_id, 8 AS query_ref, 1 AS query_type_a28, 'x' AS code FROM next_query_id";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "PATCH failed"));
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "documents/testfb_accounts", 200, "{}");
    request.sql_template = (char*)"INSERT INTO accounts_new SELECT * FROM accounts RETURNING name";
    result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_EQUAL(0, result->affected_rows);
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "documents/testfb_accounts", 200,
        "{\"documents\":[{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_accounts/9\","
        "\"fields\":{\"account_id\":{\"integerValue\":\"9\"},\"name\":{\"stringValue\":\"Ada\"}}}]}");
    firebase_http_test_set_method_response("PATCH", "testfb_accounts_new/9", 500, "{}");
    request.sql_template = (char*)"INSERT INTO accounts_new SELECT * FROM accounts";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "PATCH failed"));
    release_query_result(result);

    FirebaseSqlStatement* extra = firebase_sql_parse(
        "CREATE TABLE queries ("
        " query_id integer NOT NULL,"
        " query_ref integer NOT NULL,"
        " query_type_a28 integer NOT NULL,"
        " code text NOT NULL,"
        " note text DEFAULT 'hi',"
        " PRIMARY KEY(query_id)"
        ")");
    char* extra_cat = firebase_catalog_document_json(&extra->table, "testfb_queries", "[]");
    firebase_sql_statement_free(extra);
    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, extra_cat);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200, "{}");
    firebase_http_test_set_method_response("GET", "testfb_queries/1", 404, "{}");
    firebase_http_test_set_method_response("PATCH", "testfb_queries/1", 200, "{}");
    request.sql_template = (char*)
        "INSERT INTO queries (query_id, query_ref, query_type_a28, code)\n"
        "WITH next_query_id AS (SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id FROM queries)\n"
        "SELECT new_query_id AS query_id, 9 AS query_ref, 1 AS query_type_a28, 'x' AS code FROM next_query_id";
    result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "hi"));
    release_query_result(result);
    free(extra_cat);

    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200, "{}");
    request.sql_template = (char*)
        "INSERT INTO queries (query_id, query_ref, query_type_a28, code)\n"
        "WITH next_query_id AS (SELECT FB_TIME_ADD(1, 2, 'm') AS new_query_id FROM queries)\n"
        "SELECT new_query_id AS query_id, 1 AS query_ref, 1 AS query_type_a28, 'x' AS code FROM next_query_id";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(result->error_message);
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200, "{}");
    request.sql_template = (char*)
        "INSERT INTO queries (query_id, query_ref, query_type_a28, code)\n"
        "WITH next_query_id AS (SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id FROM queries)\n"
        "SELECT FB_TIME_ADD(1, 2, 'm') AS query_id, 1 AS query_ref, 1 AS query_type_a28, 'x' AS code FROM next_query_id";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(result->error_message);
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200, "{}");
    request.sql_template = (char*)
        "INSERT INTO queries (query_id, query_ref, query_type_a28, code)\n"
        "WITH next_query_id AS (SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id FROM queries)\n"
        "SELECT NULL AS query_id, 1 AS query_ref, 1 AS query_type_a28, 'x' AS code FROM next_query_id";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "NOT NULL"));
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "documents/testfb_accounts", 200,
        "{\"documents\":[{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_accounts/skip\"}]}");
    request.sql_template = (char*)"INSERT INTO accounts_new SELECT * FROM accounts";
    result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_EQUAL(0, result->affected_rows);
    release_query_result(result);

    FirebaseSqlStatement* nn = firebase_sql_parse(
        "CREATE TABLE queries ("
        " query_id integer NOT NULL,"
        " query_ref integer NOT NULL,"
        " query_type_a28 integer NOT NULL,"
        " code text NOT NULL,"
        " extra integer NOT NULL,"
        " PRIMARY KEY(query_id)"
        ")");
    char* nn_cat = firebase_catalog_document_json(&nn->table, "testfb_queries", "[]");
    firebase_sql_statement_free(nn);
    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, nn_cat);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200, "{}");
    request.sql_template = (char*)
        "INSERT INTO queries (query_id, query_ref, query_type_a28, code)\n"
        "WITH next_query_id AS (SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id FROM queries)\n"
        "SELECT new_query_id AS query_id, 11 AS query_ref, 1 AS query_type_a28, 'x' AS code FROM next_query_id";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "NOT NULL"));
    release_query_result(result);
    free(nn_cat);

    free(catalog);
    destroy_handle(conn);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_dml_insert_select);
    return UNITY_END();
}
