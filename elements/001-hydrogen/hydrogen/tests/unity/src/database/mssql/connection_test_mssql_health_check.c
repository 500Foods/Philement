/*
 * Unity Test File: MSSQL health check and reset connection
 * Tests mssql_health_check() and mssql_reset_connection()
 */

#include <src/hydrogen.h>
#include <unity.h>

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_libodbc.h>

#include <src/database/database.h>
#include <src/database/mssql/connection.h>
#include <src/database/mssql/types.h>

void setUp(void);
void tearDown(void);
void test_mssql_health_check_null_connection(void);
void test_mssql_health_check_wrong_engine(void);
void test_mssql_health_check_null_conn_handle(void);
void test_mssql_health_check_null_mssql_conn_connection(void);
void test_mssql_health_check_success(void);
void test_mssql_health_check_success_with_info(void);
void test_mssql_health_check_alloc_handle_failure(void);
void test_mssql_health_check_exec_direct_failure(void);
void test_mssql_reset_connection_null(void);
void test_mssql_reset_connection_wrong_engine(void);
void test_mssql_reset_connection_success(void);
void test_mssql_reset_connection_null_designator(void);

static MSSQLConnection* create_test_mssql_connection(void);
static void free_test_mssql_connection(MSSQLConnection* conn);
static DatabaseHandle* create_test_database_handle(void);
static void free_test_database_handle(DatabaseHandle* handle);

void setUp(void) {
    mssql_mock_libodbc_reset_all();
    mock_system_reset_all();
    load_msobdc_functions("test");
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
    handle->consecutive_failures = 0;
    handle->last_health_check = 0;
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

void test_mssql_health_check_null_connection(void) {
    bool result = mssql_health_check(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_mssql_health_check_wrong_engine(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_POSTGRESQL;
    bool result = mssql_health_check(&conn);
    TEST_ASSERT_FALSE(result);
}

void test_mssql_health_check_null_conn_handle(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_MSSQL;
    conn.connection_handle = NULL;
    bool result = mssql_health_check(&conn);
    TEST_ASSERT_FALSE(result);
}

void test_mssql_health_check_null_mssql_conn_connection(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_MSSQL;
    MSSQLConnection mssql_conn = {0};
    mssql_conn.connection = NULL;
    conn.connection_handle = &mssql_conn;
    bool result = mssql_health_check(&conn);
    TEST_ASSERT_FALSE(result);
}

void test_mssql_health_check_success(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    mssql_mock_libodbc_set_SQLAllocHandle_result(0);
    mssql_mock_libodbc_set_SQLExecDirect_result(0);

    bool result = mssql_health_check(handle);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, handle->consecutive_failures);
    TEST_ASSERT_NOT_EQUAL(0, handle->last_health_check);

    free_test_database_handle(handle);
}

void test_mssql_health_check_success_with_info(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    mssql_mock_libodbc_set_SQLAllocHandle_result(0);
    mssql_mock_libodbc_set_SQLExecDirect_result(1);

    bool result = mssql_health_check(handle);
    TEST_ASSERT_TRUE(result);

    free_test_database_handle(handle);
}

void test_mssql_health_check_alloc_handle_failure(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    mssql_mock_libodbc_set_SQLAllocHandle_result(2);

    bool result = mssql_health_check(handle);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(1, handle->consecutive_failures);

    free_test_database_handle(handle);
}

void test_mssql_health_check_exec_direct_failure(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    mssql_mock_libodbc_set_SQLAllocHandle_result(0);
    mssql_mock_libodbc_set_SQLExecDirect_result(2);

    bool result = mssql_health_check(handle);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(1, handle->consecutive_failures);

    free_test_database_handle(handle);
}

void test_mssql_reset_connection_null(void) {
    bool result = mssql_reset_connection(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_mssql_reset_connection_wrong_engine(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_POSTGRESQL;
    bool result = mssql_reset_connection(&conn);
    TEST_ASSERT_FALSE(result);
}

void test_mssql_reset_connection_success(void) {
    DatabaseHandle* handle = create_test_database_handle();
    TEST_ASSERT_NOT_NULL(handle);

    handle->status = DB_CONNECTION_DISCONNECTED;
    handle->consecutive_failures = 5;
    handle->connected_since = 0;

    bool result = mssql_reset_connection(handle);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(DB_CONNECTION_CONNECTED, handle->status);
    TEST_ASSERT_EQUAL(0, handle->consecutive_failures);
    TEST_ASSERT_NOT_EQUAL(0, handle->connected_since);

    free_test_database_handle(handle);
}

void test_mssql_reset_connection_null_designator(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_MSSQL;
    conn.consecutive_failures = 3;

    bool result = mssql_reset_connection(&conn);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, conn.consecutive_failures);
    TEST_ASSERT_EQUAL(DB_CONNECTION_CONNECTED, conn.status);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mssql_health_check_null_connection);
    RUN_TEST(test_mssql_health_check_wrong_engine);
    RUN_TEST(test_mssql_health_check_null_conn_handle);
    RUN_TEST(test_mssql_health_check_null_mssql_conn_connection);
    RUN_TEST(test_mssql_health_check_success);
    RUN_TEST(test_mssql_health_check_success_with_info);
    RUN_TEST(test_mssql_health_check_alloc_handle_failure);
    RUN_TEST(test_mssql_health_check_exec_direct_failure);
    RUN_TEST(test_mssql_reset_connection_null);
    RUN_TEST(test_mssql_reset_connection_wrong_engine);
    RUN_TEST(test_mssql_reset_connection_success);
    RUN_TEST(test_mssql_reset_connection_null_designator);

    return UNITY_END();
}
