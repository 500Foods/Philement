/*
 * Unity Test File: MSSQL transaction management functions
 * Tests mssql_begin_transaction, mssql_commit_transaction,
 * and mssql_rollback_transaction.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/mssql/types.h>
#include <src/database/mssql/connection.h>
#include <src/database/mssql/transaction.h>
#include <src/utils/utils_uuid.h>

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_libodbc.h>
#include <unity/mocks/mock_logging.h>

#define SQL_SUCCESS 0
#define SQL_SUCCESS_WITH_INFO 1
#define SQL_NO_DATA 100
#define SQL_HANDLE_STMT 3

extern SQLEndTran_t mssql_SQLEndTran_ptr;
extern SQLGetDiagRec_t mssql_SQLGetDiagRec_ptr;

/* Test declarations for mssql_begin_transaction */
void test_mssql_begin_tx_null_params(void);
void test_mssql_begin_tx_wrong_engine(void);
void test_mssql_begin_tx_null_mssql_conn(void);
void test_mssql_begin_tx_null_connection_in_mssql(void);
void test_mssql_begin_tx_no_endtran_ptr(void);
void test_mssql_begin_tx_endtran_failure(void);
void test_mssql_begin_tx_success(void);
void test_mssql_begin_tx_success_with_designator(void);
void test_mssql_begin_tx_calloc_tx_failure(void);
void test_mssql_begin_tx_strdup_failure(void);

/* Test declarations for mssql_commit_transaction */
void test_mssql_commit_tx_null_params(void);
void test_mssql_commit_tx_wrong_engine(void);
void test_mssql_commit_tx_null_mssql_conn(void);
void test_mssql_commit_tx_null_connection_in_mssql(void);
void test_mssql_commit_tx_no_endtran_ptr(void);
void test_mssql_commit_tx_endtran_failure(void);
void test_mssql_commit_tx_success(void);
void test_mssql_commit_tx_success_no_endtran_ptr(void);

/* Test declarations for mssql_rollback_transaction */
void test_mssql_rollback_tx_null_params(void);
void test_mssql_rollback_tx_wrong_engine(void);
void test_mssql_rollback_tx_null_mssql_conn(void);
void test_mssql_rollback_tx_null_connection_in_mssql(void);
void test_mssql_rollback_tx_no_endtran_ptr(void);
void test_mssql_rollback_tx_success_no_endtran_ptr(void);
void test_mssql_rollback_tx_endtran_failure_with_diag(void);
void test_mssql_rollback_tx_endtran_failure_no_diag(void);
void test_mssql_rollback_tx_endtran_failure_getdiagrec_null(void);
void test_mssql_rollback_tx_endtran_success_with_info(void);
void test_mssql_rollback_tx_success(void);

#define NUM_TESTS 24

/* Helper to create a DatabaseHandle for testing */
static DatabaseHandle* test_create_tx_connection(void) {
    DatabaseHandle* conn = malloc(sizeof(DatabaseHandle));
    if (!conn) return NULL;
    memset(conn, 0, sizeof(DatabaseHandle));
    conn->engine_type = DB_ENGINE_MSSQL;
    conn->designator = NULL;
    conn->config = malloc(sizeof(ConnectionConfig));
    if (conn->config) {
        memset(conn->config, 0, sizeof(ConnectionConfig));
    }
    conn->connection_handle = malloc(sizeof(MSSQLConnection));
    if (conn->connection_handle) {
        memset(conn->connection_handle, 0, sizeof(MSSQLConnection));
        MSSQLConnection* mc = (MSSQLConnection*)conn->connection_handle;
        mc->connection = (void*)0xDEADBEEF;
    }
    return conn;
}

static void test_free_tx_connection(DatabaseHandle* conn) {
    if (!conn) return;
    if (conn->current_transaction) {
        free(conn->current_transaction->transaction_id);
        free(conn->current_transaction);
        conn->current_transaction = NULL;
    }
    if (conn->config) free(conn->config);
    if (conn->connection_handle) free(conn->connection_handle);
    free(conn);
}

static Transaction* test_create_transaction(void) {
    Transaction* tx = calloc(1, sizeof(Transaction));
    if (!tx) return NULL;
    tx->transaction_id = strdup("test-tx-id");
    tx->isolation_level = DB_ISOLATION_READ_COMMITTED;
    tx->started_at = time(NULL);
    tx->active = true;
    return tx;
}

static void test_free_transaction(Transaction* tx) {
    if (!tx) return;
    free(tx->transaction_id);
    free(tx);
}

void setUp(void) {
    mssql_mock_libodbc_reset_all();
    mock_system_reset_all();
    mock_logging_reset_all();
    load_msobdc_functions("MSSQL-TEST");
}

void tearDown(void) {
}

/*
 * mssql_begin_transaction tests
 */

void test_mssql_begin_tx_null_params(void) {
    Transaction* tx = NULL;

    TEST_ASSERT_FALSE(mssql_begin_transaction(NULL, DB_ISOLATION_READ_COMMITTED, &tx));
    TEST_ASSERT_FALSE(mssql_begin_transaction(NULL, DB_ISOLATION_READ_COMMITTED, NULL));
    TEST_ASSERT_FALSE(mssql_begin_transaction(NULL, DB_ISOLATION_READ_COMMITTED, NULL));
}

void test_mssql_begin_tx_wrong_engine(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);
    conn->engine_type = DB_ENGINE_POSTGRESQL;

    Transaction* tx = NULL;
    TEST_ASSERT_FALSE(mssql_begin_transaction(conn, DB_ISOLATION_READ_COMMITTED, &tx));

    test_free_tx_connection(conn);
}

void test_mssql_begin_tx_null_mssql_conn(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);
    free(conn->connection_handle);
    conn->connection_handle = NULL;

    Transaction* tx = NULL;
    TEST_ASSERT_FALSE(mssql_begin_transaction(conn, DB_ISOLATION_READ_COMMITTED, &tx));

    test_free_tx_connection(conn);
}

void test_mssql_begin_tx_null_connection_in_mssql(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);
    ((MSSQLConnection*)conn->connection_handle)->connection = NULL;

    Transaction* tx = NULL;
    TEST_ASSERT_FALSE(mssql_begin_transaction(conn, DB_ISOLATION_READ_COMMITTED, &tx));

    test_free_tx_connection(conn);
}

void test_mssql_begin_tx_no_endtran_ptr(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);

    SQLEndTran_t saved = mssql_SQLEndTran_ptr;
    mssql_SQLEndTran_ptr = NULL;

    Transaction* tx = NULL;
    TEST_ASSERT_TRUE(mssql_begin_transaction(conn, DB_ISOLATION_READ_COMMITTED, &tx));
    TEST_ASSERT_NOT_NULL(tx);
    TEST_ASSERT_TRUE(tx->active);
    TEST_ASSERT_EQUAL_INT(DB_ISOLATION_READ_COMMITTED, tx->isolation_level);
    TEST_ASSERT_NOT_NULL(tx->transaction_id);
    TEST_ASSERT_EQUAL_PTR(tx, conn->current_transaction);

    test_free_transaction(tx);
    mssql_SQLEndTran_ptr = saved;
    conn->current_transaction = NULL;
    test_free_tx_connection(conn);
}

void test_mssql_begin_tx_endtran_failure(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);

    mssql_mock_libodbc_set_SQLEndTran_result(-1);

    Transaction* tx = NULL;
    TEST_ASSERT_FALSE(mssql_begin_transaction(conn, DB_ISOLATION_READ_COMMITTED, &tx));
    TEST_ASSERT_NULL(tx);

    test_free_tx_connection(conn);
}

void test_mssql_begin_tx_success(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);

    Transaction* tx = NULL;
    TEST_ASSERT_TRUE(mssql_begin_transaction(conn, DB_ISOLATION_READ_COMMITTED, &tx));
    TEST_ASSERT_NOT_NULL(tx);
    TEST_ASSERT_TRUE(tx->active);
    TEST_ASSERT_EQUAL_INT(DB_ISOLATION_READ_COMMITTED, tx->isolation_level);
    TEST_ASSERT_NOT_NULL(tx->transaction_id);
    TEST_ASSERT_EQUAL_PTR(tx, conn->current_transaction);

    test_free_transaction(tx);
    conn->current_transaction = NULL;
    test_free_tx_connection(conn);
}

void test_mssql_begin_tx_success_with_designator(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);
    conn->designator = strdup("DQM-TEST");

    Transaction* tx = NULL;
    TEST_ASSERT_TRUE(mssql_begin_transaction(conn, DB_ISOLATION_SERIALIZABLE, &tx));
    TEST_ASSERT_NOT_NULL(tx);
    TEST_ASSERT_TRUE(tx->active);
    TEST_ASSERT_EQUAL_INT(DB_ISOLATION_SERIALIZABLE, tx->isolation_level);

    test_free_transaction(tx);
    conn->current_transaction = NULL;
    free(conn->designator);
    conn->designator = NULL;
    test_free_tx_connection(conn);
}

void test_mssql_begin_tx_calloc_tx_failure(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);

    /* Allocation sequence in mssql_begin_transaction:
     * 1: calloc(Transaction) -> calloc_failure(1) triggers here */
    mock_system_set_calloc_failure(1);
    Transaction* tx = NULL;
    TEST_ASSERT_FALSE(mssql_begin_transaction(conn, DB_ISOLATION_READ_COMMITTED, &tx));
    mock_system_set_calloc_failure(0);

    test_free_tx_connection(conn);
}

void test_mssql_begin_tx_strdup_failure(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);

    /* Allocation sequence in mssql_begin_transaction:
     * 1: calloc(Transaction) -> succeeds (count=1)
     * 2: strdup(uuid) -> FAILS (malloc_failure(2)) */
    mock_system_set_malloc_failure(2);
    Transaction* tx = NULL;
    TEST_ASSERT_FALSE(mssql_begin_transaction(conn, DB_ISOLATION_READ_COMMITTED, &tx));
    mock_system_set_malloc_failure(0);

    test_free_tx_connection(conn);
}

/*
 * mssql_commit_transaction tests
 */

void test_mssql_commit_tx_null_params(void) {
    TEST_ASSERT_FALSE(mssql_commit_transaction(NULL, NULL));
    TEST_ASSERT_FALSE(mssql_commit_transaction(NULL, (Transaction*)0x1));
}

void test_mssql_commit_tx_wrong_engine(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);
    conn->engine_type = DB_ENGINE_POSTGRESQL;

    Transaction* tx = test_create_transaction();
    TEST_ASSERT_FALSE(mssql_commit_transaction(conn, tx));

    test_free_transaction(tx);
    test_free_tx_connection(conn);
}

void test_mssql_commit_tx_null_mssql_conn(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);
    free(conn->connection_handle);
    conn->connection_handle = NULL;

    Transaction* tx = test_create_transaction();
    TEST_ASSERT_FALSE(mssql_commit_transaction(conn, tx));

    test_free_transaction(tx);
    test_free_tx_connection(conn);
}

void test_mssql_commit_tx_null_connection_in_mssql(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);
    ((MSSQLConnection*)conn->connection_handle)->connection = NULL;

    Transaction* tx = test_create_transaction();
    TEST_ASSERT_FALSE(mssql_commit_transaction(conn, tx));

    test_free_transaction(tx);
    test_free_tx_connection(conn);
}

void test_mssql_commit_tx_endtran_failure(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);

    mssql_mock_libodbc_set_SQLEndTran_result(-1);

    Transaction* tx = test_create_transaction();
    conn->current_transaction = tx;

    TEST_ASSERT_FALSE(mssql_commit_transaction(conn, tx));
    TEST_ASSERT_TRUE(tx->active);

    test_free_transaction(tx);
    conn->current_transaction = NULL;
    test_free_tx_connection(conn);
}

void test_mssql_commit_tx_success(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);

    Transaction* tx = test_create_transaction();
    conn->current_transaction = tx;

    TEST_ASSERT_TRUE(mssql_commit_transaction(conn, tx));
    TEST_ASSERT_FALSE(tx->active);
    TEST_ASSERT_NULL(conn->current_transaction);

    free(tx->transaction_id);
    free(tx);
    test_free_tx_connection(conn);
}

void test_mssql_commit_tx_success_no_endtran_ptr(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);

    SQLEndTran_t saved = mssql_SQLEndTran_ptr;
    mssql_SQLEndTran_ptr = NULL;

    Transaction* tx = test_create_transaction();
    conn->current_transaction = tx;

    TEST_ASSERT_TRUE(mssql_commit_transaction(conn, tx));
    TEST_ASSERT_FALSE(tx->active);
    TEST_ASSERT_NULL(conn->current_transaction);

    free(tx->transaction_id);
    free(tx);
    mssql_SQLEndTran_ptr = saved;
    test_free_tx_connection(conn);
}

/*
 * mssql_rollback_transaction tests
 */

void test_mssql_rollback_tx_null_params(void) {
    TEST_ASSERT_FALSE(mssql_rollback_transaction(NULL, NULL));
    TEST_ASSERT_FALSE(mssql_rollback_transaction(NULL, (Transaction*)0x1));
}

void test_mssql_rollback_tx_wrong_engine(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);
    conn->engine_type = DB_ENGINE_POSTGRESQL;

    Transaction* tx = test_create_transaction();
    TEST_ASSERT_FALSE(mssql_rollback_transaction(conn, tx));

    test_free_transaction(tx);
    test_free_tx_connection(conn);
}

void test_mssql_rollback_tx_null_mssql_conn(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);
    free(conn->connection_handle);
    conn->connection_handle = NULL;

    Transaction* tx = test_create_transaction();
    TEST_ASSERT_FALSE(mssql_rollback_transaction(conn, tx));

    test_free_transaction(tx);
    test_free_tx_connection(conn);
}

void test_mssql_rollback_tx_null_connection_in_mssql(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);
    ((MSSQLConnection*)conn->connection_handle)->connection = NULL;

    Transaction* tx = test_create_transaction();
    TEST_ASSERT_FALSE(mssql_rollback_transaction(conn, tx));

    test_free_transaction(tx);
    test_free_tx_connection(conn);
}

void test_mssql_rollback_tx_no_endtran_ptr(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);

    SQLEndTran_t saved = mssql_SQLEndTran_ptr;
    mssql_SQLEndTran_ptr = NULL;

    Transaction* tx = test_create_transaction();
    conn->current_transaction = tx;

    TEST_ASSERT_TRUE(mssql_rollback_transaction(conn, tx));
    TEST_ASSERT_FALSE(tx->active);
    TEST_ASSERT_NULL(conn->current_transaction);

    free(tx->transaction_id);
    free(tx);
    mssql_SQLEndTran_ptr = saved;
    test_free_tx_connection(conn);
}

void test_mssql_rollback_tx_success_no_endtran_ptr(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);

    SQLEndTran_t saved = mssql_SQLEndTran_ptr;
    mssql_SQLEndTran_ptr = NULL;

    Transaction* tx = test_create_transaction();
    conn->current_transaction = tx;

    TEST_ASSERT_TRUE(mssql_rollback_transaction(conn, tx));
    TEST_ASSERT_FALSE(tx->active);
    TEST_ASSERT_NULL(conn->current_transaction);

    free(tx->transaction_id);
    free(tx);
    mssql_SQLEndTran_ptr = saved;
    test_free_tx_connection(conn);
}

void test_mssql_rollback_tx_endtran_failure_with_diag(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);

    /* SQLEndTran returns -1 (not SQL_SUCCESS or SQL_SUCCESS_WITH_INFO) */
    mssql_mock_libodbc_set_SQLEndTran_result(-1);

    /* SQLGetDiagRec returns SQL_SUCCESS */
    mssql_mock_libodbc_set_SQLGetDiagRec_result(0);

    Transaction* tx = test_create_transaction();
    conn->current_transaction = tx;

    TEST_ASSERT_FALSE(mssql_rollback_transaction(conn, tx));
    TEST_ASSERT_TRUE(tx->active);

    test_free_transaction(tx);
    conn->current_transaction = NULL;
    test_free_tx_connection(conn);
}

void test_mssql_rollback_tx_endtran_failure_no_diag(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);

    /* SQLEndTran returns -1 */
    mssql_mock_libodbc_set_SQLEndTran_result(-1);

    /* SQLGetDiagRec returns something other than SQL_SUCCESS or SQL_SUCCESS_WITH_INFO */
    mssql_mock_libodbc_set_SQLGetDiagRec_result(-1);

    Transaction* tx = test_create_transaction();
    conn->current_transaction = tx;

    TEST_ASSERT_FALSE(mssql_rollback_transaction(conn, tx));
    TEST_ASSERT_TRUE(tx->active);

    test_free_transaction(tx);
    conn->current_transaction = NULL;
    test_free_tx_connection(conn);
}

void test_mssql_rollback_tx_endtran_failure_getdiagrec_null(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);

    /* SQLEndTran returns -1 */
    mssql_mock_libodbc_set_SQLEndTran_result(-1);

    /* Set SQLGetDiagRec_ptr to NULL */
    SQLGetDiagRec_t saved_diag = mssql_SQLGetDiagRec_ptr;
    mssql_SQLGetDiagRec_ptr = NULL;

    Transaction* tx = test_create_transaction();
    conn->current_transaction = tx;

    TEST_ASSERT_FALSE(mssql_rollback_transaction(conn, tx));
    TEST_ASSERT_TRUE(tx->active);

    test_free_transaction(tx);
    conn->current_transaction = NULL;
    mssql_SQLGetDiagRec_ptr = saved_diag;
    test_free_tx_connection(conn);
}

void test_mssql_rollback_tx_endtran_success_with_info(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);

    /* SQLEndTran returns SQL_SUCCESS_WITH_INFO (1) */
    mssql_mock_libodbc_set_SQLEndTran_result(1);

    Transaction* tx = test_create_transaction();
    conn->current_transaction = tx;

    TEST_ASSERT_TRUE(mssql_rollback_transaction(conn, tx));
    TEST_ASSERT_FALSE(tx->active);
    TEST_ASSERT_NULL(conn->current_transaction);

    free(tx->transaction_id);
    free(tx);
    test_free_tx_connection(conn);
}

void test_mssql_rollback_tx_success(void) {
    DatabaseHandle* conn = test_create_tx_connection();
    TEST_ASSERT_NOT_NULL(conn);

    Transaction* tx = test_create_transaction();
    conn->current_transaction = tx;

    TEST_ASSERT_TRUE(mssql_rollback_transaction(conn, tx));
    TEST_ASSERT_FALSE(tx->active);
    TEST_ASSERT_NULL(conn->current_transaction);

    free(tx->transaction_id);
    free(tx);
    test_free_tx_connection(conn);
}

int main(void) {
    UNITY_BEGIN();

    /* mssql_begin_transaction */
    RUN_TEST(test_mssql_begin_tx_null_params);
    RUN_TEST(test_mssql_begin_tx_wrong_engine);
    RUN_TEST(test_mssql_begin_tx_null_mssql_conn);
    RUN_TEST(test_mssql_begin_tx_null_connection_in_mssql);
    RUN_TEST(test_mssql_begin_tx_no_endtran_ptr);
    RUN_TEST(test_mssql_begin_tx_endtran_failure);
    RUN_TEST(test_mssql_begin_tx_success);
    RUN_TEST(test_mssql_begin_tx_success_with_designator);
    RUN_TEST(test_mssql_begin_tx_calloc_tx_failure);
    RUN_TEST(test_mssql_begin_tx_strdup_failure);

    /* mssql_commit_transaction */
    RUN_TEST(test_mssql_commit_tx_null_params);
    RUN_TEST(test_mssql_commit_tx_wrong_engine);
    RUN_TEST(test_mssql_commit_tx_null_mssql_conn);
    RUN_TEST(test_mssql_commit_tx_null_connection_in_mssql);
    RUN_TEST(test_mssql_commit_tx_endtran_failure);
    RUN_TEST(test_mssql_commit_tx_success);
    RUN_TEST(test_mssql_commit_tx_success_no_endtran_ptr);

    /* mssql_rollback_transaction */
    RUN_TEST(test_mssql_rollback_tx_null_params);
    RUN_TEST(test_mssql_rollback_tx_wrong_engine);
    RUN_TEST(test_mssql_rollback_tx_null_mssql_conn);
    RUN_TEST(test_mssql_rollback_tx_null_connection_in_mssql);
    RUN_TEST(test_mssql_rollback_tx_no_endtran_ptr);
    RUN_TEST(test_mssql_rollback_tx_success_no_endtran_ptr);
    RUN_TEST(test_mssql_rollback_tx_endtran_failure_with_diag);
    RUN_TEST(test_mssql_rollback_tx_endtran_failure_no_diag);
    RUN_TEST(test_mssql_rollback_tx_endtran_failure_getdiagrec_null);
    RUN_TEST(test_mssql_rollback_tx_endtran_success_with_info);
    RUN_TEST(test_mssql_rollback_tx_success);

    return UNITY_END();
}
