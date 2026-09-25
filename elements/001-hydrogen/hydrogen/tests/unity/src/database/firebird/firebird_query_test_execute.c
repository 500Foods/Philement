/*
 * Unity Test File: Firebird Execute Query and Prepare
 * Tests firebird_execute_query() and firebird_execute_prepared() —
 * the public entry points that validate parameters and delegate to
 * firebird_execute_sql().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/connection.h>
#include <src/database/firebird/query.h>

#ifndef USE_MOCK_LIBFBC
#define USE_MOCK_LIBFBC
#endif
#include <unity/mocks/mock_libfbclient.h>

#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

/* Forward declarations for function being tested */
bool firebird_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool firebird_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt,
                               QueryRequest* request, QueryResult** result);

/* Helper: create a Firebird DatabaseHandle for tests */
static DatabaseHandle* create_test_handle(void) {
    FirebirdConnection* fb_conn = firebird_create_connection_wrapper();
    fb_conn->db_handle = (void*)0xDEADBEEF;

    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    if (!conn) {
        firebird_destroy_connection_wrapper(fb_conn);
        return NULL;
    }
    conn->engine_type = DB_ENGINE_FIREBIRD;
    conn->connection_handle = fb_conn;
    conn->config = NULL;
    conn->designator = strdup("DQM-TEST");
    if (!conn->designator) {
        free(conn);
        firebird_destroy_connection_wrapper(fb_conn);
        return NULL;
    }
    conn->status = DB_CONNECTION_CONNECTED;
    pthread_mutex_init(&conn->connection_lock, NULL);
    return conn;
}

static void destroy_test_handle(DatabaseHandle* conn) {
    if (conn) {
        FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
        if (fb_conn) {
            firebird_destroy_connection_wrapper(fb_conn);
        }
        pthread_mutex_destroy(&conn->connection_lock);
        if (conn->designator) free((void*)conn->designator);
        free(conn);
    }
}

static void free_result(QueryResult* r) {
    if (!r) return;
    free(r->data_json);
    free(r->error_message);
    if (r->column_names) {
        for (size_t i = 0; i < r->column_count; i++) {
            free(r->column_names[i]);
        }
        free(r->column_names);
    }
    free(r);
}

/* Test function prototypes */
void test_execute_query_null_connection(void);
void test_execute_query_null_request(void);
void test_execute_query_null_result(void);
void test_execute_query_wrong_engine(void);
void test_execute_query_null_sql_template(void);
void test_execute_query_success_no_params(void);
void test_execute_prepared_null_connection(void);
void test_execute_prepared_null_stmt(void);
void test_execute_prepared_null_result(void);
void test_execute_prepared_wrong_engine(void);
void test_execute_prepared_null_sql_template(void);
void test_execute_prepared_success_no_params(void);
void test_execute_prepared_null_request(void);

void setUp(void) {
    mock_libfbc_reset_all();
    mock_system_reset_all();
    load_libfbclient_functions("test");
}

void tearDown(void) {
    mock_libfbc_reset_all();
    mock_system_reset_all();
}

void test_execute_query_null_connection(void) {
    QueryRequest req = {0};
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebird_execute_query(NULL, &req, &result));
    TEST_ASSERT_NULL(result);
}

void test_execute_query_null_request(void) {
    DatabaseHandle* conn = create_test_handle();
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebird_execute_query(conn, NULL, &result));
    TEST_ASSERT_NULL(result);
    destroy_test_handle(conn);
}

void test_execute_query_null_result(void) {
    DatabaseHandle* conn = create_test_handle();
    QueryRequest req = {0};
    req.sql_template = strdup("SELECT 1");
    TEST_ASSERT_FALSE(firebird_execute_query(conn, &req, NULL));
    free(req.sql_template);
    destroy_test_handle(conn);
}

void test_execute_query_wrong_engine(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_POSTGRESQL;
    QueryRequest req = {0};
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebird_execute_query(&conn, &req, &result));
}

void test_execute_query_null_sql_template(void) {
    DatabaseHandle* conn = create_test_handle();
    QueryRequest req = {0};
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebird_execute_query(conn, &req, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(DB_ERR_OTHER, result->error_class);
    free_result(result);
    destroy_test_handle(conn);
}

void test_execute_query_success_no_params(void) {
    DatabaseHandle* conn = create_test_handle();
    QueryRequest req = {0};
    req.sql_template = strdup("CREATE TABLE test (id INTEGER)");
    QueryResult* result = NULL;
    bool ok = firebird_execute_query(conn, &req, &result);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    free_result(result);
    free(req.sql_template);
    destroy_test_handle(conn);
}

void test_execute_prepared_null_connection(void) {
    PreparedStatement stmt = {0};
    QueryRequest req = {0};
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebird_execute_prepared(NULL, &stmt, &req, &result));
    TEST_ASSERT_NULL(result);
}

void test_execute_prepared_null_stmt(void) {
    DatabaseHandle* conn = create_test_handle();
    QueryRequest req = {0};
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebird_execute_prepared(conn, NULL, &req, &result));
    TEST_ASSERT_NULL(result);
    destroy_test_handle(conn);
}

void test_execute_prepared_null_result(void) {
    DatabaseHandle* conn = create_test_handle();
    PreparedStatement stmt = {0};
    stmt.sql_template = strdup("SELECT 1");
    QueryRequest req = {0};
    TEST_ASSERT_FALSE(firebird_execute_prepared(conn, &stmt, &req, NULL));
    free(stmt.sql_template);
    destroy_test_handle(conn);
}

void test_execute_prepared_wrong_engine(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_POSTGRESQL;
    PreparedStatement stmt = {0};
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebird_execute_prepared(&conn, &stmt, NULL, &result));
}

void test_execute_prepared_null_sql_template(void) {
    DatabaseHandle* conn = create_test_handle();
    PreparedStatement stmt = {0};
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebird_execute_prepared(conn, &stmt, NULL, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(DB_ERR_OTHER, result->error_class);
    free_result(result);
    destroy_test_handle(conn);
}

void test_execute_prepared_success_no_params(void) {
    DatabaseHandle* conn = create_test_handle();
    PreparedStatement stmt = {0};
    stmt.sql_template = strdup("CREATE TABLE test (id INTEGER)");
    QueryResult* result = NULL;
    bool ok = firebird_execute_prepared(conn, &stmt, NULL, &result);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    free_result(result);
    free(stmt.sql_template);
    destroy_test_handle(conn);
}

void test_execute_prepared_null_request(void) {
    DatabaseHandle* conn = create_test_handle();
    PreparedStatement stmt = {0};
    stmt.sql_template = strdup("CREATE TABLE test (id INTEGER)");
    QueryResult* result = NULL;
    /* request can be NULL — params will be NULL */
    bool ok = firebird_execute_prepared(conn, &stmt, NULL, &result);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    free_result(result);
    free(stmt.sql_template);
    destroy_test_handle(conn);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_execute_query_null_connection);
    RUN_TEST(test_execute_query_null_request);
    RUN_TEST(test_execute_query_null_result);
    RUN_TEST(test_execute_query_wrong_engine);
    RUN_TEST(test_execute_query_null_sql_template);
    RUN_TEST(test_execute_query_success_no_params);
    RUN_TEST(test_execute_prepared_null_connection);
    RUN_TEST(test_execute_prepared_null_stmt);
    RUN_TEST(test_execute_prepared_null_result);
    RUN_TEST(test_execute_prepared_wrong_engine);
    RUN_TEST(test_execute_prepared_null_sql_template);
    RUN_TEST(test_execute_prepared_success_no_params);
    RUN_TEST(test_execute_prepared_null_request);

    return UNITY_END();
}
