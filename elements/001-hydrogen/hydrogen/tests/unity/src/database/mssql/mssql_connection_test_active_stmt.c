/*
 * Unity Test File: MSSQL active statement tracking and cancel inflight
 * Tests mssql_cancel_inflight(), mssql_active_stmt_set(), mssql_active_stmt_clear()
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <unity/mocks/mock_libodbc.h>

#include <src/database/database.h>
#include <src/database/mssql/connection.h>
#include <src/database/mssql/types.h>

void setUp(void);
void tearDown(void);
void test_mssql_cancel_inflight_null_connection(void);
void test_mssql_cancel_inflight_wrong_engine(void);
void test_mssql_cancel_inflight_null_cancel_fn(void);
void test_mssql_cancel_inflight_no_conn_handle(void);
void test_mssql_cancel_inflight_no_active_stmt(void);
void test_mssql_cancel_inflight_with_active_stmt(void);
void test_mssql_cancel_inflight_sql_cancel_failure(void);
void test_mssql_cancel_inflight_null_designator(void);
void test_mssql_active_stmt_set_null_connection(void);
void test_mssql_active_stmt_set_wrong_engine(void);
void test_mssql_active_stmt_set_null_stmt_handle(void);
void test_mssql_active_stmt_set_success(void);
void test_mssql_active_stmt_set_null_conn_handle(void);
void test_mssql_active_stmt_clear_null_connection(void);
void test_mssql_active_stmt_clear_wrong_engine(void);
void test_mssql_active_stmt_clear_null_conn_handle(void);
void test_mssql_active_stmt_clear_matching_handle(void);
void test_mssql_active_stmt_clear_non_matching_handle(void);
void test_mssql_active_stmt_clear_null_active_stmt(void);

static MSSQLConnection* create_test_mssql_connection(void);
static void free_test_mssql_connection(MSSQLConnection* conn);
static DatabaseHandle* create_test_database_handle(void);
static void free_test_database_handle(DatabaseHandle* handle);

void setUp(void) {
    mssql_mock_libodbc_reset_all();
    load_msobdc_functions("MSSQL-TEST");
}

void tearDown(void) {
}

static MSSQLConnection* create_test_mssql_connection(void) {
    MSSQLConnection* conn = calloc(1, sizeof(MSSQLConnection));
    if (!conn) return NULL;
    conn->environment = (void*)0x1000;
    conn->connection = (void*)0x2000;
    conn->prepared_statements = NULL;
    conn->active_stmt = NULL;
    pthread_mutex_init(&conn->active_stmt_lock, NULL);
    return conn;
}

static void free_test_mssql_connection(MSSQLConnection* conn) {
    if (!conn) return;
    pthread_mutex_destroy(&conn->active_stmt_lock);
    free(conn);
}

static DatabaseHandle* create_test_database_handle(void) {
    DatabaseHandle* handle = calloc(1, sizeof(DatabaseHandle));
    if (!handle) return NULL;
    handle->engine_type = DB_ENGINE_MSSQL;
    handle->connection_handle = create_test_mssql_connection();
    handle->status = DB_CONNECTION_CONNECTED;
    handle->designator = strdup("MSSQL-TEST");
    pthread_mutex_init(&handle->connection_lock, NULL);
    return handle;
}

static void free_test_database_handle(DatabaseHandle* handle) {
    if (!handle) return;
    if (handle->connection_handle) {
        free_test_mssql_connection((MSSQLConnection*)handle->connection_handle);
    }
    pthread_mutex_destroy(&handle->connection_lock);
    free(handle->designator);
    free(handle);
}

void test_mssql_cancel_inflight_null_connection(void) {
    mssql_cancel_inflight(NULL);
    TEST_PASS();
}

void test_mssql_cancel_inflight_wrong_engine(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_POSTGRESQL;
    mssql_cancel_inflight(&conn);
    TEST_PASS();
}

void test_mssql_cancel_inflight_null_cancel_fn(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    mssql_SQLCancel_ptr = NULL;

    mssql_cancel_inflight(handle);
    TEST_PASS();

    free_test_database_handle(handle);
}

void test_mssql_cancel_inflight_no_conn_handle(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_MSSQL;
    conn.connection_handle = NULL;
    conn.designator = strdup("MSSQL-TEST");

    mssql_cancel_inflight(&conn);
    TEST_PASS();

    free(conn.designator);
}

void test_mssql_cancel_inflight_no_active_stmt(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    mssql_cancel_inflight(handle);
    TEST_PASS();

    free_test_database_handle(handle);
}

void test_mssql_cancel_inflight_with_active_stmt(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    MSSQLConnection* mssql_conn = (MSSQLConnection*)handle->connection_handle;
    void* fake_stmt = (void*)0xDEADBEEF;
    mssql_conn->active_stmt = fake_stmt;

    mssql_cancel_inflight(handle);
    TEST_PASS();

    free_test_database_handle(handle);
}

void test_mssql_cancel_inflight_sql_cancel_failure(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    MSSQLConnection* mssql_conn = (MSSQLConnection*)handle->connection_handle;
    void* fake_stmt = (void*)0xDEADBEEF;
    mssql_conn->active_stmt = fake_stmt;

    mssql_mock_libodbc_set_SQLCancel_result(2);

    mssql_cancel_inflight(handle);
    TEST_PASS();

    free_test_database_handle(handle);
}

void test_mssql_cancel_inflight_null_designator(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_MSSQL;
    MSSQLConnection mssql_conn = {0};
    mssql_conn.connection = (void*)0x2000;
    mssql_conn.active_stmt = (void*)0xDEADBEEF;
    pthread_mutex_init(&mssql_conn.active_stmt_lock, NULL);
    conn.connection_handle = &mssql_conn;
    conn.designator = NULL;

    mssql_cancel_inflight(&conn);
    TEST_PASS();

    pthread_mutex_destroy(&mssql_conn.active_stmt_lock);
}

void test_mssql_active_stmt_set_null_connection(void) {
    mssql_active_stmt_set(NULL, (void*)0x1234);
    TEST_PASS();
}

void test_mssql_active_stmt_set_wrong_engine(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_POSTGRESQL;
    mssql_active_stmt_set(&conn, (void*)0x1234);
    TEST_PASS();
}

void test_mssql_active_stmt_set_null_stmt_handle(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    mssql_active_stmt_set(handle, NULL);
    TEST_ASSERT_NULL(((MSSQLConnection*)handle->connection_handle)->active_stmt);

    free_test_database_handle(handle);
}

void test_mssql_active_stmt_set_success(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    void* fake_stmt = (void*)0xABCDEF;
    mssql_active_stmt_set(handle, fake_stmt);
    TEST_ASSERT_EQUAL_PTR(fake_stmt, ((MSSQLConnection*)handle->connection_handle)->active_stmt);

    free_test_database_handle(handle);
}

void test_mssql_active_stmt_set_null_conn_handle(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_MSSQL;
    conn.connection_handle = NULL;
    mssql_active_stmt_set(&conn, (void*)0x1234);
    TEST_PASS();
}

void test_mssql_active_stmt_clear_null_connection(void) {
    mssql_active_stmt_clear(NULL, (void*)0x1234);
    TEST_PASS();
}

void test_mssql_active_stmt_clear_wrong_engine(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_POSTGRESQL;
    mssql_active_stmt_clear(&conn, (void*)0x1234);
    TEST_PASS();
}

void test_mssql_active_stmt_clear_null_conn_handle(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_MSSQL;
    conn.connection_handle = NULL;
    mssql_active_stmt_clear(&conn, (void*)0x1234);
    TEST_PASS();
}

void test_mssql_active_stmt_clear_matching_handle(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    MSSQLConnection* mssql_conn = (MSSQLConnection*)handle->connection_handle;
    void* fake_stmt = (void*)0xABCD1234;
    mssql_conn->active_stmt = fake_stmt;

    mssql_active_stmt_clear(handle, fake_stmt);
    TEST_ASSERT_NULL(mssql_conn->active_stmt);

    free_test_database_handle(handle);
}

void test_mssql_active_stmt_clear_non_matching_handle(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    MSSQLConnection* mssql_conn = (MSSQLConnection*)handle->connection_handle;
    void* fake_stmt = (void*)0xABCD1234;
    mssql_conn->active_stmt = fake_stmt;

    void* different_stmt = (void*)0x99999999;
    mssql_active_stmt_clear(handle, different_stmt);
    TEST_ASSERT_EQUAL_PTR(fake_stmt, mssql_conn->active_stmt);

    free_test_database_handle(handle);
}

void test_mssql_active_stmt_clear_null_active_stmt(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    MSSQLConnection* mssql_conn = (MSSQLConnection*)handle->connection_handle;
    mssql_conn->active_stmt = NULL;

    mssql_active_stmt_clear(handle, (void*)0x1234);
    TEST_ASSERT_NULL(mssql_conn->active_stmt);

    free_test_database_handle(handle);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mssql_cancel_inflight_null_connection);
    RUN_TEST(test_mssql_cancel_inflight_wrong_engine);
    RUN_TEST(test_mssql_cancel_inflight_null_cancel_fn);
    RUN_TEST(test_mssql_cancel_inflight_no_conn_handle);
    RUN_TEST(test_mssql_cancel_inflight_no_active_stmt);
    RUN_TEST(test_mssql_cancel_inflight_with_active_stmt);
    RUN_TEST(test_mssql_cancel_inflight_sql_cancel_failure);
    RUN_TEST(test_mssql_cancel_inflight_null_designator);
    RUN_TEST(test_mssql_active_stmt_set_null_connection);
    RUN_TEST(test_mssql_active_stmt_set_wrong_engine);
    RUN_TEST(test_mssql_active_stmt_set_null_stmt_handle);
    RUN_TEST(test_mssql_active_stmt_set_success);
    RUN_TEST(test_mssql_active_stmt_set_null_conn_handle);
    RUN_TEST(test_mssql_active_stmt_clear_null_connection);
    RUN_TEST(test_mssql_active_stmt_clear_wrong_engine);
    RUN_TEST(test_mssql_active_stmt_clear_null_conn_handle);
    RUN_TEST(test_mssql_active_stmt_clear_matching_handle);
    RUN_TEST(test_mssql_active_stmt_clear_non_matching_handle);
    RUN_TEST(test_mssql_active_stmt_clear_null_active_stmt);

    return UNITY_END();
}
