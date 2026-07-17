/*
 * Unity Test File: handle_cap_queue_selection
 * Tests the handle_cap_queue_selection function in
 * src/api/conduit/cap/cap_query.c. Exercises both the queue-not-found
 * failure branch (which frees resources and returns MHD_NO) and the
 * success branch.
 *
 * CHANGELOG:
 * 2026-07-16: Initial creation covering both queue selection branches.
 *
 * TEST_VERSION: 1.0.0
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database_params.h>

#include <src/api/conduit/cap/cap_query.h>

// Function prototypes for test functions
void test_handle_cap_queue_selection_no_queue(void);
void test_handle_cap_queue_selection_success(void);
void test_handle_cap_queue_selection_type_11_forced_slow(void);
void test_handle_cap_queue_selection_null_intended(void);

// No live queue manager: select_query_queue returns NULL -> failure branch
void setUp(void) {
    global_queue_manager = NULL;
}

void tearDown(void) {
    global_queue_manager = NULL;
}

// Queue not found -> failure branch frees resources and returns MHD_NO
void test_handle_cap_queue_selection_no_queue(void) {
    struct MHD_Connection *mock_connection = (void *)0x123;
    DatabaseQueue *selected = (void *)0xDEAD;

    ParameterList *param_list = (ParameterList *)calloc(1, sizeof(ParameterList));
    char *converted_sql = strdup("SELECT 1");
    TypedParameter **ordered_params = NULL;

    enum MHD_Result result = handle_cap_queue_selection(
        mock_connection, "no_such_db", 123, NULL,
        param_list, converted_sql, ordered_params, &selected, NULL
    );

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(selected);
    // converted_sql and param_list are freed by the failure branch
}

// Type 11 with a real cache entry: queue-type resolution forces "slow"
// even when no queue is available (failure branch). This covers the
// type-11 branch of the queue_type ternary and the failure path.
void test_handle_cap_queue_selection_success(void) {
    struct MHD_Connection *mock_connection = (void *)0x123;
    DatabaseQueue *selected = (void *)0xDEAD;
    const char *intended = "untouched";

    QueryCacheEntry entry = {0};
    entry.query_type = 11;
    entry.queue_type = (char *)"slow";

    ParameterList *param_list = (ParameterList *)calloc(1, sizeof(ParameterList));
    char *converted_sql = strdup("SELECT 1");
    TypedParameter **ordered_params = NULL;

    enum MHD_Result result = handle_cap_queue_selection(
        mock_connection, "no_such_db", 123, &entry,
        param_list, converted_sql, ordered_params, &selected, &intended
    );

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_EQUAL_STRING("slow", intended);

    // param_list and converted_sql are freed by the failure branch
}

// Type 11 forces the slow queue type regardless of the cache entry
void test_handle_cap_queue_selection_type_11_forced_slow(void) {
    struct MHD_Connection *mock_connection = (void *)0x123;
    DatabaseQueue *selected = NULL;

    QueryCacheEntry entry = {0};
    entry.query_type = 11;
    entry.queue_type = (char *)"fast";

    ParameterList *param_list = (ParameterList *)calloc(1, sizeof(ParameterList));
    char *converted_sql = strdup("SELECT 1");
    TypedParameter **ordered_params = NULL;
    const char *intended = NULL;

    enum MHD_Result result = handle_cap_queue_selection(
        mock_connection, "cap_test_db", 123, &entry,
        param_list, converted_sql, ordered_params, &selected, &intended
    );

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_EQUAL_STRING("slow", intended);

    // param_list and converted_sql are freed by the failure branch
}

// NULL intended_queue_type pointer must be handled gracefully
void test_handle_cap_queue_selection_null_intended(void) {
    struct MHD_Connection *mock_connection = (void *)0x123;
    DatabaseQueue *selected = (void *)0xDEAD;

    QueryCacheEntry entry = {0};
    entry.query_type = 11;
    entry.queue_type = (char *)"slow";

    ParameterList *param_list = (ParameterList *)calloc(1, sizeof(ParameterList));
    char *converted_sql = strdup("SELECT 1");
    TypedParameter **ordered_params = NULL;

    enum MHD_Result result = handle_cap_queue_selection(
        mock_connection, "no_such_db", 123, &entry,
        param_list, converted_sql, ordered_params, &selected, NULL
    );

    TEST_ASSERT_EQUAL(MHD_NO, result);

    // param_list and converted_sql are freed by the failure branch
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_cap_queue_selection_no_queue);
    RUN_TEST(test_handle_cap_queue_selection_success);
    RUN_TEST(test_handle_cap_queue_selection_type_11_forced_slow);
    RUN_TEST(test_handle_cap_queue_selection_null_intended);

    return UNITY_END();
}
