/*
 * Unity Test File: Chat Proxy - Completed Stream Cleanup
 * Tests chat_proxy_cleanup_completed_streams() and chat_proxy_get_multi_manager()
 * in proxy.c.
 *
 * active_streams is a file-scope static list populated only by the streaming
 * request path (which spawns real CURL worker threads). At process start it is
 * NULL, so cleanup is a safe no-op; we verify that and the singleton accessor.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/proxy.h>

// Test function prototypes
void test_cleanup_completed_streams_empty(void);
void test_get_multi_manager(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

void test_cleanup_completed_streams_empty(void) {
    // With an empty (NULL) active_streams list, this must be safe and return.
    chat_proxy_cleanup_completed_streams();
    chat_proxy_cleanup_completed_streams();
    TEST_ASSERT_TRUE(1); // Reached -> no crash
}

void test_get_multi_manager(void) {
    MultiStreamManager* mgr = chat_proxy_get_multi_manager();
    TEST_ASSERT_NOT_NULL(mgr);
    // The singleton is stable across calls
    TEST_ASSERT_EQUAL_PTR(mgr, chat_proxy_get_multi_manager());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_cleanup_completed_streams_empty);
    RUN_TEST(test_get_multi_manager);

    return UNITY_END();
}
