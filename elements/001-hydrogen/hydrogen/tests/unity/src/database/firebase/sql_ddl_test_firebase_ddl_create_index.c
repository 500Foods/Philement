/*
 * Unity tests for firebase_ddl_create_index().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebase/types.h>
#include <src/database/firebase/http.h>
#include <src/database/firebase/connection.h>
#include <src/database/firebase/query.h>
#include <src/database/firebase/sql_ddl.h>

static ConnectionConfig g_config;

void destroy_handle(DatabaseHandle* handle);
DatabaseHandle* connect_testfb(void);
char* min_catalog(void);
void test_firebase_ddl_create_index_null(void);
void test_firebase_ddl_create_index_records(void);
void test_firebase_ddl_create_index_missing_table(void);
void test_firebase_ddl_create_unique_index(void);

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

char* min_catalog(void) {
    FirebaseSqlTable table = {0};
    FirebaseSqlColumn col = {0};
    col.name = (char*)"last_accessed";
    col.type = (char*)"timestamp";
    table.columns = &col;
    table.column_count = 1;
    return firebase_catalog_document_json(&table, "testfb_queries", "[]");
}

void test_firebase_ddl_create_index_null(void) {
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_ddl_create_index(NULL, NULL, &result));
    TEST_ASSERT_FALSE(result->success);
    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_firebase_ddl_create_index_records(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    char* catalog = min_catalog();
    TEST_ASSERT_NOT_NULL(catalog);
    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog);
    firebase_http_test_set_method_response("PATCH", "_schema/testfb_queries", 200, "{}");
    QueryRequest request = {0};
    request.sql_template = (char*)
        "CREATE INDEX queries_idx_last_accessed ON testfb_queries(last_accessed);";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL_STRING("PATCH", firebase_http_test_last_method());
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "queries_idx_last_accessed"));
    free(catalog);
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

void test_firebase_ddl_create_unique_index(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    char* catalog = min_catalog();
    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog);
    firebase_http_test_set_method_response("PATCH", "_schema/testfb_queries", 200, "{}");
    QueryRequest request = {0};
    request.sql_template = (char*)"CREATE UNIQUE INDEX uq ON testfb_queries (last_accessed);";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "unique"));
    free(catalog);
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

void test_firebase_ddl_create_index_missing_table(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 404, "{}");
    QueryRequest request = {0};
    request.sql_template = (char*)"CREATE INDEX x ON testfb_queries(id);";
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "does not exist"));
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_ddl_create_index_null);
    RUN_TEST(test_firebase_ddl_create_index_records);
    RUN_TEST(test_firebase_ddl_create_unique_index);
    RUN_TEST(test_firebase_ddl_create_index_missing_table);
    return UNITY_END();
}
