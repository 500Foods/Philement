#include <src/hydrogen.h>
#include <unity.h>
#include <src/mcp/mcp_session.h>
#include <src/mcp/mcp_stats.h>

void test_mcp_session_generate_and_bind(void);
void test_mcp_session_hijack_other_sub(void);
void test_mcp_session_unknown_404(void);
void test_mcp_session_delete(void);
void test_mcp_session_reaper(void);
void test_mcp_session_max_sessions(void);
void test_mcp_session_initialize_unknown_creates(void);
void test_mcp_session_reap_keeps_fresh(void);
void test_mcp_session_not_ready_paths(void);
void test_mcp_session_free_entry_null(void);
void test_mcp_session_delete_second(void);
void test_mcp_session_unknown_empty_id(void);

void setUp(void) {
    mcp_stats_reset();
    mcp_session_shutdown();
    mcp_session_init();
    mcp_session_set_now(1000);
}

void tearDown(void) {
    mcp_session_shutdown();
    mcp_session_clear_now();
    mcp_stats_reset();
}

void test_mcp_session_generate_and_bind(void) {
    char *id = NULL;
    char *again = NULL;
    McpSessionResult st;

    st = mcp_session_resolve(NULL, "user-a", true, 8, 900, &id);
    TEST_ASSERT_EQUAL(MCP_SESSION_CREATED, st);
    TEST_ASSERT_NOT_NULL(id);
    TEST_ASSERT_EQUAL(1, mcp_session_count());
    st = mcp_session_resolve(id, "user-a", false, 8, 900, &again);
    TEST_ASSERT_EQUAL(MCP_SESSION_OK, st);
    TEST_ASSERT_EQUAL_STRING(id, again);
    free(id);
    free(again);
}

void test_mcp_session_hijack_other_sub(void) {
    char *id = NULL;
    TEST_ASSERT_EQUAL(MCP_SESSION_CREATED, mcp_session_resolve(NULL, "user-a", true, 8, 900, &id));
    TEST_ASSERT_EQUAL(MCP_SESSION_HIJACK, mcp_session_resolve(id, "user-b", false, 8, 900, NULL));
    free(id);
}

void test_mcp_session_unknown_404(void) {
    TEST_ASSERT_EQUAL(MCP_SESSION_UNKNOWN,
                      mcp_session_resolve("missing-id", "user-a", false, 8, 900, NULL));
    TEST_ASSERT_EQUAL(0, mcp_session_count());
}

void test_mcp_session_delete(void) {
    char *id = NULL;
    TEST_ASSERT_EQUAL(MCP_SESSION_CREATED, mcp_session_resolve(NULL, "user-a", true, 8, 900, &id));
    TEST_ASSERT_EQUAL(MCP_SESSION_HIJACK, mcp_session_delete(id, "user-b"));
    TEST_ASSERT_EQUAL(1, mcp_session_count());
    TEST_ASSERT_EQUAL(MCP_SESSION_DELETED, mcp_session_delete(id, "user-a"));
    TEST_ASSERT_EQUAL(0, mcp_session_count());
    TEST_ASSERT_EQUAL(MCP_SESSION_UNKNOWN, mcp_session_delete(id, "user-a"));
    TEST_ASSERT_EQUAL(MCP_SESSION_UNKNOWN, mcp_session_delete(NULL, "user-a"));
    free(id);
}

void test_mcp_session_reaper(void) {
    char *id = NULL;
    McpMetrics snap;

    TEST_ASSERT_EQUAL(MCP_SESSION_CREATED, mcp_session_resolve(NULL, "user-a", true, 8, 900, &id));
    mcp_session_set_now(1000 + 901);
    TEST_ASSERT_EQUAL(1, mcp_session_reap(900));
    TEST_ASSERT_EQUAL(0, mcp_session_count());
    mcp_collect_metrics(&snap);
    TEST_ASSERT_EQUAL_UINT64(1, snap.sessions_expired);
    TEST_ASSERT_EQUAL(MCP_SESSION_UNKNOWN, mcp_session_resolve(id, "user-a", false, 8, 900, NULL));
    free(id);
}

void test_mcp_session_max_sessions(void) {
    char *first = NULL;
    char *second = NULL;
    TEST_ASSERT_EQUAL(MCP_SESSION_CREATED, mcp_session_resolve(NULL, "user-a", true, 1, 900, &first));
    TEST_ASSERT_EQUAL(MCP_SESSION_LIMIT, mcp_session_resolve(NULL, "user-b", true, 1, 900, &second));
    TEST_ASSERT_NULL(second);
    TEST_ASSERT_EQUAL(1, mcp_session_count());
    free(first);
}

void test_mcp_session_initialize_unknown_creates(void) {
    char *id = NULL;
    TEST_ASSERT_EQUAL(MCP_SESSION_CREATED,
                      mcp_session_resolve("stale-client-id", "user-a", true, 8, 900, &id));
    TEST_ASSERT_NOT_NULL(id);
    TEST_ASSERT_TRUE(strcmp(id, "stale-client-id") != 0);
    TEST_ASSERT_EQUAL(1, mcp_session_count());
    free(id);
}

void test_mcp_session_reap_keeps_fresh(void) {
    char *old_id = NULL;
    char *fresh_id = NULL;

    TEST_ASSERT_EQUAL(MCP_SESSION_CREATED, mcp_session_resolve(NULL, "old", true, 8, 900, &old_id));
    mcp_session_set_now(1000 + 500);
    TEST_ASSERT_EQUAL(MCP_SESSION_CREATED, mcp_session_resolve(NULL, "fresh", true, 8, 900, &fresh_id));
    mcp_session_set_now(1000 + 901);
    TEST_ASSERT_EQUAL(1, mcp_session_reap(900));
    TEST_ASSERT_EQUAL(1, mcp_session_count());
    TEST_ASSERT_EQUAL(MCP_SESSION_OK, mcp_session_resolve(fresh_id, "fresh", false, 8, 900, NULL));
    TEST_ASSERT_EQUAL(0, mcp_session_reap(0));
    free(old_id);
    free(fresh_id);
}

void test_mcp_session_not_ready_paths(void) {
    mcp_session_shutdown();
    TEST_ASSERT_EQUAL(0, mcp_session_count());
    TEST_ASSERT_EQUAL(0, mcp_session_reap(900));
    mcp_session_shutdown();
    mcp_session_init();
    mcp_session_init();
    TEST_ASSERT_EQUAL(0, mcp_session_count());
}

void test_mcp_session_free_entry_null(void) {
    mcp_session_free_entry(NULL);
    TEST_ASSERT_NULL(mcp_session_find_locked(NULL));
}

void test_mcp_session_delete_second(void) {
    char *first = NULL;
    char *second = NULL;

    TEST_ASSERT_EQUAL(MCP_SESSION_CREATED, mcp_session_resolve(NULL, "a", true, 8, 900, &first));
    TEST_ASSERT_EQUAL(MCP_SESSION_CREATED, mcp_session_resolve(NULL, "b", true, 8, 900, &second));
    TEST_ASSERT_EQUAL(MCP_SESSION_DELETED, mcp_session_delete(first, "a"));
    TEST_ASSERT_EQUAL(1, mcp_session_count());
    TEST_ASSERT_EQUAL(MCP_SESSION_DELETED, mcp_session_delete(second, "b"));
    free(first);
    free(second);
}

void test_mcp_session_unknown_empty_id(void) {
    TEST_ASSERT_EQUAL(MCP_SESSION_UNKNOWN, mcp_session_resolve(NULL, "user-a", false, 8, 900, NULL));
    TEST_ASSERT_EQUAL(MCP_SESSION_UNKNOWN, mcp_session_resolve("", "user-a", false, 8, 900, NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mcp_session_generate_and_bind);
    RUN_TEST(test_mcp_session_hijack_other_sub);
    RUN_TEST(test_mcp_session_unknown_404);
    RUN_TEST(test_mcp_session_delete);
    RUN_TEST(test_mcp_session_reaper);
    RUN_TEST(test_mcp_session_max_sessions);
    RUN_TEST(test_mcp_session_initialize_unknown_creates);
    RUN_TEST(test_mcp_session_reap_keeps_fresh);
    RUN_TEST(test_mcp_session_not_ready_paths);
    RUN_TEST(test_mcp_session_free_entry_null);
    RUN_TEST(test_mcp_session_delete_second);
    RUN_TEST(test_mcp_session_unknown_empty_id);
    return UNITY_END();
}
