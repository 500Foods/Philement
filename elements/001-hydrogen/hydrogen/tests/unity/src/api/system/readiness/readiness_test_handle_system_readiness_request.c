/*
 * Unity Test File: System Readiness handle_system_readiness_request Tests
 * This file contains unit tests for handle_system_readiness_request in
 * src/api/system/readiness/readiness.c
 *
 * CHANGELOG:
 * 2026-07-18: Initial implementation
 *
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks BEFORE including source headers.
// USE_MOCK_LIBMICROHTTPD is already active globally for this test target;
// USE_MOCK_DATABASE_ENGINE lets us inject a DatabaseReadiness snapshot.
#define USE_MOCK_DATABASE_ENGINE
#include <unity/mocks/mock_database_engine.h>
#include <unity/mocks/mock_libmicrohttpd.h>

// Include necessary headers for the module being tested
#include <src/api/system/readiness/readiness.h>
#include <src/database/database.h>

// Forward declarations for test functions
void test_handle_system_readiness_ready(void);
void test_handle_system_readiness_starting_no_databases(void);
void test_handle_system_readiness_starting_only(void);
void test_handle_system_readiness_started_only(void);
void test_handle_system_readiness_starting_and_started(void);
void test_handle_system_readiness_orchestrator_flag_ready(void);

// Captured MHD_queue_response status code (real api_send_json_response runs,
// we just intercept the dispatched HTTP status).
static unsigned int g_captured_status = 0;
static bool g_queue_called = false;

// Override the weak mock MHD_queue_response to record the HTTP status.
enum MHD_Result MHD_queue_response(struct MHD_Connection *connection,
                                   unsigned int status_code,
                                   struct MHD_Response *response) {
    (void)connection; (void)response;
    g_captured_status = status_code;
    g_queue_called = true;
    return MHD_YES;
}

// Helper: build a snapshot with the given started/starting names.
static DatabaseReadiness build_snapshot(int expected, int started_total,
                                       const char *started_names[],
                                       int starting_total,
                                       const char *starting_names[]) {
    DatabaseReadiness r = {0};
    r.expected = expected;
    r.started = started_total;
    r.count = started_total + starting_total;
    int idx = 0;
    for (int i = 0; i < started_total && idx < DATABASE_READINESS_MAX; i++, idx++) {
        r.entries[idx].ready = true;
        strncpy(r.entries[idx].name, started_names[i], sizeof(r.entries[idx].name) - 1);
    }
    for (int i = 0; i < starting_total && idx < DATABASE_READINESS_MAX; i++, idx++) {
        r.entries[idx].ready = false;
        strncpy(r.entries[idx].name, starting_names[i], sizeof(r.entries[idx].name) - 1);
    }
    return r;
}

void setUp(void) {
    g_captured_status = 0;
    g_queue_called = false;
    // Start from a not-ready global flag for each test.
    server_ready = 0;
    mock_database_get_readiness_reset();
    mock_mhd_reset_all();
}

void tearDown(void) {
    server_ready = 0;
    mock_database_get_readiness_reset();
}

// ready=true via global server_ready flag -> HTTP 200, status "ready"
void test_handle_system_readiness_ready(void) {
    server_ready = 1;
    DatabaseReadiness snap = build_snapshot(2, 2, (const char*[]){"Demo_PG", "Demo_SQL"}, 0, NULL);
    mock_database_set_readiness(&snap, true);

    struct MHD_Connection *conn = (struct MHD_Connection *)0xDEADBEEF;
    enum MHD_Result result = handle_system_readiness_request(conn);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_TRUE(g_queue_called);
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, g_captured_status);
}

// No databases configured but not yet ready -> expected==0 branch, "starting"
void test_handle_system_readiness_starting_no_databases(void) {
    DatabaseReadiness snap = build_snapshot(0, 0, NULL, 0, NULL);
    mock_database_set_readiness(&snap, false);

    struct MHD_Connection *conn = (struct MHD_Connection *)0xDEADBEEF;
    enum MHD_Result result = handle_system_readiness_request(conn);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_SERVICE_UNAVAILABLE, g_captured_status);
}

// Only starting databases -> "starting <names>" (line 86-87)
void test_handle_system_readiness_starting_only(void) {
    DatabaseReadiness snap = build_snapshot(2, 0, NULL, 2,
        (const char*[]){"Demo_PG", "Demo_SQL"});
    mock_database_set_readiness(&snap, false);

    struct MHD_Connection *conn = (struct MHD_Connection *)0xDEADBEEF;
    enum MHD_Result result = handle_system_readiness_request(conn);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_SERVICE_UNAVAILABLE, g_captured_status);
}

// Only started databases -> status "starting" (else branch, line 89)
void test_handle_system_readiness_started_only(void) {
    DatabaseReadiness snap = build_snapshot(2, 2,
        (const char*[]){"Demo_PG", "Demo_SQL"}, 0, NULL);
    mock_database_set_readiness(&snap, false);

    struct MHD_Connection *conn = (struct MHD_Connection *)0xDEADBEEF;
    enum MHD_Result result = handle_system_readiness_request(conn);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_SERVICE_UNAVAILABLE, g_captured_status);
}

// Both starting and started -> "starting %s; started %s" (line 84-85,
// exercises the starting_names/started_names concatenation loops 61-72)
void test_handle_system_readiness_starting_and_started(void) {
    DatabaseReadiness snap = build_snapshot(3, 1,
        (const char*[]){"Demo_PG"}, 2,
        (const char*[]){"Demo_SQL", "Demo_My"});
    mock_database_set_readiness(&snap, false);

    struct MHD_Connection *conn = (struct MHD_Connection *)0xDEADBEEF;
    enum MHD_Result result = handle_system_readiness_request(conn);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_SERVICE_UNAVAILABLE, g_captured_status);
}

// ready via db_ready && expected>0 (no global flag) -> HTTP 200
void test_handle_system_readiness_orchestrator_flag_ready(void) {
    server_ready = 0;
    DatabaseReadiness snap = build_snapshot(2, 2,
        (const char*[]){"Demo_PG", "Demo_SQL"}, 0, NULL);
    mock_database_set_readiness(&snap, true);

    struct MHD_Connection *conn = (struct MHD_Connection *)0xDEADBEEF;
    enum MHD_Result result = handle_system_readiness_request(conn);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, g_captured_status);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_system_readiness_ready);
    RUN_TEST(test_handle_system_readiness_starting_no_databases);
    RUN_TEST(test_handle_system_readiness_starting_only);
    RUN_TEST(test_handle_system_readiness_started_only);
    RUN_TEST(test_handle_system_readiness_starting_and_started);
    RUN_TEST(test_handle_system_readiness_orchestrator_flag_ready);

    return UNITY_END();
}
