/*
 * Unity Test File: database_transaction_exec_test_engine_ddl_breaks_transaction
 * Unit tests for the engine_ddl_breaks_transaction() policy helper.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database_transaction_exec.h>

void test_engine_ddl_breaks_transaction_null_and_defaults(void);
void test_engine_ddl_breaks_transaction_transactional_ddl_engines(void);
void test_engine_ddl_breaks_transaction_breaking_engines(void);
void test_engine_ddl_breaks_transaction_ai_mock_slot(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_engine_ddl_breaks_transaction_transactional_ddl_engines(void) {
    // PostgreSQL (and the YugabyteDB path), SQLite, DB2, MSSQL keep DDL
    // inside the transaction: a block stays fully atomic.
    TEST_ASSERT_FALSE(engine_ddl_breaks_transaction(DB_ENGINE_POSTGRESQL));
    TEST_ASSERT_FALSE(engine_ddl_breaks_transaction(DB_ENGINE_SQLITE));
    TEST_ASSERT_FALSE(engine_ddl_breaks_transaction(DB_ENGINE_DB2));
    TEST_ASSERT_FALSE(engine_ddl_breaks_transaction(DB_ENGINE_MSSQL));
}

void test_engine_ddl_breaks_transaction_breaking_engines(void) {
    // MySQL/MariaDB commit implicitly on DDL; Firebird hides uncommitted
    // metadata from later DSQL.
    TEST_ASSERT_TRUE(engine_ddl_breaks_transaction(DB_ENGINE_MYSQL));
    TEST_ASSERT_TRUE(engine_ddl_breaks_transaction(DB_ENGINE_MARIADB));
    TEST_ASSERT_TRUE(engine_ddl_breaks_transaction(DB_ENGINE_FIREBIRD));
}

void test_engine_ddl_breaks_transaction_ai_mock_slot(void) {
    // The Unity mock slot must not claim DDL-breaking behavior.
    TEST_ASSERT_FALSE(engine_ddl_breaks_transaction(DB_ENGINE_AI));
}

void test_engine_ddl_breaks_transaction_null_and_defaults(void) {
    // Out-of-range values fall through to the non-breaking default.
    TEST_ASSERT_FALSE(engine_ddl_breaks_transaction((DatabaseEngine)999));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_engine_ddl_breaks_transaction_transactional_ddl_engines);
    RUN_TEST(test_engine_ddl_breaks_transaction_breaking_engines);
    RUN_TEST(test_engine_ddl_breaks_transaction_ai_mock_slot);
    RUN_TEST(test_engine_ddl_breaks_transaction_null_and_defaults);

    return UNITY_END();
}
