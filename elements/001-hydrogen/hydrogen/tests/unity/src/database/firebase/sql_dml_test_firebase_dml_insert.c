/*
 * Unity tests for firebase_dml_insert().
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
char* lookups_catalog_json(void);
char* queries_catalog_json(void);
void test_firebase_dml_insert_lookups_30_6(void);
void test_firebase_dml_insert_queries_brotli(void);
void test_firebase_dml_insert_sha256_now(void);
void test_firebase_dml_insert_rejects(void);
void test_firebase_dml_insert_multi_row(void);
void test_firebase_dml_insert_catalog_and_default(void);

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

char* lookups_catalog_json(void) {
    FirebaseSqlStatement* stmt = firebase_sql_parse(
        "CREATE TABLE lookups ("
        " lookup_id integer NOT NULL,"
        " key_idx integer NOT NULL,"
        " value_txt text,"
        " collection json,"
        " created_at timestamp,"
        " code text,"
        " password_hash text,"
        " PRIMARY KEY(lookup_id, key_idx)"
        ")");
    char* json = firebase_catalog_document_json(&stmt->table, "testfb_lookups", "[]");
    firebase_sql_statement_free(stmt);
    return json;
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

void test_firebase_dml_insert_lookups_30_6(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    char* catalog = lookups_catalog_json();
    TEST_ASSERT_NOT_NULL(catalog);
    firebase_http_test_set_method_response("GET", "_schema/testfb_lookups", 200, catalog);
    firebase_http_test_set_method_response("GET", "testfb_lookups/30_6", 404, "{}");
    firebase_http_test_set_method_response("PATCH", "testfb_lookups/30_6", 200, "{}");
    firebase_now_test_set_unix(1700000000);

    QueryRequest request = {0};
    request.sql_template = (char*)
        "INSERT INTO lookups (lookup_id, key_idx, value_txt, collection, created_at) VALUES ("
        "  030, 6, 'Firebase', FB_JSON_INGEST('{\"icon\":\"fb\"}'), FB_NOW())";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(1, result->affected_rows);
    TEST_ASSERT_EQUAL_STRING("PATCH", firebase_http_test_last_method());
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_url(), "testfb_lookups/30_6"));
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "\"integerValue\":\"30\""));
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "\"integerValue\":\"6\""));
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "Firebase"));
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "{\\\"icon\\\":\\\"fb\\\"}"));
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "2023-11-14T22:13:20Z"));
    release_query_result(result);
    free(catalog);
    destroy_handle(conn);
}

void test_firebase_dml_insert_queries_brotli(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    char* catalog = queries_catalog_json();
    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, catalog);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200, "{}");
    firebase_http_test_set_method_response("GET", "testfb_queries/1", 404, "{}");
    firebase_http_test_set_method_response("PATCH", "testfb_queries/1", 200, "{}");

    QueryRequest request = {0};
    request.sql_template = (char*)
        "INSERT INTO queries (query_id, query_ref, query_type_a28, code) VALUES ("
        "  1, 1001, 1, FB_BROTLI_DECOMPRESS(FB_BASE64_DECODE('iwWASGVsbG8gV29ybGQhAw==')))";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "Hello World!"));
    TEST_ASSERT_NULL(strstr(firebase_http_test_last_body(), "iwWA"));
    release_query_result(result);
    free(catalog);
    destroy_handle(conn);
}

void test_firebase_dml_insert_sha256_now(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    char* catalog = lookups_catalog_json();
    firebase_http_test_set_method_response("GET", "_schema/testfb_lookups", 200, catalog);
    firebase_http_test_set_method_response("GET", "testfb_lookups/0_0", 404, "{}");
    firebase_http_test_set_method_response("PATCH", "testfb_lookups/0_0", 200, "{}");

    QueryRequest request = {0};
    request.sql_template = (char*)
        "INSERT INTO lookups (lookup_id, key_idx, password_hash) VALUES ("
        "  0, 0, FB_SHA256_B64('0', 'testpass'))";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(),
                                "CUQEdl7cgIo2iGBfQmsuosLbdT9uLVpbm/rRJGQlbw0="));
    release_query_result(result);
    free(catalog);
    destroy_handle(conn);
}

void test_firebase_dml_insert_rejects(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    char* catalog = lookups_catalog_json();
    firebase_http_test_set_method_response("GET", "_schema/testfb_lookups", 200, catalog);
    QueryRequest request = {0};
    request.sql_template = (char*)
        "INSERT INTO lookups (lookup_id, value_txt) VALUES (1, 'x')";
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "NOT NULL"));
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "_schema/testfb_lookups", 200, catalog);
    firebase_http_test_set_method_response("GET", "testfb_lookups/1_0", 200, "{}");
    request.sql_template = (char*)
        "INSERT INTO lookups (lookup_id, key_idx) VALUES (1, 0)";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "duplicate primary key"));
    release_query_result(result);

    size_t n = FIREBASE_MAX_FIELD_BYTES + 1;
    const char* prefix = "INSERT INTO lookups (lookup_id, key_idx, value_txt) VALUES (1, 0, '";
    char* sql = malloc(strlen(prefix) + n + 4);
    TEST_ASSERT_NOT_NULL(sql);
    memcpy(sql, prefix, strlen(prefix));
    memset(sql + strlen(prefix), 'A', n);
    memcpy(sql + strlen(prefix) + n, "')", 3);
    firebase_http_test_set_method_response("GET", "_schema/testfb_lookups", 200, catalog);
    request.sql_template = sql;
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "900 KiB"));
    release_query_result(result);
    free(sql);

    char* qcat = queries_catalog_json();
    firebase_http_test_set_method_response("GET", "_schema/testfb_queries", 200, qcat);
    firebase_http_test_set_method_response("GET", "documents/testfb_queries", 200,
        "{\"documents\":[{\"name\":\"projects/hydrodemo/databases/(default)/documents/testfb_queries/9\","
        "\"fields\":{\"query_ref\":{\"integerValue\":\"1001\"},\"query_type_a28\":{\"integerValue\":\"1\"}}}]}");
    firebase_http_test_set_method_response("GET", "testfb_queries/2", 404, "{}");
    request.sql_template = (char*)
        "INSERT INTO queries (query_id, query_ref, query_type_a28, code) VALUES (2, 1001, 1, 'x')";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "UNIQUE"));
    release_query_result(result);
    free(qcat);
    free(catalog);
    destroy_handle(conn);
}

void test_firebase_dml_insert_multi_row(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    char* catalog = lookups_catalog_json();
    firebase_http_test_set_method_response("GET", "_schema/testfb_lookups", 200, catalog);
    firebase_http_test_set_method_response("GET", "testfb_lookups/30_6", 404, "{}");
    firebase_http_test_set_method_response("PATCH", "testfb_lookups/30_6", 200, "{}");
    firebase_http_test_set_method_response("GET", "testfb_lookups/30_7", 404, "{}");
    firebase_http_test_set_method_response("PATCH", "testfb_lookups/30_7", 200, "{}");

    QueryRequest request = {0};
    request.sql_template = (char*)
        "INSERT INTO lookups (lookup_id, key_idx, value_txt) VALUES (30, 6, 'a'), (30, 7, 'b')";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(2, result->affected_rows);
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_url(), "30_7"));
    release_query_result(result);
    free(catalog);
    destroy_handle(conn);
}

void test_firebase_dml_insert_catalog_and_default(void) {
    DatabaseHandle* conn = connect_testfb();
    TEST_ASSERT_NOT_NULL(conn);
    QueryRequest request = {0};
    QueryResult* result = NULL;

    firebase_http_test_set_method_response("GET", "_schema/testfb_lookups", 404, "{}");
    request.sql_template = (char*)"INSERT INTO lookups (lookup_id, key_idx) VALUES (1, 0)";
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "does not exist"));
    release_query_result(result);

    char* catalog = lookups_catalog_json();
    firebase_http_test_set_method_response("GET", "_schema/testfb_lookups", 200, catalog);
    request.sql_template = (char*)
        "INSERT INTO lookups (lookup_id, key_idx, nope) VALUES (1, 0, 'x')";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "unknown column"));
    release_query_result(result);

    firebase_http_test_set_method_response("GET", "_schema/testfb_lookups", 200, catalog);
    firebase_http_test_set_method_response("GET", "testfb_lookups/1_0", 404, "{}");
    firebase_http_test_set_method_response("PATCH", "testfb_lookups/1_0", 200, "{}");
    request.sql_template = (char*)
        "INSERT INTO lookups (lookup_id, key_idx) VALUES (1, 0), (1, 0)";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "duplicate primary key"));
    release_query_result(result);

    FirebaseSqlStatement* schema = firebase_sql_parse(
        "CREATE TABLE nums ("
        " id integer NOT NULL,"
        " score real,"
        " note text DEFAULT 'hi',"
        " PRIMARY KEY(id)"
        ")");
    char* ncat = firebase_catalog_document_json(&schema->table, "testfb_nums", "[]");
    firebase_sql_statement_free(schema);
    firebase_http_test_set_method_response("GET", "_schema/testfb_nums", 200, ncat);
    firebase_http_test_set_method_response("GET", "testfb_nums/1", 404, "{}");
    firebase_http_test_set_method_response("PATCH", "testfb_nums/1", 200, "{}");
    request.sql_template = (char*)"INSERT INTO nums (id, score) VALUES (1, 1.5)";
    result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "doubleValue"));
    TEST_ASSERT_NOT_NULL(strstr(firebase_http_test_last_body(), "hi"));
    release_query_result(result);
    free(ncat);
    free(catalog);

    char* cat_time = lookups_catalog_json();
    firebase_http_test_set_method_response("GET", "_schema/testfb_lookups", 200, cat_time);
    request.sql_template = (char*)
        "INSERT INTO lookups (lookup_id, key_idx, value_txt) VALUES (1, 0, FB_TIME_ADD(1, 2, 'm'))";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_FALSE(result->success);
    release_query_result(result);
    free(cat_time);

    char* cat2 = lookups_catalog_json();
    firebase_http_test_set_method_response("GET", "_schema/testfb_lookups", 200, cat2);
    firebase_http_test_set_method_response("GET", "testfb_lookups/2_0", 404, "{}");
    firebase_http_test_set_method_response("PATCH", "testfb_lookups/2_0", 500, "{}");
    request.sql_template = (char*)"INSERT INTO lookups (lookup_id, key_idx) VALUES (2, 0)";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "PATCH failed"));
    release_query_result(result);
    free(cat2);

    FirebaseSqlStatement* nopk = firebase_sql_parse("CREATE TABLE nopk (id integer NOT NULL)");
    char* npcat = firebase_catalog_document_json(&nopk->table, "testfb_nopk", "[]");
    firebase_sql_statement_free(nopk);
    firebase_http_test_set_method_response("GET", "_schema/testfb_nopk", 200, npcat);
    request.sql_template = (char*)"INSERT INTO nopk (id) VALUES (1)";
    result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(conn, &request, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "missing primary key"));
    release_query_result(result);
    free(npcat);
    destroy_handle(conn);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_dml_insert_lookups_30_6);
    RUN_TEST(test_firebase_dml_insert_queries_brotli);
    RUN_TEST(test_firebase_dml_insert_sha256_now);
    RUN_TEST(test_firebase_dml_insert_rejects);
    RUN_TEST(test_firebase_dml_insert_multi_row);
    RUN_TEST(test_firebase_dml_insert_catalog_and_default);
    return UNITY_END();
}
