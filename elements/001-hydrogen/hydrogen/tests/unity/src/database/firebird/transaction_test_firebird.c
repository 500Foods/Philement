/*
 * Unity Test File: Firebird Transaction Management
 * Tests firebird_begin_transaction, firebird_commit_transaction,
 * and firebird_rollback_transaction.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/connection.h>
#include <src/database/firebird/transaction.h>
#include <src/database/firebird/interface.h>

extern bool firebird_begin_transaction(DatabaseHandle* connection,
                                        DatabaseIsolationLevel level,
                                        Transaction** transaction);
extern bool firebird_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
extern bool firebird_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

// Test function prototypes
void test_firebird_begin_transaction_null(void);
void test_firebird_begin_transaction_invalid_engine(void);
void test_firebird_begin_transaction_success(void);
void test_firebird_commit_transaction_null(void);
void test_firebird_commit_transaction_success(void);
void test_firebird_rollback_transaction_null(void);
void test_firebird_rollback_transaction_success(void);

// Helper to create a Firebird connection handle for tests
static DatabaseHandle* create_test_firebird_connection(void) {
    ConnectionConfig config = {0};
    config.database = strdup("/tmp/test.fdb");
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");
    config.port = 3050;

    DatabaseHandle* conn = NULL;
    firebird_connect(&config, &conn, SR_DATABASE);
    return conn;
}

void setUp(void) {
    // No fixtures needed
}

void tearDown(void) {
    // No cleanup needed
}

void test_firebird_begin_transaction_null(void) {
    Transaction* txn = NULL;
    bool result = firebird_begin_transaction(NULL, DB_ISOLATION_READ_COMMITTED, &txn);
    TEST_ASSERT_FALSE(result);

    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBIRD;
    result = firebird_begin_transaction(&handle, DB_ISOLATION_READ_COMMITTED, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_begin_transaction_invalid_engine(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_POSTGRESQL;
    Transaction* txn = NULL;
    bool result = firebird_begin_transaction(&handle, DB_ISOLATION_READ_COMMITTED, &txn);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_begin_transaction_success(void) {
    DatabaseHandle* conn = create_test_firebird_connection();
    TEST_ASSERT_NOT_NULL(conn);

    Transaction* txn = NULL;
    bool result = firebird_begin_transaction(conn, DB_ISOLATION_READ_COMMITTED, &txn);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(txn);
    TEST_ASSERT_TRUE(txn->active);
    TEST_ASSERT_NOT_NULL(conn->current_transaction);

    // Clean up
    free(txn);
    conn->current_transaction = NULL;
    firebird_disconnect(conn);
    pthread_mutex_destroy(&conn->connection_lock);
    if (conn->designator) free((void*)conn->designator);
    free(conn);
}

void test_firebird_commit_transaction_null(void) {
    bool result = firebird_commit_transaction(NULL, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_commit_transaction_success(void) {
    DatabaseHandle* conn = create_test_firebird_connection();
    TEST_ASSERT_NOT_NULL(conn);

    Transaction* txn = NULL;
    firebird_begin_transaction(conn, DB_ISOLATION_READ_COMMITTED, &txn);
    TEST_ASSERT_NOT_NULL(txn);

    bool result = firebird_commit_transaction(conn, txn);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(conn->current_transaction);

    firebird_disconnect(conn);
    pthread_mutex_destroy(&conn->connection_lock);
    if (conn->designator) free((void*)conn->designator);
    free(conn);
}

void test_firebird_rollback_transaction_null(void) {
    bool result = firebird_rollback_transaction(NULL, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_firebird_rollback_transaction_success(void) {
    DatabaseHandle* conn = create_test_firebird_connection();
    TEST_ASSERT_NOT_NULL(conn);

    Transaction* txn = NULL;
    firebird_begin_transaction(conn, DB_ISOLATION_READ_COMMITTED, &txn);
    TEST_ASSERT_NOT_NULL(txn);

    bool result = firebird_rollback_transaction(conn, txn);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(conn->current_transaction);

    firebird_disconnect(conn);
    pthread_mutex_destroy(&conn->connection_lock);
    if (conn->designator) free((void*)conn->designator);
    free(conn);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_firebird_begin_transaction_null);
    RUN_TEST(test_firebird_begin_transaction_invalid_engine);
    RUN_TEST(test_firebird_begin_transaction_success);
    RUN_TEST(test_firebird_commit_transaction_null);
    RUN_TEST(test_firebird_commit_transaction_success);
    RUN_TEST(test_firebird_rollback_transaction_null);
    RUN_TEST(test_firebird_rollback_transaction_success);

    return UNITY_END();
}
