/*
 * Unity tests for firebase_ddl_create_table().
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

static ConnectionConfig g_config;

void destroy_handle(DatabaseHandle* handle);
DatabaseHandle* connect_testfb(void);
void test_firebase_ddl_create_table_null(void);
void test_firebase_ddl_create_table_queries(void);
void test_firebase_ddl_create_table_already_exists(void);
void test_firebase_catalog_document_json(void);

void setUp(void) {
    firebase_http_test_clear_responses();
}

void tearDown(void) {
    firebase_http_test_clear_responses();
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

void test_firebase_ddl_create_table_null(void) {
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_ddl_create_table(NULL, NULL, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_firebase_ddl_create_table_queries(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);

    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 404, "{}");
    firebase_http_test_set_method_response("PATCH", "_schema/testfb_queries", 200, "{}");

    QueryRequest request = {0};
    request.sql_template = (char*)
        "CREATE TABLE testfb_queries (\n"
        "    query_id integer NOT NULL,\n"
        "    query_ref integer NOT NULL,\n"
        "    code text NOT NULL,\n"
        "    collection json,\n"
        "    PRIMARY KEY(query_id),\n"
        "    UNIQUE(query_ref)\n"
        ");";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_NOT_NULL(firebase_http_test_last_url());
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_url(), "_schema/testfb_queries"));
    TEST_ASSERT_EQUAL_STRING("PATCH", firebase_http_test_last_method());
    TEST_ASSERT_NOT_NULL(firebase_http_test_last_body());
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "query_id"));
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

void test_firebase_ddl_create_table_already_exists(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, "{}");
    QueryRequest request = {0};
    request.sql_template = (char*)"CREATE TABLE testfb_queries (query_id integer NOT NULL);";
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "already exists"));
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

void test_firebase_catalog_document_json(void) {
    TEST_ASSERT_NULL(firebase_catalog_document_json(NULL, "x", "[]"));
    FirebaseSqlTable table = {0};
    FirebaseSqlColumn col = {0};
    col.name = (char*)"id";
    col.type = (char*)"integer";
    col.not_null = true;
    col.default_sql = (char*)"0";
    table.columns = &col;
    table.column_count = 1;
    char pkcol[] = "id";
    char* pkptr = pkcol;
    table.primary_key.columns = &pkptr;
    table.primary_key.count = 1;
    char* json = firebase_catalog_document_json(&table, "testfb_queries", NULL);
    TEST_ASSERT_NOT_NULL(json);
    TEST_ASSERT_NOT_NULL(strstr(json, "testfb_queries"));
    TEST_ASSERT_NOT_NULL(strstr(json, "integer"));
    TEST_ASSERT_NOT_NULL(strstr(json, "default_sql"));
    free(json);
    TEST_ASSERT_EQUAL_STRING("1", firebase_ddl_doc_id_from_name("projects/x/documents/t/1"));
    TEST_ASSERT_NULL(firebase_ddl_doc_id_from_name(NULL));
    TEST_ASSERT_NULL(firebase_ddl_doc_id_from_name("noslash"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_ddl_create_table_null);
    RUN_TEST(test_firebase_ddl_create_table_queries);
    RUN_TEST(test_firebase_ddl_create_table_already_exists);
    RUN_TEST(test_firebase_catalog_document_json);
    return UNITY_END();
}
