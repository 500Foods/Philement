/*
 * Unity Test File: submit_test_build_from_result
 *
 * Directly tests database_query_build_from_result() (submit.c), the helper
 * that converts a worker-produced QueryResult into the DatabaseQuery that
 * database_queue_await_result returns to its synchronous caller. It was
 * extracted from database_queue_await_result so it can be exercised without
 * the synchronous wait machinery (which cannot be driven reliably from a
 * unit test because the signalling producer thread is starved while the
 * caller blocks in pthread_cond_timedwait in this environment).
 *
 * Covers both the success branch (copying data_json -> query_template,
 * error_message, and affected_rows) and the NULL-result branch.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <stdlib.h>
#include <string.h>

// Project includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_build_from_result_success_full(void);
void test_build_from_result_null_result(void);
void test_build_from_result_data_only(void);
void test_build_from_result_error_only(void);
void test_build_from_result_null_query_id(void);

static QueryResult* make_result(const char* data_json, const char* error_message,
                                int affected_rows, size_t rows, size_t cols) {
    QueryResult* r = calloc(1, sizeof(QueryResult));
    if (!r) return NULL;
    r->success = true;
    r->data_json = data_json ? strdup(data_json) : NULL;
    r->error_message = error_message ? strdup(error_message) : NULL;
    r->affected_rows = affected_rows;
    r->row_count = rows;
    r->column_count = cols;
    r->execution_time_ms = 42;
    return r;
}

void setUp(void) {
    // No setup required
}

void tearDown(void) {
    // No teardown required
}

// Full success path: data_json, error_message and affected_rows all copied.
void test_build_from_result_success_full(void) {
    QueryResult* r = make_result("[{\"id\":1}]", "boom", 7, 3, 1);
    DatabaseQuery* q = database_query_build_from_result("q_full", r, "DQM-X-00-SMFC");
    TEST_ASSERT_NOT_NULL(q);
    TEST_ASSERT_EQUAL_STRING("q_full", q->query_id);
    TEST_ASSERT_EQUAL_STRING("[{\"id\":1}]", q->query_template);
    TEST_ASSERT_EQUAL_STRING("boom", q->error_message);
    TEST_ASSERT_EQUAL(7, q->affected_rows);

    free(q->query_id);
    free(q->query_template);
    free(q->error_message);
    free(q);
    free(r->data_json);
    free(r->error_message);
    free(r);
}

// NULL QueryResult: synthesized "Query execution failed or timed out" error.
void test_build_from_result_null_result(void) {
    DatabaseQuery* q = database_query_build_from_result("q_null", NULL, "DQM-X-00-SMFC");
    TEST_ASSERT_NOT_NULL(q);
    TEST_ASSERT_EQUAL_STRING("q_null", q->query_id);
    TEST_ASSERT_NULL(q->query_template);
    TEST_ASSERT_EQUAL(0, q->affected_rows);
    TEST_ASSERT_EQUAL_STRING("Query execution failed or timed out", q->error_message);

    free(q->query_id);
    free(q->query_template);
    free(q->error_message);
    free(q);
}

// QueryResult with data_json but no error_message (only query_template copied).
void test_build_from_result_data_only(void) {
    QueryResult* r = make_result("[1,2,3]", NULL, 0, 3, 1);
    DatabaseQuery* q = database_query_build_from_result("q_data", r, "DQM-X-00-SMFC");
    TEST_ASSERT_NOT_NULL(q);
    TEST_ASSERT_EQUAL_STRING("[1,2,3]", q->query_template);
    TEST_ASSERT_NULL(q->error_message);

    free(q->query_id);
    free(q->query_template);
    free(q->error_message);
    free(q);
    free(r->data_json);
    free(r);
}

// QueryResult with error_message but no data_json (only error_message copied).
void test_build_from_result_error_only(void) {
    QueryResult* r = make_result(NULL, "syntax error", 0, 0, 0);
    DatabaseQuery* q = database_query_build_from_result("q_err", r, "DQM-X-00-SMFC");
    TEST_ASSERT_NOT_NULL(q);
    TEST_ASSERT_NULL(q->query_template);
    TEST_ASSERT_EQUAL_STRING("syntax error", q->error_message);

    free(q->query_id);
    free(q->query_template);
    free(q->error_message);
    free(q);
    free(r->error_message);
    free(r);
}

// Defensive guard: NULL query_id yields NULL without dereferencing.
void test_build_from_result_null_query_id(void) {
    QueryResult* r = make_result("[1]", "e", 1, 1, 1);
    DatabaseQuery* q = database_query_build_from_result(NULL, r, "DQM-X-00-SMFC");
    TEST_ASSERT_NULL(q);

    free(r->data_json);
    free(r->error_message);
    free(r);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_build_from_result_success_full);
    RUN_TEST(test_build_from_result_null_result);
    RUN_TEST(test_build_from_result_data_only);
    RUN_TEST(test_build_from_result_error_only);
    RUN_TEST(test_build_from_result_null_query_id);

    return UNITY_END();
}
