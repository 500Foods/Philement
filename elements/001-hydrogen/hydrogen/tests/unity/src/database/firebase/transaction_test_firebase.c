/*
 * Unity tests for Phase 3 Firebase transaction stubs.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebase/transaction.h>

void test_firebase_begin_transaction_not_implemented(void);
void test_firebase_commit_rollback_invalid(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_begin_transaction_not_implemented(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBASE;
    Transaction* tx = NULL;
    TEST_ASSERT_FALSE(firebase_begin_transaction(&handle, DB_ISOLATION_READ_COMMITTED, &tx));
    TEST_ASSERT_NULL(tx);
    TEST_ASSERT_FALSE(firebase_begin_transaction(NULL, DB_ISOLATION_READ_COMMITTED, &tx));
}

void test_firebase_commit_rollback_invalid(void) {
    TEST_ASSERT_FALSE(firebase_commit_transaction(NULL, NULL));
    TEST_ASSERT_FALSE(firebase_rollback_transaction(NULL, NULL));
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBASE;
    Transaction tx = {0};
    TEST_ASSERT_FALSE(firebase_commit_transaction(&handle, &tx));
    TEST_ASSERT_FALSE(firebase_rollback_transaction(&handle, &tx));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_begin_transaction_not_implemented);
    RUN_TEST(test_firebase_commit_rollback_invalid);
    return UNITY_END();
}
