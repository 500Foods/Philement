/*
 * Unity Test File: transaction_query_test_database_queue_execute_query_with_blocks
 * Unit tests for the marked runtime QueryRef path
 * (src/database/dbqueue/transaction_query.c).
 *
 * The transaction-block statements run through the shared executor's real
 * dispatchers against stub engines registered for DB_ENGINE_AI (no DDL
 * segmentation) and DB_ENGINE_FIREBIRD (commit-and-reopen on DDL). Bare
 * statements outside blocks go through the mocked database_engine_execute
 * provided by the Unity database-engine mock.
 */

#include <stdlib.h>
#include <string.h>

#include <src/hydrogen.h>
#include <unity.h>
#include <unity/mocks/mock_database_engine.h>

#include <src/database/database.h>
#include <src/database/dbqueue/transaction_query.h>

// ----- stub engine (transaction-block statements) --------------------------

static int stub_begin_calls;
static int stub_commit_calls;
static int stub_rollback_calls;
static int stub_execute_calls;
static int stub_fail_execute_call = 0;   // 1-based call number that fails
static char stub_last_parameters[256];
static char stub_all_parameters[8][256];

static void stub_reset(void) {
    stub_begin_calls = 0;
    stub_commit_calls = 0;
    stub_rollback_calls = 0;
    stub_execute_calls = 0;
    stub_fail_execute_call = 0;
    stub_last_parameters[0] = '\0';
    memset(stub_all_parameters, 0, sizeof(stub_all_parameters));
}

static bool stub_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    (void)connection;
    (void)level;
    Transaction* txn = calloc(1, sizeof(Transaction));
    if (!txn) {
        return false;
    }
    txn->transaction_id = strdup("stub-txn");
    *transaction = txn;
    stub_begin_calls++;
    return true;
}

static bool stub_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    // Ownership stays with database_engine_cleanup_transaction.
    (void)connection;
    (void)transaction;
    stub_commit_calls++;
    return true;
}

static bool stub_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    (void)connection;
    (void)transaction;
    stub_rollback_calls++;
    return true;
}

static bool stub_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) { // cppcheck-suppress constParameterCallback
    (void)connection;
    stub_execute_calls++;
    if (request) {
        if (request->parameters_json) {
            snprintf(stub_last_parameters, sizeof(stub_last_parameters), "%s", request->parameters_json);
            if (stub_execute_calls <= 8) {
                snprintf(stub_all_parameters[stub_execute_calls - 1], 256, "%s", request->parameters_json);
            }
        }
    }

    QueryResult* res = calloc(1, sizeof(QueryResult));
    if (!res) {
        return false;
    }
    res->success = (stub_fail_execute_call == 0) || (stub_execute_calls != stub_fail_execute_call);
    res->affected_rows = 3;
    res->row_count = 1;
    if (!res->success) {
        res->error_message = strdup("stub statement failure");
        res->error_class = DB_ERR_OTHER;
    }
    *result = res;
    return true;
}

static DatabaseEngineInterface ai_engine;
static bool engine_registered = false;

static void register_stub_engine(void) {
    if (engine_registered) {
        return;
    }
    database_engine_init();
    memset(&ai_engine, 0, sizeof(ai_engine));
    ai_engine.engine_type = DB_ENGINE_AI;
    ai_engine.name = (char*)"txn_query_ai";
    ai_engine.execute_query = stub_execute_query;
    ai_engine.begin_transaction = stub_begin_transaction;
    ai_engine.commit_transaction = stub_commit_transaction;
    ai_engine.rollback_transaction = stub_rollback_transaction;
    (void)database_engine_register(&ai_engine);
    engine_registered = true;
}

static DatabaseHandle* make_connection(void) {
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    if (connection) {
        connection->engine_type = DB_ENGINE_AI;
        connection->designator = strdup("TXN-QUERY-TEST");
    }
    return connection;
}

static QueryRequest make_request(const char* sql, const char* parameters_json) {
    QueryRequest request;
    memset(&request, 0, sizeof(request));
    request.query_id = (char*)"test-query";
    request.sql_template = (char*)sql;
    request.parameters_json = (char*)parameters_json;
    request.timeout_seconds = 30;
    request.isolation_level = DB_ISOLATION_READ_COMMITTED;
    request.use_prepared_statement = false;
    return request;
}

// ----- tests ---------------------------------------------------------------

void test_marked_query_parse_error(void);
void test_marked_query_single_block(void);
void test_marked_query_block_failure(void);
void test_marked_query_bare_statements(void);
void test_marked_query_parameters_filtered_per_statement(void);
void test_marked_query_invalid_request(void);

void setUp(void) {
    register_stub_engine();
    stub_reset();
    mock_database_engine_reset_all();
}

void tearDown(void) {
}

void test_marked_query_parse_error(void) {
    DatabaseHandle* connection = make_connection();
    QueryRequest request = make_request("SELECT 1;\n-- STATEMENT\nSELECT 2;", NULL);
    QueryResult* result = NULL;

    bool success = database_queue_execute_query_with_blocks(connection, &request, &result, "T");
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_NOT_NULL(result->error_message);
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "STATEMENT outside a transaction block"));
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);

    database_engine_cleanup_result(result);
    free(connection->designator);
    free(connection);
}

void test_marked_query_single_block(void) {
    DatabaseHandle* connection = make_connection();
    QueryRequest request = make_request(
        "-- TRANSACTION_START\n"
        "INSERT INTO t VALUES (1);\n"
        "-- STATEMENT\n"
        "UPDATE t SET a = 2;\n"
        "-- TRANSACTION_END", NULL);
    QueryResult* result = NULL;

    bool success = database_queue_execute_query_with_blocks(connection, &request, &result, "T");
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL_size_t(2, result->row_count);
    TEST_ASSERT_EQUAL_INT(6, result->affected_rows);

    json_error_t parse_error;
    json_t* entries = json_loads(result->data_json, 0, &parse_error);
    TEST_ASSERT_NOT_NULL(entries);
    TEST_ASSERT_EQUAL_size_t(2, json_array_size(entries));

    json_t* first = json_array_get(entries, 0);
    TEST_ASSERT_EQUAL_INT(1, json_integer_value(json_object_get(first, "statement")));
    TEST_ASSERT_EQUAL_INT(1, json_integer_value(json_object_get(first, "block")));
    TEST_ASSERT_TRUE(json_is_true(json_object_get(first, "success")));
    TEST_ASSERT_EQUAL_INT(3, json_integer_value(json_object_get(first, "affected")));

    json_t* second = json_array_get(entries, 1);
    TEST_ASSERT_EQUAL_INT(2, json_integer_value(json_object_get(second, "statement")));

    json_decref(entries);
    database_engine_cleanup_result(result);
    TEST_ASSERT_EQUAL_INT(1, stub_begin_calls);
    TEST_ASSERT_EQUAL_INT(1, stub_commit_calls);
    TEST_ASSERT_EQUAL_INT(0, stub_rollback_calls);
    free(connection->designator);
    free(connection);
}

void test_marked_query_block_failure(void) {
    DatabaseHandle* connection = make_connection();
    QueryRequest request = make_request(
        "-- TRANSACTION_START\n"
        "INSERT INTO t VALUES (1);\n"
        "-- STATEMENT\n"
        "INSERT INTO t VALUES (2);\n"
        "-- TRANSACTION_END", NULL);
    QueryResult* result = NULL;
    stub_fail_execute_call = 2;

    bool success = database_queue_execute_query_with_blocks(connection, &request, &result, "T");
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "transaction block 1 failed at statement 2"));

    json_error_t parse_error;
    json_t* entries = json_loads(result->data_json, 0, &parse_error);
    TEST_ASSERT_NOT_NULL(entries);
    // Both statements are reported: the second one carries its error.
    TEST_ASSERT_EQUAL_size_t(2, json_array_size(entries));
    json_t* second = json_array_get(entries, 1);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(second, "success")));
    TEST_ASSERT_EQUAL_STRING("stub statement failure", json_string_value(json_object_get(second, "error")));
    json_decref(entries);

    database_engine_cleanup_result(result);
    TEST_ASSERT_EQUAL_INT(1, stub_rollback_calls);
    free(connection->designator);
    free(connection);
}

void test_marked_query_bare_statements(void) {
    DatabaseHandle* connection = make_connection();
    // Bare DDL before the block, bare SELECT after; the bare statements run
    // through the mocked database_engine_execute.
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_execute_json_data("[{\"v\":9}]");
    mock_database_engine_set_affected_rows(7);

    QueryRequest request = make_request(
        "CREATE TABLE t (a INT);\n"
        "-- TRANSACTION_START\n"
        "INSERT INTO t VALUES (1);\n"
        "-- TRANSACTION_END\n"
        "SELECT * FROM t;", NULL);
    QueryResult* result = NULL;

    bool success = database_queue_execute_query_with_blocks(connection, &request, &result, "T");
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(result);

    json_error_t parse_error;
    json_t* entries = json_loads(result->data_json, 0, &parse_error);
    TEST_ASSERT_NOT_NULL(entries);
    TEST_ASSERT_EQUAL_size_t(3, json_array_size(entries));

    json_t* first = json_array_get(entries, 0);
    TEST_ASSERT_EQUAL_INT(1, json_integer_value(json_object_get(first, "block")));
    TEST_ASSERT_EQUAL_INT(7, json_integer_value(json_object_get(first, "affected")));

    json_t* middle = json_array_get(entries, 1);
    TEST_ASSERT_EQUAL_INT(2, json_integer_value(json_object_get(middle, "block")));
    TEST_ASSERT_EQUAL_INT(3, json_integer_value(json_object_get(middle, "affected")));

    json_t* last = json_array_get(entries, 2);
    TEST_ASSERT_EQUAL_INT(3, json_integer_value(json_object_get(last, "block")));
    TEST_ASSERT_EQUAL_INT(7, json_integer_value(json_object_get(last, "affected")));
    json_decref(entries);

    database_engine_cleanup_result(result);
    free(connection->designator);
    free(connection);
}

void test_marked_query_parameters_filtered_per_statement(void) {
    DatabaseHandle* connection = make_connection();
    QueryRequest request = make_request(
        "-- TRANSACTION_START\n"
        "INSERT INTO t (a) VALUES (:A);\n"
        "-- STATEMENT\n"
        "UPDATE t SET b = :B WHERE a = :A;\n"
        "-- TRANSACTION_END", "{\"A\":1,\"B\":2}");
    QueryResult* result = NULL;

    bool success = database_queue_execute_query_with_blocks(connection, &request, &result, "T");
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(result);

    // Each statement is bound only the names it references.
    TEST_ASSERT_EQUAL_STRING("{\"A\":1}", stub_all_parameters[0]);
    TEST_ASSERT_EQUAL_STRING("{\"A\":1,\"B\":2}", stub_all_parameters[1]);

    database_engine_cleanup_result(result);
    free(connection->designator);
    free(connection);
}

void test_marked_query_invalid_request(void) {
    QueryResult* result = NULL;
    bool success = database_queue_execute_query_with_blocks(NULL, NULL, &result, "T");
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    database_engine_cleanup_result(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_marked_query_parse_error);
    RUN_TEST(test_marked_query_single_block);
    RUN_TEST(test_marked_query_block_failure);
    RUN_TEST(test_marked_query_bare_statements);
    RUN_TEST(test_marked_query_parameters_filtered_per_statement);
    RUN_TEST(test_marked_query_invalid_request);

    return UNITY_END();
}
