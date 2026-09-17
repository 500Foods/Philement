/*
 * Unity tests for firebase_ddl_alter_table().
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
char* min_catalog(const char* collection);
void test_firebase_ddl_alter_table_null(void);
void test_firebase_ddl_alter_add_column(void);
void test_firebase_ddl_alter_drop_column(void);
void test_firebase_ddl_alter_rename(void);
void test_firebase_ddl_alter_rename_with_row(void);
void test_firebase_ddl_alter_add_duplicate(void);
void test_firebase_ddl_alter_drop_missing(void);

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

char* min_catalog(const char* collection) {
    FirebaseSqlTable table = {0};
    FirebaseSqlColumn col = {0};
    col.name = (char*)"account_id";
    col.type = (char*)"integer";
    col.not_null = true;
    table.columns = &col;
    table.column_count = 1;
    return firebase_catalog_document_json(&table, collection, "[]");
}

void test_firebase_ddl_alter_table_null(void) {
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_ddl_alter_table(NULL, NULL, &result));
    TEST_ASSERT_FALSE(result->success);
    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_firebase_ddl_alter_add_column(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    char* catalog = min_catalog("testfb_scripts");
    firebase_http_test_set_method_response("GET", "_schema/testfb_scripts", 200, catalog);
    firebase_http_test_set_method_response("PATCH", "_schema/testfb_scripts", 200, "{}");
    QueryRequest request = {0};
    request.sql_template = (char*)
        "ALTER TABLE testfb_scripts ADD COLUMN invokable integer NOT NULL DEFAULT 0;";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "invokable"));
    free(catalog);
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

void test_firebase_ddl_alter_drop_column(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    FirebaseSqlTable table = {0};
    FirebaseSqlColumn cols[2] = {0};
    cols[0].name = (char*)"account_id";
    cols[0].type = (char*)"integer";
    cols[0].not_null = true;
    cols[1].name = (char*)"invokable";
    cols[1].type = (char*)"integer";
    table.columns = cols;
    table.column_count = 2;
    char* catalog = firebase_catalog_document_json(&table, "testfb_scripts", "[]");
    firebase_http_test_set_method_response("GET", "_schema/testfb_scripts", 200, catalog);
    firebase_http_test_set_method_response("PATCH", "_schema/testfb_scripts", 200, "{}");
    QueryRequest request = {0};
    request.sql_template = (char*)"ALTER TABLE testfb_scripts DROP COLUMN invokable;";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    free(catalog);
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

void test_firebase_ddl_alter_rename(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    char* catalog = min_catalog("testfb_accounts_new");
    firebase_http_test_set_method_response("GET", "_schema/testfb_accounts_new", 200, catalog);
    firebase_http_test_set_method_response("GET", "_schema/testfb_accounts", 404, "{}");
    firebase_http_test_set_method_response("PATCH", "_schema/testfb_accounts", 200, "{}");
    firebase_http_test_set_method_response("GET", "documents/testfb_accounts_new", 200, "{}");
    firebase_http_test_set_method_response("DELETE", "_schema/testfb_accounts_new", 200, "{}");
    QueryRequest request = {0};
    request.sql_template = (char*)"ALTER TABLE testfb_accounts_new RENAME TO accounts;";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_url(), "_schema/testfb_accounts_new"));
    free(catalog);
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

void test_firebase_ddl_alter_rename_with_row(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    char* catalog = min_catalog("testfb_accounts_new");
    firebase_http_test_set_method_response("GET", "_schema/testfb_accounts_new", 200, catalog);
    firebase_http_test_set_method_response("GET", "_schema/testfb_accounts", 404, "{}");
    firebase_http_test_set_method_response("PATCH", "_schema/testfb_accounts", 200, "{}");
    firebase_http_test_set_method_response("GET", "documents/testfb_accounts_new", 200,
        "{\"documents\":[{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_accounts_new/1\",\"fields\":{\"account_id\":{\"integerValue\":\"1\"}}}]}");
    firebase_http_test_set_method_response("PATCH", "testfb_accounts/1", 200, "{}");
    firebase_http_test_set_method_response("GET", "documents/testfb_accounts_new", 200,
        "{\"documents\":[{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_accounts_new/1\",\"fields\":{}}]}");
    firebase_http_test_set_method_response("DELETE", "testfb_accounts_new/1", 200, "{}");
    firebase_http_test_set_method_response("DELETE", "_schema/testfb_accounts_new", 200, "{}");
    QueryRequest request = {0};
    request.sql_template = (char*)"ALTER TABLE testfb_accounts_new RENAME TO accounts;";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    free(catalog);
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

void test_firebase_ddl_alter_add_duplicate(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    char* catalog = min_catalog("testfb_scripts");
    firebase_http_test_set_method_response("GET", "_schema/testfb_scripts", 200, catalog);
    QueryRequest request = {0};
    request.sql_template = (char*)"ALTER TABLE testfb_scripts ADD COLUMN account_id integer;";
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "already exists"));
    free(catalog);
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

void test_firebase_ddl_alter_drop_missing(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    char* catalog = min_catalog("testfb_scripts");
    firebase_http_test_set_method_response("GET", "_schema/testfb_scripts", 200, catalog);
    QueryRequest request = {0};
    request.sql_template = (char*)"ALTER TABLE testfb_scripts DROP COLUMN nope;";
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "does not exist"));
    free(catalog);
    free(result->error_message);
    free(result->data_json);
    free(result);
    destroy_handle(conn);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_ddl_alter_table_null);
    RUN_TEST(test_firebase_ddl_alter_add_column);
    RUN_TEST(test_firebase_ddl_alter_drop_column);
    RUN_TEST(test_firebase_ddl_alter_rename);
    RUN_TEST(test_firebase_ddl_alter_rename_with_row);
    RUN_TEST(test_firebase_ddl_alter_add_duplicate);
    RUN_TEST(test_firebase_ddl_alter_drop_missing);
    return UNITY_END();
}
