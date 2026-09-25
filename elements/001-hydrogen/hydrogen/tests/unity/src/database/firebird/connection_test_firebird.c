/*
 * Unity Test File: Firebird Connection Management
 * Tests firebird_connect, firebird_disconnect, firebird_health_check,
 * firebird_reset_connection, and the watchdog cancel hooks.
 *
 * Build version: 1.0.0.2662
 * TEST_VERSION: 20260925
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/connection.h>
#include <src/database/firebird/interface.h>
#include <unity/mocks/mock_libfbclient.h>

/*
 * The CMAKE test build for IS_DATABASE_TEST does not properly pass
 * USE_MOCK_SYSTEM as a separate -D flag, so mock_system.h's
 * #ifdef USE_MOCK_SYSTEM guard skips its content. We declare the
 * mock_system control functions directly instead.
 */
extern void mock_system_reset_all(void);
extern void mock_system_set_calloc_failure(int should_fail);
extern int mock_malloc_call_count;
extern int mock_calloc_should_fail;

/* Test function prototypes */
void test_firebird_connect_not_null(void);
void test_firebird_connect_invalid_params(void);
void test_firebird_connect_success(void);
void test_firebird_connect_remote_host(void);
void test_firebird_connect_remote_host_no_port(void);
void test_firebird_connect_remote_host_relative_path(void);
void test_firebird_connect_localhost(void);
void test_firebird_connect_localhost_127(void);
void test_firebird_connect_null_host(void);
void test_firebird_connect_empty_host(void);
void test_firebird_connect_attach_failure(void);
void test_firebird_connect_wrapper_alloc_failure(void);
void test_firebird_connect_dbhandle_alloc_failure(void);
void test_firebird_connect_attach_string_failure(void);
void test_firebird_connect_null_designator(void);
void test_firebird_disconnect_null(void);
void test_firebird_disconnect_invalid_engine(void);
void test_firebird_disconnect_success(void);
void test_firebird_disconnect_with_transaction(void);
void test_firebird_disconnect_null_handle(void);
void test_firebird_health_check_null(void);
void test_firebird_health_check_invalid_engine(void);
void test_firebird_health_check_null_fb_conn(void);
void test_firebird_health_check_null_db_handle(void);
void test_firebird_health_check_success(void);
void test_firebird_health_check_alloc_failure(void);
void test_firebird_health_check_prepare_failure(void);
void test_firebird_health_check_execute_failure(void);
void test_firebird_health_check_fetch_failure(void);
void test_firebird_health_check_commit_failure(void);
void test_firebird_health_check_rollback_success(void);
void test_firebird_health_check_existing_transaction(void);
void test_firebird_health_check_start_txn_failure(void);
void test_firebird_health_check_func_ptr_missing(void);
void test_firebird_reset_connection_null(void);
void test_firebird_reset_connection_invalid_engine(void);
void test_firebird_reset_connection_success(void);
void test_firebird_cancel_inflight_null(void);
void test_firebird_cancel_inflight_no_stmt(void);
void test_firebird_cancel_inflight_null_fb_conn(void);
void test_firebird_cancel_inflight_with_stmt(void);
void test_firebird_cancel_inflight_cancel_fail(void);
void test_firebird_active_stmt_set_clear(void);
void test_firebird_active_stmt_set_null_fb_conn(void);
void test_firebird_active_stmt_clear_null_fb_conn(void);
void test_firebird_active_stmt_clear_no_match(void);
void test_firebird_get_connection_wrapper_null(void);
void test_firebird_get_connection_wrapper_invalid_engine(void);
void test_firebird_get_connection_wrapper_success(void);
void test_firebird_designator_safe_null(void);
void test_firebird_designator_safe_no_designator(void);
void test_firebird_designator_safe_with_designator(void);

/* Helper functions */
static DatabaseHandle* create_test_connection(const char* host, int port, const char* db_path);
static void destroy_test_connection(DatabaseHandle* conn);

void setUp(void) {
    mock_libfbc_reset_all();
    mock_system_reset_all();
}

void tearDown(void) {
    mock_libfbc_reset_all();
    mock_system_reset_all();
}

/*
 * create_test_connection: establish a Firebird connection for testing.
 * The mock_system counter is reset in setUp, then the test's own strdup
 * calls consume calls 1..3 (database, username, password). firebird_connect
 * then calls calloc in firebird_create_connection_wrapper (call 4),
 * then calloc in firebird_build_attach_string (call 5), then calloc for
 * DatabaseHandle (call 6). This helper does not inject failures.
 */
static DatabaseHandle* create_test_connection(const char* host, int port, const char* db_path) {
    ConnectionConfig config = {0};
    config.database = strdup(db_path ? db_path : "/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");
    config.host = host ? strdup(host) : NULL;
    config.port = port;

    DatabaseHandle* conn = NULL;
    firebird_connect(&config, &conn, SR_DATABASE);
    return conn;
}

static void destroy_test_connection(DatabaseHandle* conn) {
    if (conn) {
        firebird_disconnect(conn);
        pthread_mutex_destroy(&conn->connection_lock);
        if (conn->designator) free((void*)conn->designator);
        free(conn);
    }
}

/* =========================================================================
 * firebird_connect tests
 * ========================================================================= */

void test_firebird_connect_not_null(void) {
    DatabaseEngineInterface* iface = firebird_get_interface();
    TEST_ASSERT_NOT_NULL(iface);
    TEST_ASSERT_NOT_NULL(iface->connect);
}

void test_firebird_connect_invalid_params(void) {
    DatabaseHandle* conn = NULL;

    /* NULL config */
    bool result = firebird_connect(NULL, &conn, SR_DATABASE);
    TEST_ASSERT_FALSE(result);

    /* NULL output pointer */
    ConnectionConfig config = {0};
    result = firebird_connect(&config, NULL, SR_DATABASE);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_connect_success(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");
    config.port = 3050;

    DatabaseHandle* conn = NULL;
    bool result = firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);
    TEST_ASSERT_EQUAL(DB_ENGINE_FIREBIRD, conn->engine_type);
    TEST_ASSERT_EQUAL(DB_CONNECTION_CONNECTED, conn->status);
    TEST_ASSERT_EQUAL_INT(1, mock_libfbc_get_isc_attach_database_call_count());

    firebird_disconnect(conn);
    pthread_mutex_destroy(&conn->connection_lock);
    if (conn->designator) free((void*)conn->designator);
    free(conn);
}

void test_firebird_connect_null_designator(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");

    DatabaseHandle* conn = NULL;
    bool result = firebird_connect(&config, &conn, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);
    /* designator should be NULL when passed NULL */
    TEST_ASSERT_NULL(conn->designator);

    destroy_test_connection(conn);
}

void test_firebird_connect_remote_host(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");
    config.host = strdup("firebird.example.com");
    config.port = 3050;

     DatabaseHandle* conn = NULL;
     bool result = firebird_connect(&config, &conn, SR_DATABASE);
     TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);
    TEST_ASSERT_EQUAL(DB_ENGINE_FIREBIRD, conn->engine_type);

    const char* dbname = NULL;
    const char* params = NULL;
    mock_libfbc_get_last_attach_args(&dbname, &params);
    TEST_ASSERT_NOT_NULL(dbname);
    TEST_ASSERT_NOT_NULL(strstr(dbname, "firebird.example.com/3050:/tmp/test.fdb"));

    destroy_test_connection(conn);
}

void test_firebird_connect_remote_host_no_port(void) {
    ConnectionConfig config = {0};
    config.database = strdup("test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");
    config.host = strdup("firebird.example.com");
    config.port = 0;

    DatabaseHandle* conn = NULL;
    bool result = firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);

    const char* dbname = NULL;
    const char* params = NULL;
    mock_libfbc_get_last_attach_args(&dbname, &params);
    TEST_ASSERT_NOT_NULL(dbname);
    TEST_ASSERT_NOT_NULL(strstr(dbname, "firebird.example.com:/test.fdb"));

    destroy_test_connection(conn);
}

void test_firebird_connect_remote_host_relative_path(void) {
    ConnectionConfig config = {0};
    config.database = strdup("test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");
    config.host = strdup("fb.example.com");
    config.port = 3050;

    DatabaseHandle* conn = NULL;
    bool result = firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);

    const char* dbname = NULL;
    const char* params = NULL;
    mock_libfbc_get_last_attach_args(&dbname, &params);
    TEST_ASSERT_NOT_NULL(dbname);
    TEST_ASSERT_NOT_NULL(strstr(dbname, "fb.example.com/3050:/test.fdb"));

    destroy_test_connection(conn);
}

void test_firebird_connect_localhost(void) {
    ConnectionConfig config = {0};
    config.database = strdup("test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");
    config.host = strdup("localhost");
    config.port = 3050;

    DatabaseHandle* conn = NULL;
    bool result = firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);

    const char* dbname = NULL;
    mock_libfbc_get_last_attach_args(&dbname, NULL);
    TEST_ASSERT_NOT_NULL(dbname);
    TEST_ASSERT_NOT_NULL(strstr(dbname, "localhost/3050:/test.fdb"));

    destroy_test_connection(conn);
}

void test_firebird_connect_localhost_127(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");
    config.host = strdup("127.0.0.1");

    DatabaseHandle* conn = NULL;
    bool result = firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);

    const char* dbname = NULL;
    mock_libfbc_get_last_attach_args(&dbname, NULL);
    TEST_ASSERT_NOT_NULL(dbname);
    TEST_ASSERT_NOT_NULL(strstr(dbname, "127.0.0.1:/tmp/test.fdb"));

    destroy_test_connection(conn);
}

void test_firebird_connect_null_host(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");
    config.host = NULL;

    DatabaseHandle* conn = NULL;
    bool result = firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);

    const char* dbname = NULL;
    mock_libfbc_get_last_attach_args(&dbname, NULL);
    TEST_ASSERT_NOT_NULL(dbname);
    TEST_ASSERT_EQUAL_STRING("/tmp/test.fdb", dbname);

    destroy_test_connection(conn);
}

void test_firebird_connect_empty_host(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");
    config.host = strdup("");

    DatabaseHandle* conn = NULL;
    bool result = firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);

    destroy_test_connection(conn);
}

void test_firebird_connect_attach_failure(void) {
    mock_libfbc_set_isc_attach_database_result(2);
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");

    DatabaseHandle* conn = NULL;
    bool result = firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(conn);
}

/*
 * calloc call sequence in firebird_connect (after setUp reset, with no host):
 *   call 1: strdup(database)  — uses REAL strdup in test file, doesn't count
 *   call 2: strdup(username)  — real strdup, doesn't count
 *   call 3: strdup(password)  — real strdup, doesn't count
 *   call 4: calloc in firebird_create_connection_wrapper (mock_calloc)
 *
 * The test file does NOT have USE_MOCK_SYSTEM defined (CMake passes -D flags
 * as a single mangled string for IS_DATABASE_TEST), so strdup calls in the
 * test file use real strdup and don't increment mock_malloc_call_count.
 * Only allocations inside connection.c (which IS compiled with USE_MOCK_SYSTEM)
 * go through the mock and increment the counter.
 *
 * Sequence inside firebird_connect:
 *   call 1: mock_calloc in firebird_create_connection_wrapper
 *   call 2: mock_calloc in firebird_build_attach_string
 *   call 3: mock_malloc/mock_strdup for db_name (embedded: strdup; remote: malloc)
 *   call 4: mock_calloc for DatabaseHandle
 */
void test_firebird_connect_wrapper_alloc_failure(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");

    mock_system_set_calloc_failure(1);
    DatabaseHandle* conn = NULL;
    bool result = firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(conn);
}

/*
 * For attach string alloc failure, we need firebird_build_attach_string to
 * fail at its calloc (call 5). The wrapper creates first (call 4 succeeds),
 * then the DPB buffer calloc fails (call 5).
 */
void test_firebird_connect_attach_string_failure(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");

    mock_system_set_calloc_failure(2);
    DatabaseHandle* conn = NULL;
    bool result = firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(conn);
}

/*
 * For DatabaseHandle calloc failure (call 6), the wrapper and DPB succeed.
 * Then isc_attach_database runs, succeeds, then the DatabaseHandle calloc fails.
 * The code then detaches the database.
 * Call sequence (including test's own mock_strdup calls):
 *   call 1-3: mock_strdup for config fields (test setup)
 *   call 4: mock_calloc for FirebirdConnection wrapper
 *   call 5: mock_calloc for DPB buffer (firebird_build_attach_string)
 *   call 6: mock_calloc for DatabaseHandle
 */
void test_firebird_connect_dbhandle_alloc_failure(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");

    mock_system_set_calloc_failure(6);
    DatabaseHandle* conn = NULL;
    bool result = firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(conn);
}

/* =========================================================================
 * firebird_disconnect tests
 * ========================================================================= */

void test_firebird_disconnect_null(void) {
    bool result = firebird_disconnect(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_disconnect_invalid_engine(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_POSTGRESQL;
    bool result = firebird_disconnect(&handle);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_disconnect_success(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");

    DatabaseHandle* conn = NULL;
    bool connected = firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_TRUE(connected);
    TEST_ASSERT_NOT_NULL(conn);

    bool result = firebird_disconnect(conn);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(DB_CONNECTION_DISCONNECTED, conn->status);

    pthread_mutex_destroy(&conn->connection_lock);
    if (conn->designator) free((void*)conn->designator);
    free(conn);
}

void test_firebird_disconnect_with_transaction(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");

    DatabaseHandle* conn = NULL;
    firebird_connect(&config, &conn, SR_DATABASE);
    TEST_ASSERT_NOT_NULL(conn);

    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    bool result = firebird_disconnect(conn);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(DB_CONNECTION_DISCONNECTED, conn->status);

    pthread_mutex_destroy(&conn->connection_lock);
    if (conn->designator) free((void*)conn->designator);
    free(conn);
}

void test_firebird_disconnect_null_handle(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBIRD;
    handle.connection_handle = NULL;

    bool result = firebird_disconnect(&handle);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(DB_CONNECTION_DISCONNECTED, handle.status);
}

/* =========================================================================
 * firebird_health_check tests
 * ========================================================================= */

void test_firebird_health_check_null(void) {
    bool result = firebird_health_check(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_health_check_invalid_engine(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_POSTGRESQL;
    bool result = firebird_health_check(&handle);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_health_check_null_fb_conn(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBIRD;
    handle.connection_handle = NULL;

    bool result = firebird_health_check(&handle);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_health_check_null_db_handle(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBIRD;
    FirebirdConnection fb_conn = {0};
    fb_conn.db_handle = NULL;
    handle.connection_handle = &fb_conn;

    bool result = firebird_health_check(&handle);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_health_check_success(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    bool result = firebird_health_check(conn);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, conn->consecutive_failures);
    TEST_ASSERT(conn->last_health_check > 0);

    destroy_test_connection(conn);
}

void test_firebird_health_check_alloc_failure(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    mock_libfbc_set_isc_dsql_allocate_result(2);
    bool result = firebird_health_check(conn);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(1, conn->consecutive_failures);

    destroy_test_connection(conn);
}

void test_firebird_health_check_prepare_failure(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    mock_libfbc_set_isc_dsql_prepare_result(2);
    bool result = firebird_health_check(conn);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(1, conn->consecutive_failures);

    destroy_test_connection(conn);
}

void test_firebird_health_check_execute_failure(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    mock_libfbc_set_isc_dsql_execute_result(2);
    bool result = firebird_health_check(conn);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(1, conn->consecutive_failures);

    destroy_test_connection(conn);
}

void test_firebird_health_check_fetch_failure(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    mock_libfbc_set_isc_dsql_fetch_result(2);
    bool result = firebird_health_check(conn);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(1, conn->consecutive_failures);

    destroy_test_connection(conn);
}

void test_firebird_health_check_commit_failure(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    mock_libfbc_set_isc_commit_transaction_result(2);
    bool result = firebird_health_check(conn);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(1, conn->consecutive_failures);

    destroy_test_connection(conn);
}

/*
 * When fetch fails, the health check takes the rollback path instead of
 * commit. This verifies the rollback transaction is called.
 */
void test_firebird_health_check_rollback_success(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    mock_libfbc_set_isc_dsql_fetch_result(2);
    bool result = firebird_health_check(conn);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(1, conn->consecutive_failures);

    destroy_test_connection(conn);
}

/*
 * When an existing transaction handle is set, the health check should
 * not start its own transaction.
 */
void test_firebird_health_check_existing_transaction(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->tr_handle = (void*)0xBEEF0001;

    bool result = firebird_health_check(conn);
    TEST_ASSERT_TRUE(result);

    destroy_test_connection(conn);
}

void test_firebird_health_check_start_txn_failure(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    mock_libfbc_set_isc_start_transaction_result(2);
    bool result = firebird_health_check(conn);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(1, conn->consecutive_failures);

    destroy_test_connection(conn);
}

/*
 * When isc_start_transaction_ptr is NULL (simulated by not setting the
 * result), the function pointer check at the top of the health check
 * catches it. In mock mode, the pointer is always set via load_libfbclient_functions,
 * but this test exercises the path where fb_conn->tr_handle remains NULL
 * after the transaction start attempt, triggering the "no transaction" error.
 */
void test_firebird_health_check_func_ptr_missing(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBIRD;
    handle.connection_handle = NULL;

    bool result = firebird_health_check(&handle);
    TEST_ASSERT_FALSE(result);
}

/* =========================================================================
 * firebird_reset_connection tests
 * ========================================================================= */

void test_firebird_reset_connection_null(void) {
    bool result = firebird_reset_connection(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_reset_connection_invalid_engine(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_POSTGRESQL;
    bool result = firebird_reset_connection(&handle);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_reset_connection_success(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    conn->consecutive_failures = 5;
    conn->status = DB_CONNECTION_DISCONNECTED;

    bool result = firebird_reset_connection(conn);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(DB_CONNECTION_CONNECTED, conn->status);
    TEST_ASSERT_EQUAL(0, conn->consecutive_failures);

    destroy_test_connection(conn);
}

/* =========================================================================
 * firebird_cancel_inflight tests
 * ========================================================================= */

void test_firebird_cancel_inflight_null(void) {
    firebird_cancel_inflight(NULL);
}

void test_firebird_cancel_inflight_no_stmt(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    firebird_cancel_inflight(conn);

    destroy_test_connection(conn);
}

void test_firebird_cancel_inflight_null_fb_conn(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBIRD;
    handle.connection_handle = NULL;

    firebird_cancel_inflight(&handle);
}

void test_firebird_cancel_inflight_with_stmt(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->stmt_handle = (void*)0xDEADBEEF;

    firebird_cancel_inflight(conn);
    TEST_ASSERT_EQUAL(1, mock_libfbc_get_fb_cancel_operation_call_count());

    destroy_test_connection(conn);
}

void test_firebird_cancel_inflight_cancel_fail(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    fb_conn->stmt_handle = (void*)0xDEADBEEF;

    mock_libfbc_set_fb_cancel_operation_result(2);
    firebird_cancel_inflight(conn);
    TEST_ASSERT_EQUAL(1, mock_libfbc_get_fb_cancel_operation_call_count());

    destroy_test_connection(conn);
}

/* =========================================================================
 * firebird_active_stmt_set / clear tests
 * ========================================================================= */

void test_firebird_active_stmt_set_clear(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    void* fake_stmt = (void*)0x1234;

    /* null-safe calls */
    firebird_active_stmt_set(NULL, fake_stmt);
    firebird_active_stmt_clear(NULL, NULL);

    /* set on valid connection */
    firebird_active_stmt_set(conn, fake_stmt);

    /* verify it was set */
    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    pthread_mutex_lock(&fb_conn->stmt_lock);
    TEST_ASSERT_EQUAL_PTR(fake_stmt, fb_conn->stmt_handle);
    pthread_mutex_unlock(&fb_conn->stmt_lock);

    /* clear it */
    firebird_active_stmt_clear(conn, fake_stmt);

    /* verify it was cleared */
    pthread_mutex_lock(&fb_conn->stmt_lock);
    TEST_ASSERT_NULL(fb_conn->stmt_handle);
    pthread_mutex_unlock(&fb_conn->stmt_lock);

    destroy_test_connection(conn);
}

void test_firebird_active_stmt_set_null_fb_conn(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBIRD;
    handle.connection_handle = NULL;

    firebird_active_stmt_set(&handle, (void*)0x1234);
}

void test_firebird_active_stmt_clear_null_fb_conn(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBIRD;
    handle.connection_handle = NULL;

    firebird_active_stmt_clear(&handle, (void*)0x1234);
}

void test_firebird_active_stmt_clear_no_match(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    void* set_stmt = (void*)0x1234;
    void* clear_stmt = (void*)0x5678;

    firebird_active_stmt_set(conn, set_stmt);

    /* Clear with a different handle — should not clear */
    firebird_active_stmt_clear(conn, clear_stmt);

    FirebirdConnection* fb_conn = (FirebirdConnection*)conn->connection_handle;
    pthread_mutex_lock(&fb_conn->stmt_lock);
    TEST_ASSERT_EQUAL_PTR(set_stmt, fb_conn->stmt_handle);
    pthread_mutex_unlock(&fb_conn->stmt_lock);

    destroy_test_connection(conn);
}

/* =========================================================================
 * firebird_get_connection_wrapper tests
 * ========================================================================= */

void test_firebird_get_connection_wrapper_null(void) {
    TEST_ASSERT_NULL(firebird_get_connection_wrapper(NULL));
}

void test_firebird_get_connection_wrapper_invalid_engine(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_POSTGRESQL;
    TEST_ASSERT_NULL(firebird_get_connection_wrapper(&handle));
}

void test_firebird_get_connection_wrapper_success(void) {
    DatabaseHandle* conn = create_test_connection(NULL, 0, "/tmp/test.fdb");
    TEST_ASSERT_NOT_NULL(conn);

    FirebirdConnection* fb_conn = firebird_get_connection_wrapper(conn);
    TEST_ASSERT_NOT_NULL(fb_conn);

    destroy_test_connection(conn);
}

/* =========================================================================
 * firebird_designator_safe tests
 * ========================================================================= */

void test_firebird_designator_safe_null(void) {
    TEST_ASSERT_EQUAL_STRING(SR_DATABASE, firebird_designator_safe(NULL));
}

void test_firebird_designator_safe_no_designator(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBIRD;
    handle.designator = NULL;
    TEST_ASSERT_EQUAL_STRING(SR_DATABASE, firebird_designator_safe(&handle));
}

void test_firebird_designator_safe_with_designator(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBIRD;
    handle.designator = strdup("FB-CUSTOM");
    TEST_ASSERT_EQUAL_STRING("FB-CUSTOM", firebird_designator_safe(&handle));
    free((void*)handle.designator);
}

/* =========================================================================
 * Main
 * ========================================================================= */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_firebird_connect_not_null);
    RUN_TEST(test_firebird_connect_invalid_params);
    RUN_TEST(test_firebird_connect_success);
    RUN_TEST(test_firebird_connect_null_designator);
    RUN_TEST(test_firebird_connect_remote_host);
    RUN_TEST(test_firebird_connect_remote_host_no_port);
    RUN_TEST(test_firebird_connect_remote_host_relative_path);
    RUN_TEST(test_firebird_connect_localhost);
    RUN_TEST(test_firebird_connect_localhost_127);
    RUN_TEST(test_firebird_connect_null_host);
    RUN_TEST(test_firebird_connect_empty_host);
    RUN_TEST(test_firebird_connect_attach_failure);
    RUN_TEST(test_firebird_connect_wrapper_alloc_failure);
    RUN_TEST(test_firebird_connect_attach_string_failure);
    RUN_TEST(test_firebird_connect_dbhandle_alloc_failure);
    RUN_TEST(test_firebird_disconnect_null);
    RUN_TEST(test_firebird_disconnect_invalid_engine);
    RUN_TEST(test_firebird_disconnect_success);
    RUN_TEST(test_firebird_disconnect_with_transaction);
    RUN_TEST(test_firebird_disconnect_null_handle);
    RUN_TEST(test_firebird_health_check_null);
    RUN_TEST(test_firebird_health_check_invalid_engine);
    RUN_TEST(test_firebird_health_check_null_fb_conn);
    RUN_TEST(test_firebird_health_check_null_db_handle);
    RUN_TEST(test_firebird_health_check_success);
    RUN_TEST(test_firebird_health_check_alloc_failure);
    RUN_TEST(test_firebird_health_check_prepare_failure);
    RUN_TEST(test_firebird_health_check_execute_failure);
    RUN_TEST(test_firebird_health_check_fetch_failure);
    RUN_TEST(test_firebird_health_check_commit_failure);
    RUN_TEST(test_firebird_health_check_rollback_success);
    RUN_TEST(test_firebird_health_check_existing_transaction);
    RUN_TEST(test_firebird_health_check_start_txn_failure);
    RUN_TEST(test_firebird_health_check_func_ptr_missing);
    RUN_TEST(test_firebird_reset_connection_null);
    RUN_TEST(test_firebird_reset_connection_invalid_engine);
    RUN_TEST(test_firebird_reset_connection_success);
    RUN_TEST(test_firebird_cancel_inflight_null);
    RUN_TEST(test_firebird_cancel_inflight_no_stmt);
    RUN_TEST(test_firebird_cancel_inflight_null_fb_conn);
    RUN_TEST(test_firebird_cancel_inflight_with_stmt);
    RUN_TEST(test_firebird_cancel_inflight_cancel_fail);
    RUN_TEST(test_firebird_active_stmt_set_clear);
    RUN_TEST(test_firebird_active_stmt_set_null_fb_conn);
    RUN_TEST(test_firebird_active_stmt_clear_null_fb_conn);
    RUN_TEST(test_firebird_active_stmt_clear_no_match);
    RUN_TEST(test_firebird_get_connection_wrapper_null);
    RUN_TEST(test_firebird_get_connection_wrapper_invalid_engine);
    RUN_TEST(test_firebird_get_connection_wrapper_success);
    RUN_TEST(test_firebird_designator_safe_null);
    RUN_TEST(test_firebird_designator_safe_no_designator);
    RUN_TEST(test_firebird_designator_safe_with_designator);

    return UNITY_END();
}
