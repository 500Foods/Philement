/*
 * Unity tests for firebase_ddl_refuse_drop() / DROP_CHECK.
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
void test_firebase_ddl_refuse_drop_empty(void);
void test_firebase_ddl_refuse_drop_with_row(void);
void test_firebase_ddl_refuse_drop_null(void);

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

void test_firebase_ddl_refuse_drop_null(void) {
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_ddl_refuse_drop(NULL, NULL, &result));
    TEST_ASSERT_FALSE(result->success);
    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_firebase_ddl_refuse_drop_empty(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    firebase_http_test_set_method_response("GET", "documents/testfb_lookups", 200, "{}");
    QueryRequest request = {0};
    request.sql_template = (char*)
        "SELECT FB_REFUSE_DROP('testfb_lookups') WHERE EXISTS (SELECT 1 FROM testfb_lookups)";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

void test_firebase_ddl_refuse_drop_with_row(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    firebase_http_test_set_method_response("GET", "documents/testfb_lookups", 200,
        "{\"documents\":[{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_lookups/1_0\"}]}");
    QueryRequest request = {0};
    request.sql_template = (char*)
        "SELECT FB_REFUSE_DROP('testfb_lookups') WHERE EXISTS (SELECT 1 FROM testfb_lookups)";
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL(DB_ERR_OTHER, result->error_class);
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "testfb_lookups"));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "contains data"));
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_ddl_refuse_drop_null);
    RUN_TEST(test_firebase_ddl_refuse_drop_empty);
    RUN_TEST(test_firebase_ddl_refuse_drop_with_row);
    return UNITY_END();
}
