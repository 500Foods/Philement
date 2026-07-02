/*
 * Unity Test File: orchestrator_test_shutdown.c
 *
 * Phase 11d of the LUA_PLAN. Validates the Orchestrator's
 * shutdown path: a long-running orchestrator's loop sees the
 * shutdown flag, exits, the pthread joins, and the lua_State is
 * destroyed.
 *
 *   - A long-running in-memory script that logs its iteration
 *     count. The test logs the mock to confirm the loop ran at
 *     least once before destroy.
 *   - destroy() flips the shutdown flag, the script's H.sleep
 *     wakes within ~100 ms, the loop exits, the script returns.
 *   - destroy() joins the pthread within the test's bound
 *     (no pthread_join deadlock).
 *   - After destroy, scripting_orchestrator_state is NULL and
 *     is_running() returns false.
 *   - A second start_after_destroy succeeds (the module is
 *     reusable).
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Modules under test
#include <src/scripting/scripting.h>
#include <src/scripting/orchestrator.h>

// Mock log helpers
#include "mock_logging.h"

// Forward declarations (required for -Wmissing-prototypes)
void test_shutdown_wakes_long_running_orchestrator(void);
void test_shutdown_joins_pthread_within_bound(void);
void test_start_again_after_destroy(void);

// A long-running orchestrator. Each iteration logs a count line
// so the test can confirm the loop is alive; the loop polls
// H.shutdown_requested() (H.sleep returns early on shutdown so
// the loop wakes within ~100 ms of the flag being set).
static const char* LONG_RUNNING_SCRIPT =
    "local i = 0\n"
    "while not H.shutdown_requested() do\n"
    "  i = i + 1\n"
    "  H.log.info('orchestrator-shutdown-test: tick %d', i)\n"
    "  H.sleep(50)\n"
    "end\n"
    "H.log.info('orchestrator-shutdown-test: stopped after %d ticks', i)\n";

void setUp(void) {
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
    memset(&scripting_threads, 0, sizeof(scripting_threads));
    mock_logging_reset_all();
}

void tearDown(void) {
    if (scripting_orchestrator_is_running()) {
        scripting_orchestrator_destroy();
    }
    scripting_orchestrator_state = NULL;
    scripting_system_shutdown = 0;
    memset(&scripting_threads, 0, sizeof(scripting_threads));
}

void test_shutdown_wakes_long_running_orchestrator(void) {
    int rc = scripting_orchestrator_start_with_source(
        LONG_RUNNING_SCRIPT, "orchestrator_shutdown_test");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_TRUE(scripting_orchestrator_is_running());

    // Wait until the loop has run at least one iteration
    // (look for any 'tick' message in the mock log).
    bool saw_tick = false;
    for (int i = 0; i < 100; i++) {  // up to ~500 ms
        const char* last = mock_logging_get_last_message();
        if (last && strstr(last, "tick") != NULL) {
            saw_tick = true;
            break;
        }
        usleep(5000);
    }
    TEST_ASSERT_TRUE_MESSAGE(saw_tick, "orchestrator did not iterate before destroy");

    // destroy must wake the long H.sleep within ~100 ms and
    // join the pthread. The bound is generous to absorb CI jitter.
    scripting_orchestrator_destroy();
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
    TEST_ASSERT_NULL(scripting_orchestrator_state);
}

void test_shutdown_joins_pthread_within_bound(void) {
    int rc = scripting_orchestrator_start_with_source(
        LONG_RUNNING_SCRIPT, "orchestrator_join_test");
    TEST_ASSERT_EQUAL(1, rc);

    // Let the loop run a few iterations
    for (int i = 0; i < 20; i++) {
        usleep(5000);
    }

    // Use a coarse time bound on the destroy itself: it must
    // return promptly (H.sleep wakes the script within 100 ms,
    // then the pthread exits and joins).
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    scripting_orchestrator_destroy();
    clock_gettime(CLOCK_MONOTONIC, &t1);

    long elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000L
                    + (t1.tv_nsec - t0.tv_nsec) / 1000000L;
    // 2000 ms is the hard upper bound; the script's H.sleep(50)
    // wakes in <= 100 ms and the join should be near-instant.
    TEST_ASSERT_LESS_THAN(2000, elapsed_ms);
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());
}

void test_start_again_after_destroy(void) {
    int rc1 = scripting_orchestrator_start_with_source(
        LONG_RUNNING_SCRIPT, "orchestrator_reuse_first");
    TEST_ASSERT_EQUAL(1, rc1);
    scripting_orchestrator_destroy();
    TEST_ASSERT_FALSE(scripting_orchestrator_is_running());

    // After destroy, a fresh start must succeed (the module is
    // reset, not one-shot).
    int rc2 = scripting_orchestrator_start_with_source(
        LONG_RUNNING_SCRIPT, "orchestrator_reuse_second");
    TEST_ASSERT_EQUAL(1, rc2);
    TEST_ASSERT_TRUE(scripting_orchestrator_is_running());
    scripting_orchestrator_destroy();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_shutdown_wakes_long_running_orchestrator);
    RUN_TEST(test_shutdown_joins_pthread_within_bound);
    RUN_TEST(test_start_again_after_destroy);

    return UNITY_END();
}
