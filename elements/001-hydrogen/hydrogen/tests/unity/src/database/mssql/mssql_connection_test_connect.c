/*
 * Unity Test File: MSSQL connection management
 * Tests mssql_connect() and mssql_disconnect()
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
void test_mssql_connect_null_config(void);
void test_mssql_connect_null_connection_out(void);
void test_mssql_connect_success_with_connection_string(void);
void test_mssql_connect_success_with_config_fields(void);
void test_mssql_connect_sql_alloc_handle_env_failure(void);
void test_mssql_connect_sql_driver_connect_failure(void);
void test_mssql_connect_sql_driver_connect_with_info_failure(void);
void test_mssql_connect_malloc_failure_db_handle(void);
void test_mssql_connect_prepare_cache_failure(void);
void test_mssql_connect_null_designator(void);
void test_mssql_connect_sql_set_env_attr_failure(void);
void test_mssql_connect_sql_alloc_handle_dbc_failure(void);
void test_mssql_connect_pwd_no_semicolon(void);
void test_mssql_connect_set_connect_attr_failure(void);
void test_mssql_connect_get_diag_rec_failure(void);
void test_mssql_connect_db_handle_calloc_failure(void);
void test_mssql_connect_mssql_wrapper_calloc_failure(void);
void test_mssql_connect_cache_names_calloc_failure(void);
void test_mssql_disconnect_null_connection(void);
void test_mssql_disconnect_wrong_engine(void);
void test_mssql_disconnect_success(void);
void test_mssql_disconnect_no_conn_handle(void);

static ConnectionConfig* create_test_config(const char* conn_str);
static void free_test_config(ConnectionConfig* config);
static void free_test_connection(DatabaseHandle* conn);

void setUp(void) {
    mssql_mock_libodbc_reset_all();
    mock_system_reset_all();
    load_msobdc_functions("MSSQL-TEST");
}

void tearDown(void) {
}

static ConnectionConfig* create_test_config(const char* conn_str) {
    ConnectionConfig* config = calloc(1, sizeof(ConnectionConfig));
    if (!config) return NULL;
    if (conn_str) {
        config->connection_string = strdup(conn_str);
    }
    config->host = strdup("localhost");
    config->port = 1433;
    config->database = strdup("testdb");
    config->username = strdup("testuser");
    config->password = strdup("testpwd");
    return config;
}

static void free_test_config(ConnectionConfig* config) {
    if (!config) return;
    free(config->connection_string);
    free(config->host);
    free(config->database);
    free(config->username);
    free(config->password);
    free(config);
}

static void free_test_connection(DatabaseHandle* conn) {
    if (!conn) return;
    if (conn->connection_handle) {
        MSSQLConnection* mssql_conn = (MSSQLConnection*)conn->connection_handle;
        if (mssql_conn->prepared_statements) {
            mssql_destroy_prepared_statement_cache(mssql_conn->prepared_statements);
        }
        pthread_mutex_destroy(&mssql_conn->active_stmt_lock);
        free(mssql_conn);
    }
    pthread_mutex_destroy(&conn->connection_lock);
    free(conn->designator);
    free(conn);
}

void test_mssql_connect_null_config(void) {
    DatabaseHandle* conn = NULL;
    bool result = mssql_connect(NULL, &conn, "test");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(conn);
}

void test_mssql_connect_null_connection_out(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;");
    bool result = mssql_connect(config, NULL, "test");
    TEST_ASSERT_FALSE(result);
    free_test_config(config);
}

void test_mssql_connect_success_with_connection_string(void) {
    ConnectionConfig* config = create_test_config("DRIVER={ODBC Driver 18 for SQL Server};SERVER=localhost,1433;DATABASE=testdb;UID=testuser;PWD=testpwd;");
    DatabaseHandle* conn = NULL;

    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);

    if (conn) {
        TEST_ASSERT_EQUAL(DB_ENGINE_MSSQL, conn->engine_type);
        TEST_ASSERT_EQUAL(DB_CONNECTION_CONNECTED, conn->status);
        TEST_ASSERT_NOT_NULL(conn->connection_handle);
        MSSQLConnection* mssql_conn = (MSSQLConnection*)conn->connection_handle;
        TEST_ASSERT_NOT_NULL(mssql_conn->environment);
        TEST_ASSERT_NOT_NULL(mssql_conn->connection);
        TEST_ASSERT_NOT_NULL(mssql_conn->prepared_statements);
        TEST_ASSERT_NULL(mssql_conn->active_stmt);

        free_test_connection(conn);
    }
    free_test_config(config);
}

void test_mssql_connect_success_with_config_fields(void) {
    ConnectionConfig* config = calloc(1, sizeof(ConnectionConfig));
    TEST_ASSERT_NOT_NULL(config);
    config->host = strdup("localhost");
    config->port = 1433;
    config->database = strdup("testdb");
    config->username = strdup("testuser");
    config->password = strdup("testpwd");
    config->connection_string = NULL;

    DatabaseHandle* conn = NULL;
    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);

    if (conn) {
        free_test_connection(conn);
    }
    free_test_config(config);
}

void test_mssql_connect_sql_alloc_handle_env_failure(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;");
    DatabaseHandle* conn = NULL;

    mssql_mock_libodbc_set_SQLAllocHandle_result(1);
    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(conn);

    free_test_config(config);
}

void test_mssql_connect_sql_driver_connect_failure(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;");
    DatabaseHandle* conn = NULL;

    mssql_mock_libodbc_set_SQLDriverConnect_result(2);
    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(conn);

    free_test_config(config);
}

void test_mssql_connect_sql_driver_connect_with_info_failure(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;PWD=mypassword;");
    DatabaseHandle* conn = NULL;

    mssql_mock_libodbc_set_SQLDriverConnect_result(1);
    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(conn);

    free_test_config(config);
}

void test_mssql_connect_malloc_failure_db_handle(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;");
    DatabaseHandle* conn = NULL;

    mock_system_set_malloc_failure(1);
    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(conn);
    mock_system_set_malloc_failure(0);

    free_test_config(config);
}

void test_mssql_connect_prepare_cache_failure(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;");
    DatabaseHandle* conn = NULL;

    mock_system_set_calloc_failure(3);
    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(conn);
    mock_system_set_calloc_failure(0);

    free_test_config(config);
}

void test_mssql_connect_null_designator(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;");
    DatabaseHandle* conn = NULL;

    bool result = mssql_connect(config, &conn, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);

    if (conn) {
        TEST_ASSERT_NULL(conn->designator);
        free_test_connection(conn);
    }
    free_test_config(config);
}

void test_mssql_connect_sql_set_env_attr_failure(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;");
    DatabaseHandle* conn = NULL;

    mssql_mock_libodbc_set_SQLSetEnvAttr_result(2);
    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);

    if (conn) {
        free_test_connection(conn);
    }
    free_test_config(config);
}

void test_mssql_connect_sql_alloc_handle_dbc_failure(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;");
    DatabaseHandle* conn = NULL;

    mssql_mock_libodbc_set_SQLAllocHandle_fail_at_call(2);
    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(conn);

    free_test_config(config);
}

void test_mssql_connect_pwd_no_semicolon(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;PWD=mypassword");
    DatabaseHandle* conn = NULL;

    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);

    if (conn) {
        free_test_connection(conn);
    }
    free_test_config(config);
}

void test_mssql_connect_set_connect_attr_failure(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;PWD=mypassword;");
    DatabaseHandle* conn = NULL;

    mssql_mock_libodbc_set_SQLSetConnectAttr_result(2);
    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);

    if (conn) {
        free_test_connection(conn);
    }
    free_test_config(config);
}

void test_mssql_connect_get_diag_rec_failure(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;");
    DatabaseHandle* conn = NULL;

    mssql_mock_libodbc_set_SQLDriverConnect_result(2);
    mssql_mock_libodbc_set_SQLGetDiagRec_result(2);
    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(conn);

    free_test_config(config);
}

void test_mssql_connect_db_handle_calloc_failure(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;");
    DatabaseHandle* conn = NULL;

    mock_system_set_calloc_failure(2);
    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(conn);
    mock_system_set_calloc_failure(0);

    free_test_config(config);
}

void test_mssql_connect_mssql_wrapper_calloc_failure(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;");
    DatabaseHandle* conn = NULL;

    mock_system_set_calloc_failure(2);
    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(conn);
    mock_system_set_calloc_failure(0);

    free_test_config(config);
}

void test_mssql_connect_cache_names_calloc_failure(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;");
    DatabaseHandle* conn = NULL;

    mock_system_set_calloc_failure(4);
    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(conn);
    mock_system_set_calloc_failure(0);

    free_test_config(config);
}

void test_mssql_disconnect_null_connection(void) {
    bool result = mssql_disconnect(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_mssql_disconnect_wrong_engine(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_POSTGRESQL;
    bool result = mssql_disconnect(&conn);
    TEST_ASSERT_FALSE(result);
}

void test_mssql_disconnect_success(void) {
    ConnectionConfig* config = create_test_config("DRIVER={test};SERVER=test;DATABASE=test;PWD=mypassword;");
    DatabaseHandle* conn = NULL;

    bool result = mssql_connect(config, &conn, "MSSQL-TEST");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(conn);

    if (conn) {
        result = mssql_disconnect(conn);
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_EQUAL(DB_CONNECTION_DISCONNECTED, conn->status);

        MSSQLConnection* mssql_conn = (MSSQLConnection*)conn->connection_handle;
        TEST_ASSERT_NULL(mssql_conn->connection);
        TEST_ASSERT_NULL(mssql_conn->environment);

        free_test_connection(conn);
    }
    free_test_config(config);
}

void test_mssql_disconnect_no_conn_handle(void) {
    DatabaseHandle conn = {0};
    conn.engine_type = DB_ENGINE_MSSQL;
    conn.connection_handle = NULL;
    conn.designator = strdup("MSSQL-TEST");

    bool result = mssql_disconnect(&conn);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(DB_CONNECTION_DISCONNECTED, conn.status);

    free(conn.designator);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mssql_connect_null_config);
    RUN_TEST(test_mssql_connect_null_connection_out);
    RUN_TEST(test_mssql_connect_success_with_connection_string);
    RUN_TEST(test_mssql_connect_success_with_config_fields);
    RUN_TEST(test_mssql_connect_sql_alloc_handle_env_failure);
    RUN_TEST(test_mssql_connect_sql_driver_connect_failure);
    RUN_TEST(test_mssql_connect_sql_driver_connect_with_info_failure);
    RUN_TEST(test_mssql_connect_malloc_failure_db_handle);
    RUN_TEST(test_mssql_connect_prepare_cache_failure);
    RUN_TEST(test_mssql_connect_null_designator);
    RUN_TEST(test_mssql_connect_sql_set_env_attr_failure);
    RUN_TEST(test_mssql_connect_sql_alloc_handle_dbc_failure);
    RUN_TEST(test_mssql_connect_pwd_no_semicolon);
    RUN_TEST(test_mssql_connect_set_connect_attr_failure);
    RUN_TEST(test_mssql_connect_get_diag_rec_failure);
    RUN_TEST(test_mssql_connect_db_handle_calloc_failure);
    RUN_TEST(test_mssql_connect_mssql_wrapper_calloc_failure);
    RUN_TEST(test_mssql_connect_cache_names_calloc_failure);
    RUN_TEST(test_mssql_disconnect_null_connection);
    RUN_TEST(test_mssql_disconnect_wrong_engine);
    RUN_TEST(test_mssql_disconnect_success);
    RUN_TEST(test_mssql_disconnect_no_conn_handle);

    return UNITY_END();
}
