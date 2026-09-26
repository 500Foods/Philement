/*
 * Unity Test File: MySQL query.c error and edge-path coverage
 * Covers bind-null, bind failures, prepared-path errors, and error classification.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <unity/mocks/mock_libmariadb.h>
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/database/database.h>
#include <src/database/database_params.h>
#include <src/database/mariadb/connection.h>
#include <src/database/mariadb/query.h>
#include <src/database/mariadb/types.h>

extern mariadb_stmt_init_t mariadb_stmt_init_ptr;
extern mariadb_stmt_prepare_t mariadb_stmt_prepare_ptr;
extern mariadb_stmt_execute_t mariadb_stmt_execute_ptr;
extern mariadb_stmt_bind_param_t mariadb_stmt_bind_param_ptr;
extern mariadb_stmt_error_t mariadb_stmt_error_ptr;
extern mariadb_stmt_close_t mariadb_stmt_close_ptr;

extern void* mock_mariadb_stmt_init(void* mysql);
extern int mock_mariadb_stmt_prepare(void* stmt, const char* query, unsigned long length);
extern int mock_mariadb_stmt_execute(void* stmt);
extern int mock_mariadb_stmt_bind_param(void* stmt, void* bind);
extern const char* mock_mariadb_stmt_error(void* stmt);
extern int mock_mariadb_stmt_close(void* stmt);

bool mariadb_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool mariadb_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);
bool mariadb_bind_single_parameter(void* bind, unsigned int param_index, TypedParameter* param,
                                 void** bound_values, size_t total_param_count, const char* designator);

void test_bind_invalid_parameters(void);
void test_bind_null_parameter(void);
void test_bind_unsupported_type(void);
void test_bind_invalid_datetime(void);
void test_bind_string_length_malloc_fail(void);
void test_bind_text_length_malloc_fail(void);
void test_bind_null_length_malloc_fail(void);
void test_execute_query_null_json_param(void);
void test_execute_query_invalid_datetime_param(void);
void test_execute_query_convert_params_fail(void);
void test_execute_query_stmt_init_fail(void);
void test_execute_query_stmt_prepare_fail_with_error(void);
void test_execute_query_stmt_prepare_fail_no_error(void);
void test_execute_query_bind_alloc_fail(void);
void test_execute_query_bind_param_fail(void);
void test_execute_query_stmt_execute_fail(void);
void test_execute_query_transport_error(void);
void test_execute_query_timeout_error(void);
void test_execute_query_calloc_after_success_frees_result(void);
void test_execute_prepared_calloc_no_sql_fail(void);
void test_execute_prepared_stmt_execute_unavailable(void);
void test_execute_prepared_process_result_fail(void);

static DatabaseHandle* make_conn(void) {
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);
    MariadbConnection* mariadb_conn = calloc(1, sizeof(MariadbConnection));
    TEST_ASSERT_NOT_NULL(mariadb_conn);
    mariadb_conn->connection = (void*)0x12345678;
    connection->engine_type = DB_ENGINE_MARIADB;
    connection->connection_handle = mariadb_conn;
    connection->designator = strdup("test_db");
    TEST_ASSERT_NOT_NULL(connection->designator);
    return connection;
}

static void free_conn(DatabaseHandle* connection) {
    if (!connection) return;
    free(connection->designator);
    free(connection->connection_handle);
    free(connection);
}

static void free_result(QueryResult* result) {
    if (!result) return;
    free(result->data_json);
    free(result->error_message);
    if (result->column_names) {
        for (size_t i = 0; i < result->column_count; i++) {
            free(result->column_names[i]);
        }
        free(result->column_names);
    }
    free(result);
}

void setUp(void) {
    mock_libmariadb_reset_all();
    mock_system_reset_all();
    load_libmariadb_functions(NULL);
    mariadb_stmt_init_ptr = mock_mariadb_stmt_init;
    mariadb_stmt_prepare_ptr = mock_mariadb_stmt_prepare;
    mariadb_stmt_execute_ptr = mock_mariadb_stmt_execute;
    mariadb_stmt_bind_param_ptr = mock_mariadb_stmt_bind_param;
    mariadb_stmt_error_ptr = mock_mariadb_stmt_error;
    mariadb_stmt_close_ptr = mock_mariadb_stmt_close;
}

void tearDown(void) {
    mock_libmariadb_reset_all();
    mock_system_reset_all();
}

void test_bind_invalid_parameters(void) {
    TypedParameter param = {0};
    param.name = (char*)"p";
    param.type = PARAM_TYPE_INTEGER;
    void* bound[2] = {0};
    unsigned char bind_buf[256] = {0};

    TEST_ASSERT_FALSE(mariadb_bind_single_parameter(NULL, 0, &param, bound, 1, "T"));
    TEST_ASSERT_FALSE(mariadb_bind_single_parameter(bind_buf, 0, NULL, bound, 1, "T"));
    TEST_ASSERT_FALSE(mariadb_bind_single_parameter(bind_buf, 0, &param, NULL, 1, "T"));
}

void test_bind_null_parameter(void) {
    TypedParameter param = {0};
    param.name = (char*)"p";
    param.type = PARAM_TYPE_STRING;
    param.is_null = true;
    void** bound = calloc(2, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound);
    unsigned char bind_buf[512] = {0};

    TEST_ASSERT_TRUE(mariadb_bind_single_parameter(bind_buf, 0, &param, bound, 1, "T"));
    TEST_ASSERT_NOT_NULL(bound[0]);
    TEST_ASSERT_NOT_NULL(bound[1]);
    mariadb_cleanup_bound_values(bound, 1);
}

void test_bind_unsupported_type(void) {
    TypedParameter param = {0};
    param.name = (char*)"p";
    param.type = (ParameterType)99;
    void* bound[2] = {0};
    unsigned char bind_buf[256] = {0};

    TEST_ASSERT_FALSE(mariadb_bind_single_parameter(bind_buf, 0, &param, bound, 1, "T"));
}

void test_bind_invalid_datetime(void) {
    TypedParameter param = {0};
    param.name = (char*)"p";
    param.type = PARAM_TYPE_DATETIME;
    param.value.datetime_value = (char*)"not-a-datetime";
    void* bound[2] = {0};
    unsigned char bind_buf[512] = {0};

    TEST_ASSERT_FALSE(mariadb_bind_single_parameter(bind_buf, 0, &param, bound, 1, "T"));
}

void test_bind_string_length_malloc_fail(void) {
    TypedParameter param = {0};
    param.name = (char*)"p";
    param.type = PARAM_TYPE_STRING;
    param.value.string_value = (char*)"hello";
    void* bound[2] = {0};
    unsigned char bind_buf[512] = {0};

    mock_system_set_malloc_failure(2);
    TEST_ASSERT_FALSE(mariadb_bind_single_parameter(bind_buf, 0, &param, bound, 1, "T"));
    mock_system_reset_all();
}

void test_bind_text_length_malloc_fail(void) {
    TypedParameter param = {0};
    param.name = (char*)"p";
    param.type = PARAM_TYPE_TEXT;
    param.value.text_value = (char*)"blob";
    void* bound[2] = {0};
    unsigned char bind_buf[512] = {0};

    mock_system_set_malloc_failure(2);
    TEST_ASSERT_FALSE(mariadb_bind_single_parameter(bind_buf, 0, &param, bound, 1, "T"));
    mock_system_reset_all();
}

void test_bind_null_length_malloc_fail(void) {
    TypedParameter param = {0};
    param.name = (char*)"p";
    param.type = PARAM_TYPE_STRING;
    param.is_null = true;
    void* bound[2] = {0};
    unsigned char bind_buf[512] = {0};

    mock_system_set_malloc_failure(2);
    TEST_ASSERT_FALSE(mariadb_bind_single_parameter(bind_buf, 0, &param, bound, 1, "T"));
    mock_system_reset_all();
}

void test_execute_query_null_json_param(void) {
    DatabaseHandle* connection = make_conn();
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM t WHERE c = :c");
    request.parameters_json = strdup("{\"STRING\": {\"c\": null}}");
    QueryResult* result = NULL;

    bool ok = mariadb_execute_query(connection, &request, &result);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(result);

    free_result(result);
    free(request.parameters_json);
    free(request.sql_template);
    free_conn(connection);
}

void test_execute_query_invalid_datetime_param(void) {
    DatabaseHandle* connection = make_conn();
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM t WHERE c = :c");
    request.parameters_json = strdup("{\"DATETIME\": {\"c\": \"bad-format\"}}");
    QueryResult* result = NULL;

    bool ok = mariadb_execute_query(connection, &request, &result);
    TEST_ASSERT_FALSE(ok);

    free_result(result);
    free(request.parameters_json);
    free(request.sql_template);
    free_conn(connection);
}

void test_execute_query_convert_params_fail(void) {
    DatabaseHandle* connection = make_conn();
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM t WHERE a = :missing");
    request.parameters_json = strdup("{\"INTEGER\": {\"other\": 1}}");
    QueryResult* result = NULL;

    bool ok = mariadb_execute_query(connection, &request, &result);
    TEST_ASSERT_FALSE(ok);

    free_result(result);
    free(request.parameters_json);
    free(request.sql_template);
    free_conn(connection);
}

void test_execute_query_stmt_init_fail(void) {
    DatabaseHandle* connection = make_conn();
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM t WHERE id = :id");
    request.parameters_json = strdup("{\"INTEGER\": {\"id\": 1}}");
    QueryResult* result = NULL;

    mock_libmariadb_set_mysql_stmt_init_result(NULL);
    bool ok = mariadb_execute_query(connection, &request, &result);
    TEST_ASSERT_FALSE(ok);

    free_result(result);
    free(request.parameters_json);
    free(request.sql_template);
    free_conn(connection);
}

void test_execute_query_stmt_prepare_fail_with_error(void) {
    DatabaseHandle* connection = make_conn();
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM t WHERE id = :id");
    request.parameters_json = strdup("{\"INTEGER\": {\"id\": 1}}");
    QueryResult* result = NULL;

    mock_libmariadb_set_mysql_stmt_prepare_result(1);
    mock_libmariadb_set_mysql_error_result("syntax error near X");
    bool ok = mariadb_execute_query(connection, &request, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_NOT_NULL(result->error_message);

    free_result(result);
    free(request.parameters_json);
    free(request.sql_template);
    free_conn(connection);
}

void test_execute_query_stmt_prepare_fail_no_error(void) {
    DatabaseHandle* connection = make_conn();
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM t WHERE id = :id");
    request.parameters_json = strdup("{\"INTEGER\": {\"id\": 1}}");
    QueryResult* result = NULL;

    mock_libmariadb_set_mysql_stmt_prepare_result(1);
    mock_libmariadb_set_mysql_error_result("");
    bool ok = mariadb_execute_query(connection, &request, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->error_message);

    free_result(result);
    free(request.parameters_json);
    free(request.sql_template);
    free_conn(connection);
}

void test_execute_query_bind_alloc_fail(void) {
    DatabaseHandle* connection = make_conn();
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM t WHERE id = :id");
    request.parameters_json = strdup("{\"INTEGER\": {\"id\": 1}}");
    bool saw_fail = false;

    for (int n = 1; n <= 40; n++) {
        QueryResult* result = NULL;
        mock_system_reset_all();
        mock_libmariadb_reset_all();
        load_libmariadb_functions(NULL);
        mariadb_stmt_init_ptr = mock_mariadb_stmt_init;
        mariadb_stmt_prepare_ptr = mock_mariadb_stmt_prepare;
        mariadb_stmt_execute_ptr = mock_mariadb_stmt_execute;
        mariadb_stmt_bind_param_ptr = mock_mariadb_stmt_bind_param;
        mariadb_stmt_error_ptr = mock_mariadb_stmt_error;
        mariadb_stmt_close_ptr = mock_mariadb_stmt_close;
        mock_system_set_malloc_failure(n);
        bool ok = mariadb_execute_query(connection, &request, &result);
        mock_system_reset_all();
        free_result(result);
        if (!ok) {
            saw_fail = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(saw_fail);

    free(request.parameters_json);
    free(request.sql_template);
    free_conn(connection);
}

void test_execute_query_bind_param_fail(void) {
    DatabaseHandle* connection = make_conn();
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM t WHERE id = :id");
    request.parameters_json = strdup("{\"INTEGER\": {\"id\": 1}}");
    QueryResult* result = NULL;

    mock_libmariadb_set_mysql_stmt_bind_param_result(1);
    mock_libmariadb_set_mysql_error_result("bind failed detail");
    bool ok = mariadb_execute_query(connection, &request, &result);
    TEST_ASSERT_FALSE(ok);

    free_result(result);
    free(request.parameters_json);
    free(request.sql_template);
    free_conn(connection);
}

void test_execute_query_stmt_execute_fail(void) {
    DatabaseHandle* connection = make_conn();
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM t WHERE id = :id");
    request.parameters_json = strdup("{\"INTEGER\": {\"id\": 1}}");
    QueryResult* result = NULL;

    mock_libmariadb_set_mysql_stmt_execute_result(1);
    mock_libmariadb_set_mysql_error_result("execute failed detail");
    bool ok = mariadb_execute_query(connection, &request, &result);
    TEST_ASSERT_FALSE(ok);

    free_result(result);
    free(request.parameters_json);
    free(request.sql_template);
    free_conn(connection);
}

void test_execute_query_transport_error(void) {
    DatabaseHandle* connection = make_conn();
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    QueryResult* result = NULL;

    mock_libmariadb_set_mysql_query_result(1);
    mock_libmariadb_set_mysql_error_result("MySQL server has gone away");
    bool ok = mariadb_execute_query(connection, &request, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(DB_ERR_TRANSPORT, result->error_class);

    free_result(result);
    free(request.sql_template);
    free_conn(connection);
}

void test_execute_query_timeout_error(void) {
    DatabaseHandle* connection = make_conn();
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    QueryResult* result = NULL;

    mock_libmariadb_set_mysql_query_result(1);
    mock_libmariadb_set_mysql_error_result("Lock wait timeout exceeded");
    bool ok = mariadb_execute_query(connection, &request, &result);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(DB_ERR_TIMEOUT, result->error_class);

    free_result(result);
    free(request.sql_template);
    free_conn(connection);
}

void test_execute_query_calloc_after_success_frees_result(void) {
    DatabaseHandle* connection = make_conn();
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    QueryResult* result = NULL;

    mock_libmariadb_set_mysql_query_result(0);
    mock_libmariadb_set_mysql_store_result_result((void*)0xABCDEF01);
    mock_system_set_malloc_failure(1);
    bool ok = mariadb_execute_query(connection, &request, &result);
    TEST_ASSERT_FALSE(ok);

    mock_system_reset_all();
    free_result(result);
    free(request.sql_template);
    free_conn(connection);
}

void test_execute_prepared_calloc_no_sql_fail(void) {
    DatabaseHandle* connection = make_conn();
    PreparedStatement stmt = {0};
    stmt.name = strdup("s");
    stmt.sql_template = strdup("-- comment only");
    stmt.engine_specific_handle = NULL;
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    QueryResult* result = NULL;

    mock_system_set_malloc_failure(1);
    bool ok = mariadb_execute_prepared(connection, &stmt, &request, &result);
    TEST_ASSERT_FALSE(ok);

    mock_system_reset_all();
    free(request.sql_template);
    free(stmt.name);
    free(stmt.sql_template);
    free_conn(connection);
}

void test_execute_prepared_stmt_execute_unavailable(void) {
    DatabaseHandle* connection = make_conn();
    PreparedStatement stmt = {0};
    stmt.name = strdup("s");
    stmt.sql_template = strdup("SELECT 1");
    stmt.engine_specific_handle = (void*)0x87654321;
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    QueryResult* result = NULL;

    mariadb_stmt_execute_t saved = mariadb_stmt_execute_ptr;
    mariadb_stmt_execute_ptr = NULL;
    bool ok = mariadb_execute_prepared(connection, &stmt, &request, &result);
    mariadb_stmt_execute_ptr = saved;
    TEST_ASSERT_FALSE(ok);

    free(request.sql_template);
    free(stmt.name);
    free(stmt.sql_template);
    free_conn(connection);
}

void test_execute_prepared_process_result_fail(void) {
    DatabaseHandle* connection = make_conn();
    PreparedStatement stmt = {0};
    stmt.name = strdup("s");
    stmt.sql_template = strdup("SELECT 1");
    stmt.engine_specific_handle = (void*)0x87654321;
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    bool saw_fail = false;
    const char* cols[] = {"a", "b"};

    for (int n = 1; n <= 40; n++) {
        QueryResult* result = NULL;
        mock_system_reset_all();
        mock_libmariadb_reset_all();
        load_libmariadb_functions(NULL);
        mariadb_stmt_init_ptr = mock_mariadb_stmt_init;
        mariadb_stmt_prepare_ptr = mock_mariadb_stmt_prepare;
        mariadb_stmt_execute_ptr = mock_mariadb_stmt_execute;
        mariadb_stmt_bind_param_ptr = mock_mariadb_stmt_bind_param;
        mariadb_stmt_error_ptr = mock_mariadb_stmt_error;
        mariadb_stmt_close_ptr = mock_mariadb_stmt_close;
        mock_libmariadb_set_mysql_stmt_execute_result(0);
        mock_libmariadb_set_mysql_store_result_result((void*)0xABCDEF01);
        mock_libmariadb_set_mysql_num_fields_result(2);
        mock_libmariadb_setup_fields(2, cols);
        mock_system_set_malloc_failure(n);
        bool ok = mariadb_execute_prepared(connection, &stmt, &request, &result);
        mock_system_reset_all();
        free_result(result);
        if (!ok) {
            saw_fail = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(saw_fail);

    free(request.sql_template);
    free(stmt.name);
    free(stmt.sql_template);
    free_conn(connection);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_bind_invalid_parameters);
    RUN_TEST(test_bind_null_parameter);
    RUN_TEST(test_bind_unsupported_type);
    RUN_TEST(test_bind_invalid_datetime);
    RUN_TEST(test_bind_string_length_malloc_fail);
    RUN_TEST(test_bind_text_length_malloc_fail);
    RUN_TEST(test_bind_null_length_malloc_fail);
    RUN_TEST(test_execute_query_null_json_param);
    RUN_TEST(test_execute_query_invalid_datetime_param);
    RUN_TEST(test_execute_query_convert_params_fail);
    RUN_TEST(test_execute_query_stmt_init_fail);
    RUN_TEST(test_execute_query_stmt_prepare_fail_with_error);
    RUN_TEST(test_execute_query_stmt_prepare_fail_no_error);
    RUN_TEST(test_execute_query_bind_alloc_fail);
    RUN_TEST(test_execute_query_bind_param_fail);
    RUN_TEST(test_execute_query_stmt_execute_fail);
    RUN_TEST(test_execute_query_transport_error);
    RUN_TEST(test_execute_query_timeout_error);
    RUN_TEST(test_execute_query_calloc_after_success_frees_result);
    RUN_TEST(test_execute_prepared_calloc_no_sql_fail);
    RUN_TEST(test_execute_prepared_stmt_execute_unavailable);
    RUN_TEST(test_execute_prepared_process_result_fail);

    return UNITY_END();
}
