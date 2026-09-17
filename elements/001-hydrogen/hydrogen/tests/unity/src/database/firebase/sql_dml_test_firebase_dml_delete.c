/*
 * Unity tests for firebase_dml_delete().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebase/types.h>
#include <src/database/firebase/http.h>
#include <src/database/firebase/connection.h>
#include <src/database/firebase/query.h>
#include <src/database/firebase/sql_dml.h>

static ConnectionConfig g_config;

void destroy_handle(DatabaseHandle* handle);
void release_query_result(QueryResult* result);
DatabaseHandle* connect_testfb(void);
void test_firebase_dml_delete_in_list(void);
void test_firebase_dml_delete_null_connection(void);

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

void test_firebase_dml_delete_null_connection(void) {
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_dml_delete(NULL, NULL, &result));
    TEST_ASSERT_FALSE(result->success);
    release_query_result(result);
}

void test_firebase_dml_delete_in_list(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    firebase_http_test_set_method_response("GET", "documents/testfb_lookups", 200,
        "{\"documents\":["
        "{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_lookups/30_6\","
        "\"fields\":{\"lookup_id\":{\"integerValue\":\"30\"},\"key_idx\":{\"integerValue\":\"6\"}}},"
        "{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_lookups/30_7\","
        "\"fields\":{\"lookup_id\":{\"integerValue\":\"30\"},\"key_idx\":{\"integerValue\":\"7\"}}},"
        "{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_lookups/31_0\","
        "\"fields\":{\"lookup_id\":{\"integerValue\":\"31\"},\"key_idx\":{\"integerValue\":\"0\"}}}"
        "]}");
    firebase_http_test_set_method_response("DELETE", "testfb_lookups/30_6", 200, "{}");
    firebase_http_test_set_method_response("DELETE", "testfb_lookups/30_7", 200, "{}");

    QueryRequest request = {0};
    request.sql_template = (char*)
        "DELETE FROM lookups WHERE lookup_id = 30 AND key_idx IN (6, 7)";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(2, result->affected_rows);
    TEST_ASSERT_EQUAL_STRING("DELETE", firebase_http_test_last_method());
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_url(), "30_7"));
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "documents/testfb_lookups", 200,
        "{\"documents\":["
        "{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_lookups/1_0\","
        "\"fields\":{\"lookup_id\":{\"integerValue\":\"1\"},"
        "\"valid_after\":{\"nullValue\":null}}}"
        "]}");
    firebase_http_test_set_method_response("DELETE", "testfb_lookups/1_0", 200, "{}");
    request.sql_template = (char*)"DELETE FROM lookups WHERE valid_after IS NULL";
    result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(1, result->affected_rows);
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "documents/testfb_lookups", 404, "{}");
    request.sql_template = (char*)"DELETE FROM lookups WHERE lookup_id = 1";
    result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(0, result->affected_rows);
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "documents/testfb_lookups", 200,
        "{\"documents\":[{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_lookups/1_0\","
        "\"fields\":{\"lookup_id\":{\"integerValue\":\"1\"}}}]}");
    firebase_http_test_set_method_response("DELETE", "testfb_lookups/1_0", 500, "{}");
    request.sql_template = (char*)"DELETE FROM lookups WHERE lookup_id = 1";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "DELETE: failed"));
    release_query_result(result);
    destroy_handle(conn);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_dml_delete_null_connection);
    RUN_TEST(test_firebase_dml_delete_in_list);
    return UNITY_END();
}
