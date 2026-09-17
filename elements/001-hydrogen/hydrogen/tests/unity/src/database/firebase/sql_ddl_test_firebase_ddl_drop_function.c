/*
 * Unity tests for firebase_ddl_drop_function().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebase/query.h>
#include <src/database/firebase/sql_ddl.h>

void test_firebase_ddl_drop_function_known(void);
void test_firebase_ddl_drop_function_if_exists(void);
void test_firebase_ddl_drop_function_unknown(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_ddl_drop_function_known(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBASE;
    QueryRequest request = {0};
    request.sql_template = (char*)"DROP FUNCTION FB_BROTLI_DECOMPRESS";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(&handle, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_firebase_ddl_drop_function_if_exists(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBASE;
    QueryRequest request = {0};
    request.sql_template = (char*)"DROP FUNCTION IF EXISTS brotli_decompress;";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(&handle, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_firebase_ddl_drop_function_unknown(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBASE;
    QueryRequest request = {0};
    request.sql_template = (char*)"DROP FUNCTION mystery_udf";
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(&handle, &request, &result));
    TEST_ASSERT_FALSE(result->success);
    free(result->error_message);
    free(result->data_json);
    free(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_ddl_drop_function_known);
    RUN_TEST(test_firebase_ddl_drop_function_if_exists);
    RUN_TEST(test_firebase_ddl_drop_function_unknown);
    return UNITY_END();
}
