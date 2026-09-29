/*
 * Unity Test File: Firebird Execute SQL
 * Tests firebird_execute_sql() — the core query execution path.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/database_params.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/connection.h>
#include <src/database/firebird/query.h>
#include <src/database/firebird/query_internal.h>

#ifndef USE_MOCK_LIBFBC
#define USE_MOCK_LIBFBC
#endif
#include <unity/mocks/mock_libfbclient.h>

#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

/* Forward declarations for function being tested */
bool firebird_execute_sql(DatabaseHandle* connection, const char* sql,
                          const char* parameters_json, QueryResult** result);

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
void test_execute_sql_null_connection(void);
void test_execute_sql_null_sql(void);
void test_execute_sql_null_result(void);
void test_execute_sql_wrong_engine(void);
void test_execute_sql_null_conn_handle(void);
void test_execute_sql_db_handle_null(void);
void test_execute_sql_dsql_ptrs_unavailable(void);
void test_execute_sql_success_execute_immediate(void);
void test_execute_sql_execute_immediate_failure(void);
void test_execute_sql_start_transaction_unavailable(void);
void test_execute_sql_start_transaction_failure(void);
void test_execute_sql_oom_db_result(void);
void test_execute_sql_oom_rewrite_engine(void);
void test_execute_sql_empty_params_no_rewrite(void);
void test_execute_sql_query_params_parse_failure(void);

void setUp(void) {
    mock_libfbc_reset_all();
    mock_system_reset_all();
    load_libfbclient_functions("test");
}

void tearDown(void) {
    mock_libfbc_reset_all();
    mock_system_reset_all();
}

void test_execute_sql_null_connection(void) {
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebird_execute_sql(NULL, "SELECT 1", NULL, &result));
    TEST_ASSERT_NULL(result);
}

void test_execute_sql_null_sql(void) {
    DatabaseHandle* conn = create_test_handle();
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebird_execute_sql(conn, NULL, NULL, &result));
    TEST_ASSERT_NULL(result);
    destroy_test_handle(conn);
}

void test_execute_sql_null_result(void) {
    DatabaseHandle* conn = create_test_handle();
    TEST_ASSERT_FALSE(firebird_execute_sql(conn, "SELECT 1", NULL, NULL));
    destroy_test_handle(conn);
}

void test_execute_sql_wrong_engine(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_POSTGRESQL;
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebird_execute_sql(&conn, "SELECT 1", NULL, &result));
}

void test_execute_sql_null_conn_handle(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->db_handle = NULL;

    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebird_execute_sql(conn, "SELECT 1", NULL, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(DB_ERR_TRANSPORT, result->error_class);
    TEST_ASSERT_FALSE(result->success);
    free_result(result);

    destroy_test_handle(conn);
}

void test_execute_sql_db_handle_null(void) {
    DatabaseHandle* conn = create_test_handle();
    conn->connection_handle = NULL;

    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebird_execute_sql(conn, "SELECT 1", NULL, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(DB_ERR_TRANSPORT, result->error_class);
    TEST_ASSERT_FALSE(result->success);
    free_result(result);

    destroy_test_handle(conn);
}

void test_execute_sql_dsql_ptrs_unavailable(void) {
    DatabaseHandle* conn = create_test_handle();
    /* Set all DSQL pointers NULL */
    isc_dsql_allocate_ptr = NULL;
    isc_dsql_prepare_ptr = NULL;
    isc_dsql_execute_ptr = NULL;
    isc_dsql_fetch_ptr = NULL;
    isc_dsql_free_statement_ptr = NULL;

    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebird_execute_sql(conn, "SELECT 1", NULL, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(DB_ERR_TRANSPORT, result->error_class);
    TEST_ASSERT_FALSE(result->success);
    free_result(result);

    /* Restore pointers */
    isc_dsql_allocate_ptr = mock_isc_dsql_allocate;
    isc_dsql_prepare_ptr = mock_isc_dsql_prepare;
    isc_dsql_execute_ptr = mock_isc_dsql_execute;
    isc_dsql_fetch_ptr = mock_isc_dsql_fetch;
    isc_dsql_free_statement_ptr = mock_isc_dsql_free_statement;

    destroy_test_handle(conn);
}

void test_execute_sql_success_execute_immediate(void) {
    /* SQL that doesn't expect rows and has no params: uses execute_immediate */
    DatabaseHandle* conn = create_test_handle();

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "CREATE TABLE test (id INTEGER)", NULL, &result);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_NONE, result->error_class);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    TEST_ASSERT_EQUAL(0, result->row_count);
    TEST_ASSERT_EQUAL(0, result->affected_rows);
    free_result(result);
    destroy_test_handle(conn);
}

void test_execute_sql_execute_immediate_failure(void) {
    DatabaseHandle* conn = create_test_handle();
    mock_libfbc_set_isc_dsql_execute_immediate_result(2);  /* error */

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "BAD SQL", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_OTHER, result->error_class);
    free_result(result);

    mock_libfbc_reset_all();
    destroy_test_handle(conn);
}

void test_execute_sql_start_transaction_unavailable(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = NULL;  /* No existing transaction */
    isc_start_transaction_ptr = NULL;

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT 1", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(DB_ERR_TRANSPORT, result->error_class);
    free_result(result);

    isc_start_transaction_ptr = mock_isc_start_transaction;
    destroy_test_handle(conn);
}

void test_execute_sql_start_transaction_failure(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = NULL;
    mock_libfbc_set_isc_start_transaction_result(2);  /* error */

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT 1", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);

    mock_libfbc_reset_all();
    destroy_test_handle(conn);
}

void test_execute_sql_oom_rewrite_engine(void) {
    DatabaseHandle* conn = create_test_handle();
    mock_system_set_malloc_failure(1);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "CREATE TABLE test (id INTEGER)", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);

    mock_system_reset_all();
    destroy_test_handle(conn);
    free_result(result);
}

void test_execute_sql_empty_params_no_rewrite(void) {
    /* Empty params (NULL) — SQL unchanged, uses execute_immediate path */
    DatabaseHandle* conn = create_test_handle();

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "CREATE TABLE test (id INTEGER)", NULL, &result);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    free_result(result);
    destroy_test_handle(conn);
}

void test_execute_sql_query_params_parse_failure(void) {
    /* Invalid JSON params — parse_typed_parameters returns NULL */
    DatabaseHandle* conn = create_test_handle();

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT :name FROM tbl", "not valid json", &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(DB_ERR_OTHER, result->error_class);
    free_result(result);
    destroy_test_handle(conn);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_execute_sql_null_connection);
    RUN_TEST(test_execute_sql_null_sql);
    RUN_TEST(test_execute_sql_null_result);
    RUN_TEST(test_execute_sql_wrong_engine);
    RUN_TEST(test_execute_sql_null_conn_handle);
    RUN_TEST(test_execute_sql_db_handle_null);
    RUN_TEST(test_execute_sql_dsql_ptrs_unavailable);
    RUN_TEST(test_execute_sql_success_execute_immediate);
    RUN_TEST(test_execute_sql_execute_immediate_failure);
    RUN_TEST(test_execute_sql_start_transaction_unavailable);
    RUN_TEST(test_execute_sql_start_transaction_failure);
    RUN_TEST(test_execute_sql_oom_rewrite_engine);
    RUN_TEST(test_execute_sql_empty_params_no_rewrite);
    RUN_TEST(test_execute_sql_query_params_parse_failure);

    return UNITY_END();
}
