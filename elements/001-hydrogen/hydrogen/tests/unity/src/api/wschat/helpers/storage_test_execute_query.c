/*
 * Unity Test File: chat_storage_execute_query
 * This file contains unit tests for chat_storage_execute_query()
 * in src/api/wschat/helpers/storage.c
 */

#include <src/hydrogen.h>
#include <unity.h>

/* Mocks for the database queue and engine layers */
#include <unity/mocks/mock_dbqueue.h>
#include <unity/mocks/mock_database_engine.h>

#include <src/api/wschat/helpers/storage.h>

/* Function prototype (declared in storage.c, exposed via header comment) */
bool chat_storage_execute_query(DatabaseQueue* db_queue, int query_ref,
                                const char* params_json, char** result_json);

/* Shared mock queue/handle used by the tests */
static DatabaseQueue* g_dbq = NULL;
static DatabaseHandle* g_handle = NULL;

void setUp(void) {
    mock_dbqueue_reset_all();
    mock_database_engine_reset_all();

    g_handle = calloc(1, sizeof(DatabaseHandle));
    g_dbq = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(g_handle);
    TEST_ASSERT_NOT_NULL(g_dbq);
    g_dbq->persistent_connection = g_handle;
    mock_dbqueue_set_get_database_result(g_dbq);
}

void tearDown(void) {
    free(g_dbq);
    free(g_handle);
    g_dbq = NULL;
    g_handle = NULL;
    mock_dbqueue_reset_all();
    mock_database_engine_reset_all();
}

/* NULL queue guard */
static void test_execute_query_null_queue(void) {
    char* out = NULL;
    bool result = chat_storage_execute_query(NULL, 62, "{}", &out);
    TEST_ASSERT_FALSE(result);
}

/* query_ref <= 0 guard */
static void test_execute_query_invalid_ref(void) {
    char* out = NULL;
    bool result = chat_storage_execute_query(g_dbq, 0, "{}", &out);
    TEST_ASSERT_FALSE(result);
}

/* engine returns success=false */
static void test_execute_query_engine_failure(void) {
    mock_database_engine_set_execute_result(false);
    char* out = NULL;
    bool result = chat_storage_execute_query(g_dbq, 62, "{}", &out);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(out);
}

/* engine returns success but result pointer is NULL */
static void test_execute_query_null_result_pointer(void) {
    mock_database_engine_set_execute_result(false);
    bool result = chat_storage_execute_query(g_dbq, 62, "{}", NULL);
    TEST_ASSERT_FALSE(result);
}

/* engine returns success but result->success is false */
static void test_execute_query_result_not_success(void) {
    /* Build a QueryResult where success == false */
    mock_database_engine_set_execute_result(true);
    /* The mock marks success based on set_execute_result, so to force
     * result->success false we use the error class path which returns
     * a result with success=false and a data_json. */
    mock_database_engine_set_execute_error_class(DB_ERR_TIMEOUT);
    char* out = NULL;
    bool result = chat_storage_execute_query(g_dbq, 62, "{}", &out);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(out);
}

/* success=true with data_json, result_json requested */
static void test_execute_query_success_with_data(void) {
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_execute_json_data("[{\"a\":1}]");
    char* out = NULL;
    bool result = chat_storage_execute_query(g_dbq, 62, "{\"k\":\"v\"}", &out);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("[{\"a\":1}]", out);
    free(out);
}

/* success=true with data_json, result_json pointer is NULL (no copy) */
static void test_execute_query_success_no_output_pointer(void) {
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_execute_json_data("[{\"a\":1}]");
    bool result = chat_storage_execute_query(g_dbq, 62, "{}", NULL);
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_execute_query_null_queue);
    RUN_TEST(test_execute_query_invalid_ref);
    RUN_TEST(test_execute_query_engine_failure);
    RUN_TEST(test_execute_query_null_result_pointer);
    RUN_TEST(test_execute_query_result_not_success);
    RUN_TEST(test_execute_query_success_with_data);
    RUN_TEST(test_execute_query_success_no_output_pointer);
    return UNITY_END();
}
