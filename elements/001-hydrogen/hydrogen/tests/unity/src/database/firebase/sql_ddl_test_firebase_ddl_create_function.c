/*
 * Unity tests for firebase_ddl_create_function().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebase/query.h>
#include <src/database/firebase/sql_parse.h>
#include <src/database/firebase/sql_ddl.h>

void test_firebase_ddl_create_function_known(void);
void test_firebase_ddl_create_function_unknown(void);
void test_firebase_ddl_create_function_null(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_ddl_create_function_null(void) {
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_ddl_create_function(NULL, NULL, &result));
    TEST_ASSERT_FALSE(result->success);
    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_firebase_ddl_create_function_known(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBASE;
    QueryRequest request = {0};
    request.sql_template = (char*)"CREATE FUNCTION FB_JSON_INGEST (x text)";
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_execute_query(&handle, &request, &result));
    TEST_ASSERT_TRUE(result->success);
    free(result->error_message);
    free(result->data_json);
    free(result);
}

void test_firebase_ddl_create_function_unknown(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBASE;
    QueryRequest request = {0};
    request.sql_template = (char*)"CREATE FUNCTION mystery_udf (x text)";
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_execute_query(&handle, &request, &result));
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "unknown function"));
    free(result->error_message);
    free(result->data_json);
    free(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_ddl_create_function_null);
    RUN_TEST(test_firebase_ddl_create_function_known);
    RUN_TEST(test_firebase_ddl_create_function_unknown);
    return UNITY_END();
}
