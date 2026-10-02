/*
 * Unity Test File: MSSQL complete_standalone_statement
 * Tests mssql_complete_standalone_statement()
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/mssql/query.h>
#include <src/database/mssql/types.h>
#include <src/database/mssql/connection.h>
#include <unity/mocks/mock_libodbc.h>

void setUp(void);
void tearDown(void);
void test_mssql_complete_standalone_statement_null_connection(void);
void test_mssql_complete_standalone_statement_in_transaction(void);
void test_mssql_complete_standalone_statement_null_endtran_ptr(void);
void test_mssql_complete_standalone_statement_null_mssql_conn(void);
void test_mssql_complete_standalone_statement_null_connection_handle(void);
void test_mssql_complete_standalone_statement_commit_success(void);
void test_mssql_complete_standalone_statement_rollback_success(void);
void test_mssql_complete_standalone_statement_commit_failure(void);
void test_mssql_complete_standalone_statement_null_designator(void);

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
    handle->current_transaction = NULL;
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

void test_mssql_complete_standalone_statement_null_connection(void) {
    mssql_complete_standalone_statement(NULL, true);
    TEST_PASS();
}

void test_mssql_complete_standalone_statement_in_transaction(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    handle->current_transaction = (void*)0x1234;
    mssql_complete_standalone_statement(handle, true);
    TEST_PASS();

    handle->current_transaction = NULL;
    free_test_database_handle(handle);
}

void test_mssql_complete_standalone_statement_null_endtran_ptr(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    mssql_SQLEndTran_ptr = NULL;
    mssql_complete_standalone_statement(handle, true);
    TEST_PASS();

    free_test_database_handle(handle);
}

void test_mssql_complete_standalone_statement_null_mssql_conn(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_MSSQL;
    conn.connection_handle = NULL;
    conn.designator = strdup("MSSQL-TEST");
    mssql_SQLEndTran_ptr = NULL;

    mssql_complete_standalone_statement(&conn, true);
    TEST_PASS();

    free(conn.designator);
}

void test_mssql_complete_standalone_statement_null_connection_handle(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_MSSQL;
    MSSQLConnection mssql_conn = {0};
    mssql_conn.connection = NULL;
    conn.connection_handle = &mssql_conn;
    conn.designator = strdup("MSSQL-TEST");

    mssql_complete_standalone_statement(&conn, true);
    TEST_PASS();

    free(conn.designator);
}

void test_mssql_complete_standalone_statement_commit_success(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    mssql_mock_libodbc_set_SQLEndTran_result(0);
    mssql_complete_standalone_statement(handle, true);
    TEST_PASS();

    free_test_database_handle(handle);
}

void test_mssql_complete_standalone_statement_rollback_success(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    mssql_mock_libodbc_set_SQLEndTran_result(0);
    mssql_complete_standalone_statement(handle, false);
    TEST_PASS();

    free_test_database_handle(handle);
}

void test_mssql_complete_standalone_statement_commit_failure(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    mssql_mock_libodbc_set_SQLEndTran_result(1);
    mssql_complete_standalone_statement(handle, true);
    TEST_PASS();

    free_test_database_handle(handle);
}

void test_mssql_complete_standalone_statement_null_designator(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_MSSQL;
    conn.connection_handle = NULL;
    conn.designator = NULL;
    mssql_SQLEndTran_ptr = NULL;

    mssql_complete_standalone_statement(&conn, true);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mssql_complete_standalone_statement_null_connection);
    RUN_TEST(test_mssql_complete_standalone_statement_in_transaction);
    RUN_TEST(test_mssql_complete_standalone_statement_null_endtran_ptr);
    RUN_TEST(test_mssql_complete_standalone_statement_null_mssql_conn);
    RUN_TEST(test_mssql_complete_standalone_statement_null_connection_handle);
    RUN_TEST(test_mssql_complete_standalone_statement_commit_success);
    RUN_TEST(test_mssql_complete_standalone_statement_rollback_success);
    RUN_TEST(test_mssql_complete_standalone_statement_commit_failure);
    RUN_TEST(test_mssql_complete_standalone_statement_null_designator);

    return UNITY_END();
}
