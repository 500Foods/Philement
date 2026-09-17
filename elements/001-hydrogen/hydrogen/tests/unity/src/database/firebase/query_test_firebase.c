/*
 * Unity tests for Phase 3 Firebase query stubs.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebase/query.h>

void test_firebase_execute_query_invalid(void);
void test_firebase_execute_query_not_implemented(void);
void test_firebase_execute_prepared_not_implemented(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_execute_query_invalid(void) {
    TEST_ASSERT_FALSE(firebase_execute_query(NULL, NULL, NULL));
}

void test_firebase_execute_query_not_implemented(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBASE;
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(&handle, &request, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL(DB_ERR_OTHER, result->error_class);
    TEST_ASSERT_NOT_NULL(result->error_message);
    free(result->error_message);
    free(result);
}

void test_firebase_execute_prepared_not_implemented(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBASE;
    QueryRequest request = {0};
    PreparedStatement stmt = {0};
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_prepared(&handle, &stmt, &request, &result));
    TEST_ASSERT_NOT_NULL(result);
    free(result->error_message);
    free(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_execute_query_invalid);
    RUN_TEST(test_firebase_execute_query_not_implemented);
    RUN_TEST(test_firebase_execute_prepared_not_implemented);
    return UNITY_END();
}
