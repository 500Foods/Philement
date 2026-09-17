/*
 * Unity tests for firebase_dml_update().
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
#include <src/database/firebase/fns_tz.h>

static ConnectionConfig g_config;

void destroy_handle(DatabaseHandle* handle);
void release_query_result(QueryResult* result);
DatabaseHandle* connect_testfb(void);
char* queries_catalog_json(void);
void test_firebase_dml_update_where_and(void);
void test_firebase_dml_update_null_connection(void);

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
    if (!result) {
        return;
    }
    free(result->error_message);
    free(result->data_json);
    free(result);
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
        " code text,"
        " updated_at timestamp,"
        " PRIMARY KEY(query_id)"
        ")");
    char* json = firebase_catalog_document_json(&stmt->table, "testfb_queries", "[]");
    firebase_sql_statement_free(stmt);
    return json;
}

void test_firebase_dml_update_null_connection(void) {
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_dml_update(NULL, NULL, &result));
    TEST_ASSERT_FALSE(result->success);
    release_query_result(result);
}

void test_firebase_dml_update_where_and(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    char* catalog = queries_catalog_json();
    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200,
        "{\"documents\":["
        "{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_queries/1\","
        "\"fields\":{\"query_id\":{\"integerValue\":\"1\"},"
        "\"query_ref\":{\"integerValue\":\"1001\"},"
        "\"query_type_a28\":{\"integerValue\":\"1\"},"
        "\"code\":{\"stringValue\":\"keep\"}}},"
        "{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_queries/2\","
        "\"fields\":{\"query_id\":{\"integerValue\":\"2\"},"
        "\"query_ref\":{\"integerValue\":\"1002\"},"
        "\"query_type_a28\":{\"integerValue\":\"1\"},"
        "\"code\":{\"stringValue\":\"other\"}}}"
        "]}");
    firebase_http_test_set_method_response("PATCH", "testfb_queries/1", 200, "{}");
    firebase_now_test_set_unix(1700000000);

    QueryRequest request = {0};
    request.sql_template = (char*)
        "UPDATE queries SET query_type_a28 = 2, updated_at = FB_NOW() "
        "WHERE query_ref = 1001 AND query_type_a28 = 1";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(1, result->affected_rows);
    TEST_ASSERT_EQUAL_STRING("PATCH", firebase_http_test_last_method());
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_url(), "testfb_queries/1"));
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "\"integerValue\":\"2\""));
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "keep"));
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "2023-11-14T22:13:20Z"));
    release_query_result(result);
    free(catalog);

    char* catalog2 = queries_catalog_json();
    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog2);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 500, "{}");
    request.sql_template = (char*)"UPDATE queries SET code = 'x' WHERE query_id = 1";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "failed to list"));
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog2);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200,
        "{\"documents\":[{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_queries/1\","
        "\"fields\":{\"query_id\":{\"integerValue\":\"1\"}}}]}");
    firebase_http_test_set_method_response("PATCH", "testfb_queries/1", 500, "{}");
    request.sql_template = (char*)"UPDATE queries SET code = 'x' WHERE query_id = 1";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "PATCH failed"));
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog2);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200,
        "{\"documents\":[{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_queries/1\","
        "\"fields\":{\"query_id\":{\"integerValue\":\"1\"}}}]}");
    request.sql_template = (char*)
        "UPDATE queries SET code = FB_TIME_ADD(1, 2, 'm') WHERE query_id = 1";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_FALSE(result->success);
    release_query_result(result);
    free(catalog2);
    destroy_handle(conn);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_dml_update_null_connection);
    RUN_TEST(test_firebase_dml_update_where_and);
    return UNITY_END();
}
