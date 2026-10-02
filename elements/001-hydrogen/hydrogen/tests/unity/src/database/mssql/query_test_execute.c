/*
 * Unity Test File: MSSQL query execution
 * Tests mssql_execute_query() and mssql_execute_prepared()
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/mssql/query.h>
#include <src/database/mssql/types.h>
#include <src/database/mssql/connection.h>
#include <src/database/database_params.h>
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_libodbc.h>

#define SQL_SUCCESS 0
#define SQL_SUCCESS_WITH_INFO 1
#define SQL_NO_DATA 100
#define SQL_ERROR (-1)

extern SQLExecute_t mssql_SQLExecute_ptr;

void setUp(void);
void tearDown(void);
void test_mssql_execute_query_null_connection(void);
void test_mssql_execute_query_null_request(void);
void test_mssql_execute_query_null_result(void);
void test_mssql_execute_query_wrong_engine(void);
void test_mssql_execute_query_invalid_connection_handle(void);
void test_mssql_execute_query_alloc_handle_failure(void);
void test_mssql_execute_query_sql_exec_direct_success(void);
void test_mssql_execute_query_sql_exec_direct_sql_no_data(void);
void test_mssql_execute_query_with_parameters(void);
void test_mssql_execute_query_prepare_failure(void);
void test_mssql_execute_query_bind_failure(void);
void test_mssql_execute_query_execute_failure_with_diag(void);
void test_mssql_execute_query_execute_failure_without_diag(void);
void test_mssql_execute_query_parse_failure(void);
void test_mssql_execute_query_no_placeholders_direct_exec(void);
void test_mssql_execute_prepared_null_params(void);
void test_mssql_execute_prepared_wrong_engine(void);
void test_mssql_execute_prepared_invalid_conn_handle(void);
void test_mssql_execute_prepared_null_stmt_handle(void);
void test_mssql_execute_prepared_null_sql_execute(void);
void test_mssql_execute_prepared_execute_success(void);
void test_mssql_execute_prepared_execute_failure_with_diag(void);
void test_mssql_execute_prepared_execute_failure_without_diag(void);
void test_mssql_execute_prepared_calloc_failure(void);

static DatabaseHandle* create_test_connection(void);
static void free_test_connection(DatabaseHandle* conn);
static QueryRequest* create_test_request(const char* sql, const char* params);
static void free_test_request(QueryRequest* req);

void setUp(void) {
    mssql_mock_libodbc_reset_all();
    mock_system_reset_all();
    load_msobdc_functions("MSSQL-TEST");
}

void tearDown(void) {
}

static DatabaseHandle* create_test_connection(void) {
    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    if (!conn) return NULL;

    conn->engine_type = DB_ENGINE_MSSQL;
    conn->designator = strdup("DQM-MSSQL-01");

    MSSQLConnection* mssql_conn = calloc(1, sizeof(MSSQLConnection));
    if (!mssql_conn) {
        free(conn->designator);
        free(conn);
        return NULL;
    }

    mssql_conn->environment = (void*)0x100;
    mssql_conn->connection = (void*)0x200;
    mssql_conn->prepared_statements = NULL;
    mssql_conn->active_stmt = NULL;
    pthread_mutex_init(&mssql_conn->active_stmt_lock, NULL);

    conn->connection_handle = mssql_conn;
    pthread_mutex_init(&conn->connection_lock, NULL);

    return conn;
}

static void free_test_connection(DatabaseHandle* conn) {
    if (!conn) return;
    if (conn->connection_handle) {
        MSSQLConnection* mssql_conn = (MSSQLConnection*)conn->connection_handle;
        pthread_mutex_destroy(&mssql_conn->active_stmt_lock);
        free(mssql_conn);
    }
    pthread_mutex_destroy(&conn->connection_lock);
    free(conn->designator);
    free(conn);
}

static QueryRequest* create_test_request(const char* sql, const char* params) {
    QueryRequest* req = calloc(1, sizeof(QueryRequest));
    if (!req) return NULL;
    req->query_id = strdup("test-query-1");
    req->sql_template = strdup(sql);
    if (params) {
        req->parameters_json = strdup(params);
    }
    req->timeout_seconds = 30;
    req->isolation_level = DB_ISOLATION_READ_COMMITTED;
    req->use_prepared_statement = false;
    req->max_retries = 0;
    return req;
}

static void free_test_request(QueryRequest* req) {
    if (!req) return;
    free(req->query_id);
    free(req->sql_template);
    free(req->parameters_json);
    free(req);
}

void test_mssql_execute_query_null_connection(void) {
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    QueryResult* result = NULL;
    bool ret = mssql_execute_query(NULL, req, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NULL(result);
    free_test_request(req);
}

void test_mssql_execute_query_null_request(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    QueryResult* result = NULL;
    bool ret = mssql_execute_query(conn, NULL, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NULL(result);
    free_test_connection(conn);
}

void test_mssql_execute_query_null_result(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    bool ret = mssql_execute_query(conn, req, NULL);
    TEST_ASSERT_FALSE(ret);
    free_test_request(req);
    free_test_connection(conn);
}

void test_mssql_execute_query_wrong_engine(void) {
    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(conn);
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    conn->connection_handle = (void*)0x1234;
    conn->designator = strdup("DQM-PG-01");
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    QueryResult* result = NULL;
    bool ret = mssql_execute_query(conn, req, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NULL(result);
    free(req->query_id);
    free(req->sql_template);
    free(req);
    free(conn->designator);
    free(conn);
}

void test_mssql_execute_query_invalid_connection_handle(void) {
    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(conn);
    conn->engine_type = DB_ENGINE_MSSQL;
    conn->connection_handle = NULL;
    conn->designator = strdup("DQM-MSSQL-01");
    pthread_mutex_init(&conn->connection_lock, NULL);
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    QueryResult* result = NULL;
    bool ret = mssql_execute_query(conn, req, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NULL(result);
    free_test_request(req);
    pthread_mutex_destroy(&conn->connection_lock);
    free(conn->designator);
    free(conn);
}

void test_mssql_execute_query_alloc_handle_failure(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    mssql_mock_libodbc_set_SQLAllocHandle_result(1);
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    QueryResult* result = NULL;
    bool ret = mssql_execute_query(conn, req, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NULL(result);
    free_test_request(req);
    free_test_connection(conn);
}

void test_mssql_execute_query_sql_exec_direct_success(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    mssql_mock_libodbc_set_SQLAllocHandle_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLExecDirect_result(SQL_SUCCESS);
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    QueryResult* result = NULL;
    bool ret = mssql_execute_query(conn, req, &result);
    free_test_request(req);
    if (ret) {
        TEST_ASSERT_NOT_NULL(result);
        TEST_ASSERT_TRUE(result->success);
        TEST_ASSERT_NOT_NULL(result->data_json);
        free(result->data_json);
        free(result);
    }
    free_test_connection(conn);
}

void test_mssql_execute_query_sql_exec_direct_sql_no_data(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    mssql_mock_libodbc_set_SQLAllocHandle_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLExecDirect_result(SQL_NO_DATA);
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    QueryResult* result = NULL;
    bool ret = mssql_execute_query(conn, req, &result);
    free_test_request(req);
    if (ret) {
        TEST_ASSERT_NOT_NULL(result);
        TEST_ASSERT_TRUE(result->success);
        free(result->data_json);
        free(result);
    }
    free_test_connection(conn);
}

void test_mssql_execute_query_with_parameters(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    mssql_mock_libodbc_set_SQLAllocHandle_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLPrepare_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLBindParameter_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLExecute_result(SQL_SUCCESS);
    QueryRequest* req = create_test_request("SELECT :id", "{\"INTEGER\": {\"id\": 42}}");
    QueryResult* result = NULL;
    bool ret = mssql_execute_query(conn, req, &result);
    free_test_request(req);
    if (ret) {
        TEST_ASSERT_NOT_NULL(result);
        TEST_ASSERT_TRUE(result->success);
        free(result->data_json);
        free(result);
    }
    free_test_connection(conn);
}

void test_mssql_execute_query_prepare_failure(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    mssql_mock_libodbc_set_SQLAllocHandle_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLPrepare_result(-1); // SQL_ERROR
    QueryRequest* req = create_test_request("SELECT :id", "{\"INTEGER\": {\"id\": 42}}");
    QueryResult* result = NULL;
    bool ret = mssql_execute_query(conn, req, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NULL(result);
    free_test_request(req);
    free_test_connection(conn);
}

void test_mssql_execute_query_bind_failure(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    mssql_mock_libodbc_set_SQLAllocHandle_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLPrepare_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLBindParameter_result(-1); // SQL_ERROR
    QueryRequest* req = create_test_request("SELECT :id", "{\"INTEGER\": {\"id\": 42}}");
    QueryResult* result = NULL;
    bool ret = mssql_execute_query(conn, req, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NULL(result);
    free_test_request(req);
    free_test_connection(conn);
}

void test_mssql_execute_query_execute_failure_with_diag(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    mssql_mock_libodbc_set_SQLAllocHandle_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLExecDirect_result(-1); // SQL_ERROR
    mssql_mock_libodbc_set_SQLGetDiagRec_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLGetDiagRec_error("42000", 100, "Syntax error in SQL statement");
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    QueryResult* result = NULL;
    bool ret = mssql_execute_query(conn, req, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_NOT_NULL(result->error_message);
    TEST_ASSERT_EQUAL_INT(DB_ERR_OTHER, result->error_class);
    free(result->error_message);
    free(result->data_json);
    free(result);
    free_test_request(req);
    free_test_connection(conn);
}

void test_mssql_execute_query_execute_failure_without_diag(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    mssql_mock_libodbc_set_SQLAllocHandle_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLExecDirect_result(-1); // SQL_ERROR
    mssql_mock_libodbc_set_SQLGetDiagRec_result(-1); // SQL_ERROR
    mssql_mock_libodbc_set_SQLGetDiagRec_error("", 0, "");
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    QueryResult* result = NULL;
    bool ret = mssql_execute_query(conn, req, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_NOT_NULL(result->error_message);
    free(result->error_message);
    free(result->data_json);
    free(result);
    free_test_request(req);
    free_test_connection(conn);
}

void test_mssql_execute_query_parse_failure(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    mssql_mock_libodbc_set_SQLAllocHandle_result(SQL_SUCCESS);
    QueryRequest* req = create_test_request("SELECT :id", "not valid json");
    QueryResult* result = NULL;
    bool ret = mssql_execute_query(conn, req, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NULL(result);
    free_test_request(req);
    free_test_connection(conn);
}

void test_mssql_execute_query_no_placeholders_direct_exec(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    mssql_mock_libodbc_set_SQLAllocHandle_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLPrepare_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLExecDirect_result(SQL_SUCCESS);
    QueryRequest* req = create_test_request("SELECT * FROM users", "{\"name\":{\"type\":\"string\",\"value\":\"test\"}}");
    QueryResult* result = NULL;
    bool ret = mssql_execute_query(conn, req, &result);
    free_test_request(req);
    if (ret) {
        TEST_ASSERT_NOT_NULL(result);
        TEST_ASSERT_TRUE(result->success);
        free(result->data_json);
        free(result);
    }
    free_test_connection(conn);
}

void test_mssql_execute_prepared_null_params(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    QueryResult* result = NULL;
    bool ret = mssql_execute_prepared(conn, NULL, NULL, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NULL(result);
    ret = mssql_execute_prepared(NULL, NULL, NULL, &result);
    TEST_ASSERT_FALSE(ret);
    ret = mssql_execute_prepared(conn, NULL, NULL, NULL);
    TEST_ASSERT_FALSE(ret);
    free_test_connection(conn);
}

void test_mssql_execute_prepared_wrong_engine(void) {
    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(conn);
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    conn->connection_handle = (void*)0x1234;
    conn->designator = strdup("DQM-PG-01");
    PreparedStatement stmt = {0};
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    QueryResult* result = NULL;
    bool ret = mssql_execute_prepared(conn, &stmt, req, &result);
    TEST_ASSERT_FALSE(ret);
    free_test_request(req);
    free(conn->designator);
    free(conn);
}

void test_mssql_execute_prepared_invalid_conn_handle(void) {
    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(conn);
    conn->engine_type = DB_ENGINE_MSSQL;
    conn->connection_handle = NULL;
    conn->designator = strdup("DQM-MSSQL-01");
    PreparedStatement stmt = {0};
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    QueryResult* result = NULL;
    bool ret = mssql_execute_prepared(conn, &stmt, req, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NULL(result);
    free_test_request(req);
    free(conn->designator);
    free(conn);
}

void test_mssql_execute_prepared_null_stmt_handle(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    PreparedStatement stmt = {0};
    stmt.name = strdup("test_stmt");
    stmt.sql_template = strdup("SELECT 1");
    stmt.engine_specific_handle = NULL;
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    QueryResult* result = NULL;
    bool ret = mssql_execute_prepared(conn, &stmt, req, &result);
    TEST_ASSERT_TRUE(ret);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL_INT(0, result->row_count);
    TEST_ASSERT_EQUAL_INT(0, result->column_count);
    TEST_ASSERT_EQUAL_INT(0, result->affected_rows);
    free(result->data_json);
    free(result);
    free_test_request(req);
    free(stmt.name);
    free(stmt.sql_template);
    free_test_connection(conn);
}

void test_mssql_execute_prepared_null_sql_execute(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    SQLExecute_t saved_ptr = mssql_SQLExecute_ptr;
    mssql_SQLExecute_ptr = NULL;
    PreparedStatement stmt = {0};
    stmt.name = strdup("test_stmt");
    stmt.sql_template = strdup("SELECT 1");
    stmt.engine_specific_handle = (void*)0x1234;
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    QueryResult* result = NULL;
    bool ret = mssql_execute_prepared(conn, &stmt, req, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NULL(result);
    mssql_SQLExecute_ptr = saved_ptr;
    free_test_request(req);
    free(stmt.name);
    free(stmt.sql_template);
    free_test_connection(conn);
}

void test_mssql_execute_prepared_calloc_failure(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    PreparedStatement stmt = {0};
    stmt.name = strdup("test_stmt");
    stmt.sql_template = strdup("SELECT 1");
    stmt.engine_specific_handle = NULL;
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    mock_system_set_calloc_failure(1);
    QueryResult* result = NULL;
    bool ret = mssql_execute_prepared(conn, &stmt, req, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NULL(result);
    free_test_request(req);
    free(stmt.name);
    free(stmt.sql_template);
    free_test_connection(conn);
}

void test_mssql_execute_prepared_execute_success(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    mssql_mock_libodbc_set_SQLExecute_result(SQL_SUCCESS);
    PreparedStatement stmt = {0};
    stmt.name = strdup("test_stmt");
    stmt.sql_template = strdup("SELECT 1");
    stmt.engine_specific_handle = (void*)0x1234;
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    QueryResult* result = NULL;
    bool ret = mssql_execute_prepared(conn, &stmt, req, &result);
    free_test_request(req);
    if (ret) {
        TEST_ASSERT_NOT_NULL(result);
        TEST_ASSERT_TRUE(result->success);
        free(result->data_json);
        free(result);
    }
    free(stmt.name);
    free(stmt.sql_template);
    free_test_connection(conn);
}

void test_mssql_execute_prepared_execute_failure_with_diag(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    mssql_mock_libodbc_set_SQLExecute_result(-1); // SQL_ERROR
    mssql_mock_libodbc_set_SQLGetDiagRec_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLGetDiagRec_error("42000", 100, "Syntax error");
    PreparedStatement stmt = {0};
    stmt.name = strdup("test_stmt");
    stmt.sql_template = strdup("SELECT 1");
    stmt.engine_specific_handle = (void*)0x1234;
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    QueryResult* result = NULL;
    bool ret = mssql_execute_prepared(conn, &stmt, req, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NULL(result);
    free_test_request(req);
    free(stmt.name);
    free(stmt.sql_template);
    free_test_connection(conn);
}

void test_mssql_execute_prepared_execute_failure_without_diag(void) {
    DatabaseHandle* conn = create_test_connection();
    TEST_ASSERT_NOT_NULL(conn);
    mssql_mock_libodbc_set_SQLExecute_result(-1); // SQL_ERROR
    mssql_mock_libodbc_set_SQLGetDiagRec_result(-1); // SQL_ERROR
    PreparedStatement stmt = {0};
    stmt.name = strdup("test_stmt");
    stmt.sql_template = strdup("SELECT 1");
    stmt.engine_specific_handle = (void*)0x1234;
    QueryRequest* req = create_test_request("SELECT 1", NULL);
    QueryResult* result = NULL;
    bool ret = mssql_execute_prepared(conn, &stmt, req, &result);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_NULL(result);
    free_test_request(req);
    free(stmt.name);
    free(stmt.sql_template);
    free_test_connection(conn);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mssql_execute_query_null_connection);
    RUN_TEST(test_mssql_execute_query_null_request);
    RUN_TEST(test_mssql_execute_query_null_result);
    RUN_TEST(test_mssql_execute_query_wrong_engine);
    RUN_TEST(test_mssql_execute_query_invalid_connection_handle);
    RUN_TEST(test_mssql_execute_query_alloc_handle_failure);
    RUN_TEST(test_mssql_execute_query_sql_exec_direct_success);
    RUN_TEST(test_mssql_execute_query_sql_exec_direct_sql_no_data);
    RUN_TEST(test_mssql_execute_query_with_parameters);
    RUN_TEST(test_mssql_execute_query_prepare_failure);
    RUN_TEST(test_mssql_execute_query_bind_failure);
    RUN_TEST(test_mssql_execute_query_execute_failure_with_diag);
    RUN_TEST(test_mssql_execute_query_execute_failure_without_diag);
    RUN_TEST(test_mssql_execute_query_parse_failure);
    RUN_TEST(test_mssql_execute_query_no_placeholders_direct_exec);

    RUN_TEST(test_mssql_execute_prepared_null_params);
    RUN_TEST(test_mssql_execute_prepared_wrong_engine);
    RUN_TEST(test_mssql_execute_prepared_invalid_conn_handle);
    RUN_TEST(test_mssql_execute_prepared_null_stmt_handle);
    RUN_TEST(test_mssql_execute_prepared_null_sql_execute);
    RUN_TEST(test_mssql_execute_prepared_calloc_failure);
    RUN_TEST(test_mssql_execute_prepared_execute_success);
    RUN_TEST(test_mssql_execute_prepared_execute_failure_with_diag);
    RUN_TEST(test_mssql_execute_prepared_execute_failure_without_diag);

    return UNITY_END();
}
