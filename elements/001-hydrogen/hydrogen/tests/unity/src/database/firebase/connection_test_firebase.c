/*
 * Unity tests for Firebase connect / health / disconnect via the HTTP seam.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebase/types.h>
#include <src/database/firebase/http.h>
#include <src/database/firebase/connection.h>

void destroy_firebase_handle(DatabaseHandle* handle);
void test_firebase_connect_null_args(void);
void test_firebase_connect_missing_project(void);
void test_firebase_connect_production_host_refused(void);
void test_firebase_connect_health_failure(void);
void test_firebase_connect_and_health_success(void);
void test_firebase_disconnect_null_and_wrong_engine(void);
void test_firebase_health_check_null(void);
void test_firebase_health_check_wrong_engine(void);
void test_firebase_reset_and_cancel(void);

void setUp(void) {
    firebase_http_test_clear_responses();
}

void tearDown(void) {
    firebase_http_test_clear_responses();
}

void destroy_firebase_handle(DatabaseHandle* handle) {
    if (!handle) {
        return;
    }
    firebase_disconnect(handle);
    free(handle->designator);
    pthread_mutex_destroy(&handle->connection_lock);
    free(handle);
}

void test_firebase_connect_null_args(void) {
    ConnectionConfig config = {0};
    DatabaseHandle* conn = NULL;
    TEST_ASSERT_FALSE(firebase_connect(NULL, &conn, "t"));
    TEST_ASSERT_FALSE(firebase_connect(&config, NULL, "t"));
}

void test_firebase_connect_missing_project(void) {
    ConnectionConfig config = {0};
    config.host = (char*)"127.0.0.1";
    config.port = 8080;
    DatabaseHandle* conn = NULL;
    TEST_ASSERT_FALSE(firebase_connect(&config, &conn, "t"));
}

void test_firebase_connect_production_host_refused(void) {
    ConnectionConfig config = {0};
    config.username = (char*)"hydrodemo";
    config.host = (char*)"firestore.googleapis.com";
    config.port = 443;
    DatabaseHandle* conn = NULL;
    TEST_ASSERT_FALSE(firebase_connect(&config, &conn, "t"));
}

void test_firebase_connect_health_failure(void) {
    firebase_http_test_set_response("documents/", 500, "err");
    ConnectionConfig config = {0};
    config.username = (char*)"hydrodemo";
    config.host = (char*)"127.0.0.1";
    config.port = 8080;
    config.database = (char*)"(default)";
    DatabaseHandle* conn = NULL;
    TEST_ASSERT_FALSE(firebase_connect(&config, &conn, "t"));
    TEST_ASSERT_NULL(conn);
}

void test_firebase_connect_and_health_success(void) {
    firebase_http_test_set_response("documents/", 200, "{}");
    firebase_http_test_set_response("documents/", 200, "{}");
    ConnectionConfig config = {0};
    config.username = (char*)"hydrodemo";
    config.host = (char*)"127.0.0.1";
    config.port = 8080;
    config.database = (char*)"(default)";
    config.schema = (char*)"testfb";
    DatabaseHandle* conn = NULL;
    TEST_ASSERT_TRUE(firebase_connect(&config, &conn, "dqm-fb"));
    TEST_ASSERT_NOT_NULL(conn);
    TEST_ASSERT_EQUAL(DB_ENGINE_FIREBASE, conn->engine_type);
    TEST_ASSERT_EQUAL(DB_CONNECTION_CONNECTED, conn->status);
    TEST_ASSERT_TRUE(firebase_health_check(conn));
    FirebaseConnection* fb = (FirebaseConnection*)conn->connection_handle;
    TEST_ASSERT_NOT_NULL(fb);
    TEST_ASSERT_TRUE(fb->emulator);
    TEST_ASSERT_EQUAL_STRING("hydrodemo", fb->project);
    TEST_ASSERT_EQUAL_STRING("testfb", fb->schema);
    destroy_firebase_handle(conn);
}

void test_firebase_disconnect_null_and_wrong_engine(void) {
    TEST_ASSERT_FALSE(firebase_disconnect(NULL));
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_SQLITE;
    TEST_ASSERT_FALSE(firebase_disconnect(&handle));
}

void test_firebase_health_check_null(void) {
    TEST_ASSERT_FALSE(firebase_health_check(NULL));
}

void test_firebase_health_check_wrong_engine(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_POSTGRESQL;
    TEST_ASSERT_FALSE(firebase_health_check(&handle));
}

void test_firebase_reset_and_cancel(void) {
    firebase_http_test_set_response("documents/", 200, "{}");
    firebase_http_test_set_response("documents/", 200, "{}");
    ConnectionConfig config = {0};
    config.username = (char*)"hydrodemo";
    config.host = (char*)"127.0.0.1";
    config.port = 8080;
    DatabaseHandle* conn = NULL;
    TEST_ASSERT_TRUE(firebase_connect(&config, &conn, NULL));
    TEST_ASSERT_TRUE(firebase_reset_connection(conn));
    firebase_cancel_inflight(conn);
    firebase_cancel_inflight(NULL);
    destroy_firebase_handle(conn);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_connect_null_args);
    RUN_TEST(test_firebase_connect_missing_project);
    RUN_TEST(test_firebase_connect_production_host_refused);
    RUN_TEST(test_firebase_connect_health_failure);
    RUN_TEST(test_firebase_connect_and_health_success);
    RUN_TEST(test_firebase_disconnect_null_and_wrong_engine);
    RUN_TEST(test_firebase_health_check_null);
    RUN_TEST(test_firebase_health_check_wrong_engine);
    RUN_TEST(test_firebase_reset_and_cancel);
    return UNITY_END();
}
