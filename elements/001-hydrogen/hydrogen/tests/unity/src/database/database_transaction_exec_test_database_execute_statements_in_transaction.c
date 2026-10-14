/*
 * Unity Test File: database_transaction_exec_test_database_execute_statements_in_transaction
 * Unit tests for the shared transactional statement executor.
 *
 * A stub engine interface is registered for DB_ENGINE_AI (no DDL
 * segmentation) and DB_ENGINE_FIREBIRD (DDL forces commit-and-reopen),
 * so the executor's transaction choreography runs in-process.
 */

#include <stdlib.h>
#include <string.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/database_transaction_exec.h>

// ----- stub engine ---------------------------------------------------------

static int stub_begin_calls;
static int stub_commit_calls;
static int stub_rollback_calls;
static int stub_execute_calls;
static bool stub_begin_ok = true;
static bool stub_commit_ok = true;
static int stub_fail_execute_call = 0;   // 1-based call number that fails; 0 = none
static bool stub_prepared_seen = false;
static char stub_last_parameters[256];

static void stub_reset(void) {
    stub_begin_calls = 0;
    stub_commit_calls = 0;
    stub_rollback_calls = 0;
    stub_execute_calls = 0;
    stub_begin_ok = true;
    stub_commit_ok = true;
    stub_fail_execute_call = 0;
    stub_prepared_seen = false;
    stub_last_parameters[0] = '\0';
}

static bool stub_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    (void)connection;
    (void)level;
    if (!stub_begin_ok) {
        return false;
    }
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
    // Ownership stays with database_engine_cleanup_transaction, matching
    // the real engines: commit never frees the handle.
    (void)connection;
    (void)transaction;
    if (!stub_commit_ok) {
        return false;
    }
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
        stub_prepared_seen = request->use_prepared_statement &&
                             request->prepared_statement_name != NULL;
        if (request->parameters_json) {
            snprintf(stub_last_parameters, sizeof(stub_last_parameters), "%s", request->parameters_json);
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
    }
    *result = res;
    return true;
}

static DatabaseEngineInterface ai_engine;
static DatabaseEngineInterface firebird_engine;
static bool engines_registered = false;

static void register_stub_engines(void) {
    if (engines_registered) {
        return;
    }
    database_engine_init();

    memset(&ai_engine, 0, sizeof(ai_engine));
    ai_engine.engine_type = DB_ENGINE_AI;
    ai_engine.name = (char*)"txn_exec_ai";
    ai_engine.execute_query = stub_execute_query;
    ai_engine.begin_transaction = stub_begin_transaction;
    ai_engine.commit_transaction = stub_commit_transaction;
    ai_engine.rollback_transaction = stub_rollback_transaction;
    (void)database_engine_register(&ai_engine);

    memset(&firebird_engine, 0, sizeof(firebird_engine));
    firebird_engine.engine_type = DB_ENGINE_FIREBIRD;
    firebird_engine.name = (char*)"txn_exec_firebird";
    firebird_engine.execute_query = stub_execute_query;
    firebird_engine.begin_transaction = stub_begin_transaction;
    firebird_engine.commit_transaction = stub_commit_transaction;
    firebird_engine.rollback_transaction = stub_rollback_transaction;
    (void)database_engine_register(&firebird_engine);

    engines_registered = true;
}

static DatabaseHandle* make_connection(DatabaseEngine engine_type) {
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    if (connection) {
        connection->engine_type = engine_type;
        connection->designator = strdup("TXN-EXEC-TEST");
    }
    return connection;
}

// ----- tests ---------------------------------------------------------------

void test_executor_null_connection(void);
void test_executor_empty_statement_list(void);
void test_executor_begin_failure(void);
void test_executor_happy_path(void);
void test_executor_statement_failure_rolls_back(void);
void test_executor_callback_can_abort(void);
void test_executor_ddl_segmentation_on_firebird(void);
void test_executor_ddl_last_statement_reopens_nothing(void);
void test_executor_no_ddl_segmentation_on_transactional_engine(void);
void test_executor_uses_prepared_statements_and_forwards_parameters(void);

void setUp(void) {
    register_stub_engines();
    stub_reset();
}

void tearDown(void) {
}

static char* dup_stmt(const char* text) {
    return strdup(text);
}

void test_executor_null_connection(void) {
    char* statements[] = { dup_stmt("SELECT 1;") };
    TransactionExecOutcome outcome;
    TEST_ASSERT_FALSE(database_execute_statements_in_transaction(
        NULL, statements, 1, NULL, 0, NULL, NULL, false, NULL, NULL, NULL, &outcome, "T"));
    free(statements[0]);
}

void test_executor_empty_statement_list(void) {
    DatabaseHandle* connection = make_connection(DB_ENGINE_AI);
    TransactionExecOutcome outcome;
    TEST_ASSERT_TRUE(database_execute_statements_in_transaction(
        connection, NULL, 0, NULL, 0, NULL, NULL, false, NULL, NULL, NULL, &outcome, "T"));
    TEST_ASSERT_TRUE(outcome.success);
    TEST_ASSERT_EQUAL_size_t(0, outcome.statements_executed);
    TEST_ASSERT_EQUAL_size_t((size_t)-1, outcome.failed_statement_index);
    TEST_ASSERT_EQUAL_INT(0, stub_begin_calls);
    free(connection->designator);
    free(connection);
}

void test_executor_begin_failure(void) {
    DatabaseHandle* connection = make_connection(DB_ENGINE_AI);
    char* statements[] = { dup_stmt("SELECT 1;") };
    TransactionExecOutcome outcome;
    stub_begin_ok = false;

    TEST_ASSERT_FALSE(database_execute_statements_in_transaction(
        connection, statements, 1, NULL, 0, NULL, NULL, false, NULL, NULL, NULL, &outcome, "T"));
    TEST_ASSERT_TRUE(outcome.failure_at_begin);
    TEST_ASSERT_FALSE(outcome.success);
    TEST_ASSERT_EQUAL_INT(0, stub_execute_calls);

    free(statements[0]);
    free(connection->designator);
    free(connection);
}

void test_executor_happy_path(void) {
    DatabaseHandle* connection = make_connection(DB_ENGINE_AI);
    char* statements[] = { dup_stmt("INSERT INTO t VALUES (1);"), dup_stmt("UPDATE t SET a = 2;"),
                           dup_stmt("DELETE FROM t;") };
    TransactionExecOutcome outcome;

    TEST_ASSERT_TRUE(database_execute_statements_in_transaction(
        connection, statements, 3, NULL, 0, NULL, NULL, false, NULL, NULL, NULL, &outcome, "T"));
    TEST_ASSERT_TRUE(outcome.success);
    TEST_ASSERT_EQUAL_size_t(3, outcome.statements_executed);
    TEST_ASSERT_EQUAL_INT(1, stub_begin_calls);
    TEST_ASSERT_EQUAL_INT(1, stub_commit_calls);
    TEST_ASSERT_EQUAL_INT(0, stub_rollback_calls);
    TEST_ASSERT_EQUAL_INT(3, stub_execute_calls);

    for (size_t i = 0; i < 3; i++) {
        free(statements[i]);
    }
    free(connection->designator);
    free(connection);
}

void test_executor_statement_failure_rolls_back(void) {
    DatabaseHandle* connection = make_connection(DB_ENGINE_AI);
    char* statements[] = { dup_stmt("INSERT INTO t VALUES (1);"), dup_stmt("INSERT INTO t VALUES (2);"),
                           dup_stmt("INSERT INTO t VALUES (3);") };
    TransactionExecOutcome outcome;
    stub_fail_execute_call = 2;

    TEST_ASSERT_FALSE(database_execute_statements_in_transaction(
        connection, statements, 3, NULL, 0, NULL, NULL, false, NULL, NULL, NULL, &outcome, "T"));
    TEST_ASSERT_FALSE(outcome.success);
    TEST_ASSERT_EQUAL_size_t(1, outcome.failed_statement_index);
    TEST_ASSERT_EQUAL_size_t(2, outcome.statements_executed);
    TEST_ASSERT_EQUAL_INT(1, stub_rollback_calls);
    TEST_ASSERT_EQUAL_INT(0, stub_commit_calls);
    TEST_ASSERT_EQUAL_INT(2, stub_execute_calls);

    for (size_t i = 0; i < 3; i++) {
        free(statements[i]);
    }
    free(connection->designator);
    free(connection);
}

static size_t callback_seen_index;
static size_t callback_calls;
static long long callback_last_affected;

static bool recording_callback(size_t statement_index, const char* statement,
                               const QueryResult* result, void* context) {
    (void)statement;
    (void)context;
    callback_seen_index = statement_index;
    callback_calls++;
    callback_last_affected = result ? result->affected_rows : -1;
    return true;
}

static bool aborting_callback(size_t statement_index, const char* statement,
                              const QueryResult* result, void* context) {
    (void)statement_index;
    (void)statement;
    (void)result;
    (void)context;
    return false;
}

void test_executor_callback_can_abort(void) {
    DatabaseHandle* connection = make_connection(DB_ENGINE_AI);
    char* statements[] = { dup_stmt("INSERT INTO t VALUES (1);"), dup_stmt("UPDATE t SET a = 2;") };
    TransactionExecOutcome outcome;

    TEST_ASSERT_TRUE(database_execute_statements_in_transaction(
        connection, statements, 2, NULL, 0, NULL, NULL, false, recording_callback,
        NULL, NULL, &outcome, "T"));
    TEST_ASSERT_EQUAL_size_t(2, callback_calls);
    TEST_ASSERT_EQUAL_size_t(1, callback_seen_index);
    TEST_ASSERT_EQUAL_INT(3, callback_last_affected);
    stub_reset();

    // A callback that refuses statement 2 aborts the block and rolls back.
    TEST_ASSERT_FALSE(database_execute_statements_in_transaction(
        connection, statements, 2, NULL, 0, NULL, NULL, false, aborting_callback,
        NULL, NULL, &outcome, "T"));
    TEST_ASSERT_EQUAL_INT(1, stub_rollback_calls);
    TEST_ASSERT_EQUAL_INT(0, stub_commit_calls);
    TEST_ASSERT_EQUAL_INT(1, stub_execute_calls);
    TEST_ASSERT_EQUAL_size_t(0, outcome.failed_statement_index);

    for (size_t i = 0; i < 2; i++) {
        free(statements[i]);
    }
    free(connection->designator);
    free(connection);
}

void test_executor_ddl_segmentation_on_firebird(void) {
    DatabaseHandle* connection = make_connection(DB_ENGINE_FIREBIRD);
    char* statements[] = { dup_stmt("INSERT INTO t VALUES (1);"), dup_stmt("CREATE TABLE u (a INT);"),
                           dup_stmt("INSERT INTO u VALUES (2);") };
    TransactionExecOutcome outcome;

    TEST_ASSERT_TRUE(database_execute_statements_in_transaction(
        connection, statements, 3, NULL, 0, NULL, NULL, false, NULL, NULL, NULL, &outcome, "T"));
    TEST_ASSERT_TRUE(outcome.success);
    // begin -> DDL commit -> reopen -> final commit
    TEST_ASSERT_EQUAL_INT(2, stub_begin_calls);
    TEST_ASSERT_EQUAL_INT(2, stub_commit_calls);
    TEST_ASSERT_EQUAL_INT(0, stub_rollback_calls);
    TEST_ASSERT_EQUAL_INT(3, stub_execute_calls);

    for (size_t i = 0; i < 3; i++) {
        free(statements[i]);
    }
    free(connection->designator);
    free(connection);
}

void test_executor_ddl_last_statement_reopens_nothing(void) {
    DatabaseHandle* connection = make_connection(DB_ENGINE_FIREBIRD);
    char* statements[] = { dup_stmt("INSERT INTO t VALUES (1);"), dup_stmt("DROP TABLE t;") };
    TransactionExecOutcome outcome;

    TEST_ASSERT_TRUE(database_execute_statements_in_transaction(
        connection, statements, 2, NULL, 0, NULL, NULL, false, NULL, NULL, NULL, &outcome, "T"));
    // begin -> commit at DDL -> DDL runs in autocommit -> nothing left to commit
    TEST_ASSERT_EQUAL_INT(1, stub_begin_calls);
    TEST_ASSERT_EQUAL_INT(1, stub_commit_calls);
    TEST_ASSERT_EQUAL_INT(2, stub_execute_calls);

    for (size_t i = 0; i < 2; i++) {
        free(statements[i]);
    }
    free(connection->designator);
    free(connection);
}

void test_executor_no_ddl_segmentation_on_transactional_engine(void) {
    DatabaseHandle* connection = make_connection(DB_ENGINE_AI);
    char* statements[] = { dup_stmt("INSERT INTO t VALUES (1);"), dup_stmt("CREATE TABLE u (a INT);"),
                           dup_stmt("INSERT INTO u VALUES (2);") };
    TransactionExecOutcome outcome;

    TEST_ASSERT_TRUE(database_execute_statements_in_transaction(
        connection, statements, 3, NULL, 0, NULL, NULL, false, NULL, NULL, NULL, &outcome, "T"));
    // One transaction for the whole block, DDL included.
    TEST_ASSERT_EQUAL_INT(1, stub_begin_calls);
    TEST_ASSERT_EQUAL_INT(1, stub_commit_calls);
    TEST_ASSERT_EQUAL_INT(3, stub_execute_calls);

    for (size_t i = 0; i < 3; i++) {
        free(statements[i]);
    }
    free(connection->designator);
    free(connection);
}

static const char* per_statement_parameters(size_t statement_index, const char* statement, void* context) {
    (void)statement;
    (void)context;
    return statement_index == 0 ? "{\"A\":1}" : "{\"B\":2}";
}

void test_executor_uses_prepared_statements_and_forwards_parameters(void) {
    DatabaseHandle* connection = make_connection(DB_ENGINE_AI);
    char* statements[] = { dup_stmt("INSERT INTO t VALUES (:A);"), dup_stmt("UPDATE t SET b = :B;") };
    TransactionExecOutcome outcome;

    TEST_ASSERT_TRUE(database_execute_statements_in_transaction(
        connection, statements, 2, "{\"A\":1,\"B\":2}", 45, "runtime_statement", "RTXN",
        true, NULL, per_statement_parameters, NULL, &outcome, "T"));
    TEST_ASSERT_TRUE(stub_prepared_seen);
    // The parameters callback won, so the executor-wide JSON was replaced
    // per statement; the last statement saw its own parameters.
    TEST_ASSERT_EQUAL_STRING("{\"B\":2}", stub_last_parameters);

    for (size_t i = 0; i < 2; i++) {
        free(statements[i]);
    }
    free(connection->designator);
    free(connection);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_executor_null_connection);
    RUN_TEST(test_executor_empty_statement_list);
    RUN_TEST(test_executor_begin_failure);
    RUN_TEST(test_executor_happy_path);
    RUN_TEST(test_executor_statement_failure_rolls_back);
    RUN_TEST(test_executor_callback_can_abort);
    RUN_TEST(test_executor_ddl_segmentation_on_firebird);
    RUN_TEST(test_executor_ddl_last_statement_reopens_nothing);
    RUN_TEST(test_executor_no_ddl_segmentation_on_transactional_engine);
    RUN_TEST(test_executor_uses_prepared_statements_and_forwards_parameters);

    return UNITY_END();
}
