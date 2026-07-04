/*
 * Unity Test File: submit_test_await_result_affected_rows
 *
 * Phase 14 of the LUA_PLAN. Tests that database_queue_await_result
 * propagates the engine's affected_rows from QueryResult into the
 * returned DatabaseQuery. This is the plumbing path that makes
 * H.wait -> res.affected_rows work for atomic task claiming.
 *
 * Per the plan: the await path is the seam between the engine
 * layer (which already populates QueryResult.affected_rows on all
 * 4 engines) and the scripting/conduit layer (which surfaces it).
 * Before Phase 14, this seam dropped affected_rows on the floor.
 * After Phase 14, the seam preserves it end-to-end.
 *
 * The Unity test build compiles submit.c with USE_MOCK_DBQUEUE
 * defined (per the build configuration), so the
 * `database_queue_await_result` symbol is the
 * `mock_database_queue_await_result` from the mock infrastructure.
 * The test sets the desired DatabaseQuery on the mock, calls
 * await_result, and asserts on the returned struct's fields.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>
#include <stdlib.h>

// Project includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>
#include <src/database/database_pending.h>

// Test infrastructure
#define USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_await_result_propagates_affected_rows_for_insert(void);
void test_await_result_propagates_affected_rows_for_update_one(void);
void test_await_result_propagates_affected_rows_for_update_zero(void);
void test_await_result_propagates_affected_rows_for_select(void);
void test_await_result_affected_rows_zero_when_query_result_null(void);
void test_await_result_propagates_data_json_alongside_affected_rows(void);
void test_await_result_propagates_error_message_alongside_affected_rows(void);

void setUp(void) {
    mock_dbqueue_reset_all();
}

void tearDown(void) {
    mock_dbqueue_reset_all();
}

// A non-NULL placeholder DatabaseQueue for resolve_db_queue. The
// real function never dereferences it under the mock.
static DatabaseQueue placeholder_dbq_storage = {0};
static DatabaseQueue* placeholder_dbq = &placeholder_dbq_storage;

// Build a DatabaseQuery that the mock will return. The mock
// deep-copies the strdup'd fields.
static DatabaseQuery make_dbq(int affected_rows, const char* data_json, const char* error_message) {
    DatabaseQuery q = {0};
    q.query_id = (char*)"mock_await_qid";
    q.processed_at = 0;
    q.retry_count = 0;
    q.affected_rows = affected_rows;
    if (data_json) q.query_template = (char*)data_json;
    if (error_message) q.error_message = (char*)error_message;
    return q;
}

// INSERT that affected 1 row: affected_rows=1
void test_await_result_propagates_affected_rows_for_insert(void) {
    DatabaseQuery template = make_dbq(1, NULL, NULL);
    mock_dbqueue_set_await_result(&template);

    DatabaseQuery* result = database_queue_await_result(placeholder_dbq, "await_ar_insert_1", 5);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, result->affected_rows);
    TEST_ASSERT_NULL(result->error_message);
    TEST_ASSERT_NULL(result->query_template);
    free(result->query_id);
    free(result->query_template);
    free(result->error_message);
    free(result);
}

// UPDATE that matched one row: affected_rows=1
void test_await_result_propagates_affected_rows_for_update_one(void) {
    DatabaseQuery template = make_dbq(1, NULL, NULL);
    mock_dbqueue_set_await_result(&template);

    DatabaseQuery* result = database_queue_await_result(placeholder_dbq, "await_ar_upd_one", 5);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, result->affected_rows);
    free(result->query_id);
    free(result->query_template);
    free(result->error_message);
    free(result);
}

// UPDATE that matched no row: affected_rows=0
// (This is the loser's view in an atomic task-claim race.)
void test_await_result_propagates_affected_rows_for_update_zero(void) {
    DatabaseQuery template = make_dbq(0, NULL, NULL);
    mock_dbqueue_set_await_result(&template);

    DatabaseQuery* result = database_queue_await_result(placeholder_dbq, "await_ar_upd_zero", 5);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(0, result->affected_rows);
    free(result->query_id);
    free(result->query_template);
    free(result->error_message);
    free(result);
}

// SELECT: query_template has data_json, affected_rows=0 (the engine
// reports 0 for reads; this is what a worker sees on a
// H.query("SELECT ...") call)
void test_await_result_propagates_affected_rows_for_select(void) {
    DatabaseQuery template = make_dbq(0, "[{\"id\":1}]", NULL);
    mock_dbqueue_set_await_result(&template);

    DatabaseQuery* result = database_queue_await_result(placeholder_dbq, "await_ar_select", 5);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(0, result->affected_rows);
    TEST_ASSERT_NOT_NULL(result->query_template);
    TEST_ASSERT_EQUAL_STRING("[{\"id\":1}]", result->query_template);
    free(result->query_id);
    free(result->query_template);
    free(result->error_message);
    free(result);
}

// When the mock returns NULL, await_result returns NULL and the
// caller sees no result. (This is the "engine failure or timeout"
// path.)
void test_await_result_affected_rows_zero_when_query_result_null(void) {
    mock_dbqueue_set_await_result(NULL);

    DatabaseQuery* result = database_queue_await_result(placeholder_dbq, "await_ar_null", 5);
    TEST_ASSERT_NULL(result);
}

// data_json and affected_rows both propagate. This is the
// combined-success path the H.wait code exercises.
void test_await_result_propagates_data_json_alongside_affected_rows(void) {
    DatabaseQuery template = make_dbq(3, "[{\"x\":1}]", NULL);
    mock_dbqueue_set_await_result(&template);

    DatabaseQuery* result = database_queue_await_result(placeholder_dbq, "await_ar_combined", 5);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(3, result->affected_rows);
    TEST_ASSERT_NOT_NULL(result->query_template);
    TEST_ASSERT_EQUAL_STRING("[{\"x\":1}]", result->query_template);
    free(result->query_id);
    free(result->query_template);
    free(result->error_message);
    free(result);
}

// error_message propagates alongside affected_rows=0.
void test_await_result_propagates_error_message_alongside_affected_rows(void) {
    DatabaseQuery template = make_dbq(0, NULL, "syntax error at or near WHERE");
    mock_dbqueue_set_await_result(&template);

    DatabaseQuery* result = database_queue_await_result(placeholder_dbq, "await_ar_err", 5);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(0, result->affected_rows);
    TEST_ASSERT_NOT_NULL(result->error_message);
    TEST_ASSERT_EQUAL_STRING("syntax error at or near WHERE", result->error_message);
    free(result->query_id);
    free(result->query_template);
    free(result->error_message);
    free(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_await_result_propagates_affected_rows_for_insert);
    RUN_TEST(test_await_result_propagates_affected_rows_for_update_one);
    RUN_TEST(test_await_result_propagates_affected_rows_for_update_zero);
    RUN_TEST(test_await_result_propagates_affected_rows_for_select);
    RUN_TEST(test_await_result_affected_rows_zero_when_query_result_null);
    RUN_TEST(test_await_result_propagates_data_json_alongside_affected_rows);
    RUN_TEST(test_await_result_propagates_error_message_alongside_affected_rows);

    return UNITY_END();
}
