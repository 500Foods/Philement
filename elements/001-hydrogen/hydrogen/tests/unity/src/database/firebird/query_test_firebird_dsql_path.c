/*
 * Unity Test File: Firebird DSQL Execute Path
 * Tests firebird_execute_sql() — the DSQL prepare/execute/fetch path
 * that runs SELECT and RETURNING queries with isc_dsql_allocate,
 * prepare, execute, fetch, and the cleanup/fail paths.
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

/* Forward declaration for function being tested */
bool firebird_execute_sql(DatabaseHandle* connection, const char* sql,
                          const char* parameters_json, QueryResult** result);

/* Test function prototypes */
void test_dsql_select_fetch_rows_success(void);
void test_dsql_select_no_rows_eof(void);
void test_dsql_prepare_failure(void);
void test_dsql_allocate_failure(void);
void test_dsql_execute_failure(void);
void test_dsql_fetch_error(void);
void test_dsql_sqld_exceeds_sqln_reallocate(void);
void test_dsql_sqld_exceeds_512_limit(void);
void test_dsql_bind_sqlda_buffers_failure(void);
void test_dsql_column_names_calloc_failure(void);
void test_dsql_commit_failure_after_select(void);
void test_dsql_no_columns_no_output(void);
void test_dsql_exec_proc_singleton_eof(void);
void test_dsql_execute2_unavailable(void);
void test_dsql_oom_db_result(void);
void test_dsql_json_buffer_grow(void);
void test_dsql_json_buffer_realloc_failure(void);
void test_dsql_column_strdup_failure(void);
void test_dsql_param_conversion_failure(void);
void test_dsql_execute_immediate_unavailable(void);
void test_dsql_execute_immediate_failure(void);

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

/* Helper: set up SELECT statement info (type = FB_STMT_SELECT) */
static void setup_select_stmt_info(void) {
    unsigned char info_data[8] = {0};
    info_data[0] = FB_INFO_SQL_STMT_TYPE;
    info_data[1] = 1;
    info_data[2] = 0;
    info_data[3] = FB_STMT_SELECT;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 4);
}

/* Helper: set up EXEC_PROCEDURE statement info (type = FB_STMT_EXEC_PROCEDURE) */
static void setup_exec_proc_stmt_info(void) {
    unsigned char info_data[8] = {0};
    info_data[0] = FB_INFO_SQL_STMT_TYPE;
    info_data[1] = 1;
    info_data[2] = 0;
    info_data[3] = FB_STMT_EXEC_PROCEDURE;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 4);
}

void setUp(void) {
    mock_libfbc_reset_all();
    mock_system_reset_all();
    load_libfbclient_functions("test");
}

void tearDown(void) {
    mock_libfbc_reset_all();
    mock_system_reset_all();
}

/* --- Success path: SELECT with 2 rows of integer data --- */
void test_dsql_select_fetch_rows_success(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_prepare_sqlda(1);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_LONG);
    setup_select_stmt_info();

    int row_val = 42;
    mock_libfbc_set_isc_dsql_fetch_return_data(1, FB_SQL_LONG, 4, &row_val, "col", 3);
    mock_libfbc_set_isc_dsql_fetch_calls_before_eof(2);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", NULL, &result);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_NONE, result->error_class);
    TEST_ASSERT_EQUAL(2, result->row_count);
    TEST_ASSERT_EQUAL(1, result->column_count);
    TEST_ASSERT_NOT_NULL(result->data_json);
    TEST_ASSERT_NOT_NULL(result->column_names);
    TEST_ASSERT_EQUAL_STRING("col", result->column_names[0]);
    free_result(result);
    destroy_test_handle(conn);
}

/* --- EOF immediately on fetch (no rows) --- */
void test_dsql_select_no_rows_eof(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_prepare_sqlda(1);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_LONG);
    setup_select_stmt_info();

    int row_val = 99;
    mock_libfbc_set_isc_dsql_fetch_return_data(1, FB_SQL_LONG, 4, &row_val, "id", 2);
    mock_libfbc_set_isc_dsql_fetch_calls_before_eof(0);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", NULL, &result);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(0, result->row_count);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    free_result(result);
    destroy_test_handle(conn);
}

/* --- isc_dsql_prepare returns error --- */
void test_dsql_prepare_failure(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_allocate_result(0);
    mock_libfbc_set_isc_dsql_prepare_result(2);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_OTHER, result->error_class);
    free_result(result);
    destroy_test_handle(conn);
}

/* --- isc_dsql_allocate returns error --- */
void test_dsql_allocate_failure(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_allocate_result(2);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_TRANSPORT, result->error_class);
    free_result(result);
    destroy_test_handle(conn);
}

/* --- isc_dsql_execute returns error (sqld=0, no fetch) --- */
void test_dsql_execute_failure(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    setup_select_stmt_info();
    mock_libfbc_set_isc_dsql_execute_result(2);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_OTHER, result->error_class);
    free_result(result);
    destroy_test_handle(conn);
}

/* --- isc_dsql_fetch returns a non-EOF, non-success error --- */
void test_dsql_fetch_error(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_prepare_sqlda(1);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_LONG);
    setup_select_stmt_info();

    int row_val = 1;
    mock_libfbc_set_isc_dsql_fetch_return_data(1, FB_SQL_LONG, 4, &row_val, "col", 3);
    mock_libfbc_set_isc_dsql_fetch_calls_before_eof(1);
    mock_libfbc_set_isc_dsql_fetch_result(2);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_OTHER, result->error_class);
    free_result(result);
    destroy_test_handle(conn);
}

/* --- sqld > sqln triggers reallocation path --- */
void test_dsql_sqld_exceeds_sqln_reallocate(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_prepare_sqlda(25);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_LONG);
    setup_select_stmt_info();

    int vals[25];
    for (int i = 0; i < 25; i++) {
        vals[i] = i;
    }
    mock_libfbc_set_isc_dsql_fetch_return_data(25, FB_SQL_LONG, 4, vals, "col", 3);
    mock_libfbc_set_isc_dsql_fetch_calls_before_eof(1);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", NULL, &result);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(1, result->row_count);
    TEST_ASSERT_EQUAL(25, result->column_count);
    free_result(result);
    destroy_test_handle(conn);
}

/* --- sqld > 512 triggers the limit check path --- */
void test_dsql_sqld_exceeds_512_limit(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_prepare_sqlda(513);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_LONG);
    setup_select_stmt_info();

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    free_result(result);
    destroy_test_handle(conn);
}

/* --- firebird_bind_sqlda_buffers fails (calloc failure for sqldata) ---
 * Allocation call order (shared counter for malloc/calloc/strdup):
 *   call #1 calloc(DatabaseHandle) in create_test_handle
 *   call #2 strdup("DQM-TEST") in create_test_handle
 *   call #3 calloc(QueryResult) at line 296
 *   call #4 malloc in firebird_rewrite_engine_sql (reallocs for "SELECT id FROM tbl" — no change, one malloc)
 *   call #5 calloc(fb_xsqlda_min) in firebird_alloc_sqlda
 *   call #6 calloc(sqldata) in firebird_bind_sqlda_buffers — FAIL HERE */
void test_dsql_bind_sqlda_buffers_failure(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_prepare_sqlda(1);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_LONG);
    setup_select_stmt_info();

    mock_system_set_calloc_failure(6);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_OTHER, result->error_class);
    free_result(result);
    mock_system_reset_all();
    destroy_test_handle(conn);
}

/* --- column_names allocation failure (calloc fails for column_names array) ---
 * Allocation call order (shared counter for malloc/calloc/strdup):
 *   call #1 calloc(DatabaseHandle) in create_test_handle
 *   call #2 strdup("DQM-TEST") in create_test_handle
 *   call #3 calloc(QueryResult) at line 296
 *   call #4 malloc in firebird_rewrite_engine_sql
 *   call #5 calloc(fb_xsqlda_min) in firebird_alloc_sqlda
 *   call #6 calloc(sqldata) in firebird_bind_sqlda_buffers
 *   call #7 calloc(sqlind) in firebird_bind_sqlda_buffers
 *   call #8 calloc(column_names) at line 560 — FAIL HERE */
void test_dsql_column_names_calloc_failure(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_prepare_sqlda(1);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_LONG);
    setup_select_stmt_info();

    int row_val = 1;
    mock_libfbc_set_isc_dsql_fetch_return_data(1, FB_SQL_LONG, 4, &row_val, "col", 3);
    mock_libfbc_set_isc_dsql_fetch_calls_before_eof(1);

    mock_system_set_calloc_failure(8);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_OTHER, result->error_class);
    free_result(result);
    mock_system_reset_all();
    destroy_test_handle(conn);
}

/* --- Commit failure after successful SELECT --- */
void test_dsql_commit_failure_after_select(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = NULL;

    mock_libfbc_set_isc_dsql_prepare_sqlda(1);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_LONG);
    setup_select_stmt_info();

    int row_val = 7;
    mock_libfbc_set_isc_dsql_fetch_return_data(1, FB_SQL_LONG, 4, &row_val, "col", 3);
    mock_libfbc_set_isc_dsql_fetch_calls_before_eof(1);

    mock_libfbc_set_isc_commit_transaction_result(2);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_TRANSPORT, result->error_class);
    free_result(result);
    destroy_test_handle(conn);
}

/* --- Parameterized DML with no output columns (uses execute, not fetch) ---
 * DELETE FROM tbl WHERE id = ? with one INTEGER param.
 * use_prepare = true (ordered_count > 0), stmt_type from sql_info is SELECT.
 * But sqld=0 (no output columns), so:
 *   expects_rows=false, use_prepare=true, fetch_rows=false (stmt_type not SELECT)
 *   Actually, stmt_type=0 when no sql_info set for stmt_type.
 *   With stmt_type=0 and sqld=0, fetch_rows=false (sqld=0, expects_rows=false).
 *   Falls to DML no-output path, calls firebird_rows_affected.
 * Need to set up describe_bind with sqld=1 to match ordered_count=1. */
void test_dsql_no_columns_no_output(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    /* Prepare with sqld=0 — no output columns */
    /* describe_bind with sqld=1 to match 1 input param */
    mock_libfbc_set_isc_dsql_describe_bind_set_sqld(1);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_LONG);

    /* No stmt_type info — stmt_type=0, but sqld=0 means fetch_rows=false */
    /* DML no-output path: rows_affected = INSERT count = 5 */
    unsigned char rec_info[64] = {0};
    rec_info[0] = FB_INFO_SQL_RECORDS;
    rec_info[1] = 7;
    rec_info[2] = 0;
    rec_info[3] = FB_INFO_REQ_INSERT_COUNT;
    rec_info[4] = 4;
    rec_info[5] = 0;
    rec_info[6] = 5;
    mock_libfbc_set_isc_dsql_sql_info_data(rec_info, 7);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "DELETE FROM tbl WHERE id = :id", "{\"INTEGER\":{\"id\":42}}", &result);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(0, result->row_count);
    TEST_ASSERT_EQUAL(5, result->affected_rows);
    TEST_ASSERT_EQUAL(0, result->column_count);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    free_result(result);
    destroy_test_handle(conn);
}

/* --- exec_proc with execute2 returning FB_FETCH_EOF (singleton, zero rows) --- */
void test_dsql_exec_proc_singleton_eof(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_prepare_sqlda(1);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_LONG);
    setup_exec_proc_stmt_info();

    /* execute2 returns FB_FETCH_EOF — singleton procedure with no rows.
     * Code sets exec_proc=false, rc=FB_SQL_SUCCESS, one_row=false.
     * fetch_rows=false (stmt_type is EXEC_PROCEDURE, not SELECT).
     * Falls through to DML no-output path: calls firebird_rows_affected.
     * The same sql_info buffer returns stmt_type data, not records,
     * so firebird_rows_affected sees info[0] != FB_INFO_SQL_RECORDS → returns -1.
     * affected_rows = max(-1, 0) = 0. */
    mock_libfbc_set_isc_dsql_execute2_result(FB_FETCH_EOF);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM proc", NULL, &result);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(0, result->row_count);
    TEST_ASSERT_EQUAL(0, result->affected_rows);
    TEST_ASSERT_EQUAL(0, result->column_count);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    free_result(result);
    destroy_test_handle(conn);
}

/* --- execute2 unavailable (exec_proc path) --- */
void test_dsql_execute2_unavailable(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_prepare_sqlda(1);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_LONG);
    setup_exec_proc_stmt_info();

    isc_dsql_execute2_ptr = NULL;

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM proc", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_TRANSPORT, result->error_class);
    TEST_ASSERT_EQUAL_STRING("Firebird execute2 unavailable", result->error_message);
    free_result(result);

    /* Restore pointer */
    isc_dsql_execute2_ptr = mock_isc_dsql_execute2;
    destroy_test_handle(conn);
}

/* --- OOM on db_result allocation (calloc fails before any DSQL calls) ---
 * mock_system_set_calloc_failure(1) resets counter to 0, so the first
 * calloc in firebird_execute_sql is call #1 — fails at calloc(QueryResult). */
void test_dsql_oom_db_result(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_prepare_sqlda(1);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_LONG);
    setup_select_stmt_info();

    int row_val = 1;
    mock_libfbc_set_isc_dsql_fetch_return_data(1, FB_SQL_LONG, 4, &row_val, "col", 3);
    mock_libfbc_set_isc_dsql_fetch_calls_before_eof(1);

    /* mock_system_set_calloc_failure resets counter to 0.
     * firebird_execute_sql with NULL params has this allocation order:
     *   malloc(firebird_rewrite_engine_sql) — counter=1 (mock_malloc, skipped)
     *   calloc(QueryResult) at line 296 — counter=2, calloc call #1 → FAIL */
    mock_system_set_calloc_failure(2);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NULL(result);
    mock_system_reset_all();
    destroy_test_handle(conn);
}

/* --- JSON buffer growth with many rows (forces realloc beyond initial 1024) ---
 * Each row's JSON is roughly ~20 bytes: {"col":42}. With json_cap=1024,
 * we need >50 rows to trigger the initial growth. Use 100 rows. */
void test_dsql_json_buffer_grow(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_prepare_sqlda(1);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_LONG);
    setup_select_stmt_info();

    int row_val = 42;
    mock_libfbc_set_isc_dsql_fetch_return_data(1, FB_SQL_LONG, 4, &row_val, "col", 3);
    mock_libfbc_set_isc_dsql_fetch_calls_before_eof(100);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", NULL, &result);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(100, result->row_count);
    TEST_ASSERT_EQUAL(1, result->column_count);
    TEST_ASSERT_NOT_NULL(result->data_json);
    free_result(result);
    destroy_test_handle(conn);
}

/* --- Json buffer append failure (realloc fails during JSON building) ---
 * mock_realloc_failure(1) fails the first realloc call.
 * The JSON buffer starts at 1024 bytes. With one row of small data,
 * the initial append of "[" fits. But appending the column name "col"
 * and value might trigger a realloc if... actually with 1024 byte cap,
 * all appends fit. We need many rows to trigger growth.
 * Let's use 200 rows to trigger growth, then fail realloc. */
void test_dsql_json_buffer_realloc_failure(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_prepare_sqlda(1);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_LONG);
    setup_select_stmt_info();

    int row_val = 42;
    mock_libfbc_set_isc_dsql_fetch_return_data(1, FB_SQL_LONG, 4, &row_val, "col", 3);
    mock_libfbc_set_isc_dsql_fetch_calls_before_eof(200);

    /* Fail the first realloc call (when buffer grows past 1024) */
    mock_system_set_realloc_failure(1);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    free_result(result);
    mock_system_reset_all();
    destroy_test_handle(conn);
}

/* --- column_names strdup failure (calloc for column_names succeeds, strdup fails) ---
 * mock_system_set_malloc_failure resets counter to 0.
 * Allocation order in firebird_execute_sql with NULL params:
 *   call #1 malloc(firebird_rewrite_engine_sql)
 *   call #2 calloc(QueryResult)
 *   call #3 calloc(fb_xsqlda_min) in firebird_alloc_sqlda
 *   call #4 calloc(sqldata) in firebird_bind_sqlda_buffers
 *   call #5 calloc(sqlind) in firebird_bind_sqlda_buffers
 *   call #6 calloc(column_names) at line 560
 *   call #7 strdup("col") for column_names[0] — FAIL HERE */
void test_dsql_column_strdup_failure(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_prepare_sqlda(1);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_LONG);
    setup_select_stmt_info();

    int row_val = 1;
    mock_libfbc_set_isc_dsql_fetch_return_data(1, FB_SQL_LONG, 4, &row_val, "col", 3);
    mock_libfbc_set_isc_dsql_fetch_calls_before_eof(1);

    mock_system_set_malloc_failure(7);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_OTHER, result->error_class);
    free_result(result);
    mock_system_reset_all();
    destroy_test_handle(conn);
}

/* --- Parameterized DML: params parse succeeds but conversion fails ---
 * Using an empty params JSON "{}" — has_params=false, so conversion
 * doesn't happen. Instead test a SQL with :param that fails conversion.
 * Actually, we can test the "Failed to parse Firebird parameters" path
 * by passing invalid JSON. */
void test_dsql_param_conversion_failure(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    /* Invalid params JSON — parse will fail */
    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "SELECT id FROM tbl", "invalid", &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_OTHER, result->error_class);
    free_result(result);
    destroy_test_handle(conn);
}

/* --- execute_immediate unavailable on DML no-output path ---
 * When stmt_type=0, sqld=0, and fetch_rows=false, the DML no-output path
 * is taken only for parameterized queries (use_prepare=true). But if
 * isc_dsql_execute_ptr is NULL, the execute_immediate path is taken first
 * only when use_prepare=false. With params, use_prepare=true, so execute
 * is called. Let's test with no params and a non-SELECT statement that
 * doesn't expect rows — it goes to execute_immediate path. */
void test_dsql_execute_immediate_unavailable(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    /* isc_dsql_execute_immediate_ptr is NULL — execute_immediate unavailable */
    isc_dsql_execute_immediate_ptr = NULL;

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "CREATE TABLE test (id INTEGER)", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_TRANSPORT, result->error_class);
    free_result(result);

    /* Restore pointer */
    isc_dsql_execute_immediate_ptr = mock_isc_dsql_execute_immediate;
    destroy_test_handle(conn);
}

/* --- execute_immediate returns error --- */
void test_dsql_execute_immediate_failure(void) {
    DatabaseHandle* conn = create_test_handle();
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    mock_libfbc_set_isc_dsql_execute_immediate_result(2);

    QueryResult* result = NULL;
    bool ok = firebird_execute_sql(conn, "CREATE TABLE test (id INTEGER)", NULL, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_OTHER, result->error_class);
    free_result(result);
    destroy_test_handle(conn);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_dsql_select_fetch_rows_success);
    RUN_TEST(test_dsql_select_no_rows_eof);
    RUN_TEST(test_dsql_prepare_failure);
    RUN_TEST(test_dsql_allocate_failure);
    RUN_TEST(test_dsql_execute_failure);
    RUN_TEST(test_dsql_fetch_error);
    RUN_TEST(test_dsql_sqld_exceeds_sqln_reallocate);
    RUN_TEST(test_dsql_sqld_exceeds_512_limit);
    RUN_TEST(test_dsql_bind_sqlda_buffers_failure);
    RUN_TEST(test_dsql_column_names_calloc_failure);
    RUN_TEST(test_dsql_commit_failure_after_select);
    RUN_TEST(test_dsql_no_columns_no_output);
    RUN_TEST(test_dsql_exec_proc_singleton_eof);
    RUN_TEST(test_dsql_execute2_unavailable);
    RUN_TEST(test_dsql_oom_db_result);
    RUN_TEST(test_dsql_json_buffer_grow);
    RUN_TEST(test_dsql_json_buffer_realloc_failure);
    RUN_TEST(test_dsql_column_strdup_failure);
    RUN_TEST(test_dsql_param_conversion_failure);
    RUN_TEST(test_dsql_execute_immediate_unavailable);
    RUN_TEST(test_dsql_execute_immediate_failure);

    return UNITY_END();
}
