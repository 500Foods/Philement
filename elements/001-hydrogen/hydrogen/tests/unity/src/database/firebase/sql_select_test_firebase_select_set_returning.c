/*
 * Unity tests for firebase_select_set_returning().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebase/sql_select.h>
#include <jansson.h>

void test_firebase_select_set_returning(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_select_set_returning(void) {
    json_t* fields = json_loads(
        "{\"query_id\":{\"integerValue\":\"3\"},\"name\":{\"stringValue\":\"n\"}}", 0, NULL);
    char* cols[] = {(char*)"query_id", (char*)"missing"};
    QueryResult* result = NULL;
    TEST_ASSERT_TRUE(firebase_select_set_returning(&result, 1, fields, cols, 2));
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(1, result->row_count);
    TEST_ASSERT_EQUAL(2, result->column_count);
    TEST_ASSERT_NOT_NULL(strstr(result->data_json, "\"query_id\":3"));
    TEST_ASSERT_NOT_NULL(strstr(result->data_json, "null"));
    database_engine_cleanup_result(result);
    json_decref(fields);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_select_set_returning);
    return UNITY_END();
}
