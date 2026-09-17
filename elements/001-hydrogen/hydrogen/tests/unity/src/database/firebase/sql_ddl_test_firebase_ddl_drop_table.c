/*
 * Unity tests for firebase_ddl_drop_table().
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
void test_firebase_ddl_drop_table_null(void);
void test_firebase_ddl_drop_table_empty(void);
void test_firebase_ddl_drop_table_missing(void);
void test_firebase_ddl_drop_table_if_exists(void);
void test_firebase_ddl_drop_table_with_row(void);
void test_firebase_catalog_has_documents(void);

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

void test_firebase_ddl_drop_table_null(void) {
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_ddl_drop_table(NULL, NULL, &result));
    TEST_ASSERT_FALSE(result->success);
    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_firebase_ddl_drop_table_empty(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, "{}");
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200, "{}");
    firebase_http_test_set_method_response("DELETE", "_schema/testfb_queries", 200, "{}");

    QueryRequest request = {0};
    request.sql_template = (char*)"DROP TABLE testfb_queries;";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

void test_firebase_ddl_drop_table_missing(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 404, "{}");
    QueryRequest request = {0};
    request.sql_template = (char*)"DROP TABLE testfb_queries;";
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "does not exist"));
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

void test_firebase_ddl_drop_table_if_exists(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    firebase_http_test_set_method_response("GET", "_schema/testfb_missing", 404, "{}");
    QueryRequest request = {0};
    request.sql_template = (char*)"DROP TABLE IF EXISTS missing;";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

void test_firebase_ddl_drop_table_with_row(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, "{}");
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200,
        "{\"documents\":[{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_queries/1\"}]}");
    firebase_http_test_set_method_response("DELETE", "testfb_queries/1", 200, "{}");
    firebase_http_test_set_method_response("DELETE", "_schema/testfb_queries", 200, "{}");
    QueryRequest request = {0};
    request.sql_template = (char*)"DROP TABLE testfb_queries;";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

void test_firebase_catalog_has_documents(void) {
    TEST_ASSERT_FALSE(firebase_catalog_has_documents(NULL));
    TEST_ASSERT_FALSE(firebase_catalog_has_documents("{}"));
    TEST_ASSERT_TRUE(firebase_catalog_has_documents(
        "{\"documents\":[{\"name\":\"projects/x/databases/(default)/documents/t/1\"}]}"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_ddl_drop_table_null);
    RUN_TEST(test_firebase_ddl_drop_table_empty);
    RUN_TEST(test_firebase_ddl_drop_table_missing);
    RUN_TEST(test_firebase_ddl_drop_table_if_exists);
    RUN_TEST(test_firebase_ddl_drop_table_with_row);
    RUN_TEST(test_firebase_catalog_has_documents);
    return UNITY_END();
}
