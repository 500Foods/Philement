/*
 * Unity Test File: scripting_api_test_set_current_state.c
 *
 * Phase 9 of the LUA_PLAN. Tests H.set_current_state - the voluntary
 * progress-report function exposed to Lua scripts.
 *
 * The function bridges Lua to the scoreboard:
 *
 *   H.set_current_state("Loading customer data")
 *
 *   - Reads the per-state job context to find the running job's
 *     job_id and scoreboard.
 *   - Copies the Lua string out of Lua memory (UAF discipline from
 *     Phase 1).
 *   - Calls scoreboard_update_current_state, which strdup's its own
 *     copy and frees any prior value.
 *
 * Validates:
 *   - H.set_current_state is installed as a top-level function on H
 *     (replacing the Phase 3 placeholder sub-table).
 *   - Direct C-side call from a Lua chunk updates the scoreboard
 *     when a job context is set.
 *   - End-to-end via the worker pool: a submitted job that calls
 *     H.set_current_state is reflected in the scoreboard entry.
 *   - No-op when no job context is set (the Orchestrator case; a bare
 *     lua_State with no extraspace context). Does not crash, does not
 *     log a warning, does not raise.
 *   - Non-string first argument: logs an error, does not raise.
 *   - Missing argument: logs an error, does not raise.
 *   - The function returns no values (the script is not required to
 *     consume a return value).
 *   - Multiple updates overwrite the prior value.
 *   - Empty string clears the field.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>
#include <time.h>

// Lua headers
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Modules under test
#include <src/scripting/lua_context.h>
#include <src/scripting/scripting_api.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/scripting.h>
#include <src/scripting/worker_pool.h>
#include <src/threads/threads.h>

// Mock logging introspection. H.set_current_state failure paths log
// via log_this; we assert on the mock's "last call" API.
#include <tests/unity/mocks/mock_logging.h>

// Mock app_config (zeroed = no special defaults needed for this suite).
static AppConfig mock_app_config_storage = {0};

// Forward declarations (required for -Wmissing-prototypes)
void test_set_current_state_is_a_function_on_H(void);
void test_set_current_state_direct_call_updates_scoreboard(void);
void test_set_current_state_no_context_is_noop(void);
void test_set_current_state_non_string_arg_logs_and_does_not_raise(void);
void test_set_current_state_missing_arg_logs_and_does_not_raise(void);
void test_set_current_state_returns_no_values(void);
void test_set_current_state_multiple_calls_overwrite(void);
void test_set_current_state_empty_string_clears_field(void);
void test_set_current_state_end_to_end_through_worker_pool(void);
void test_set_current_state_string_survives_lua_gc(void);

// Poll the scoreboard until the job reaches a terminal status. The
// same shape used by worker_pool_test_execute.c / _progress.c.
#define POLL_TIMEOUT_MS 5000
#define POLL_SLEEP_USEC 1000
static ScoreboardJobStatus wait_for_terminal(const char* job_id,
                                             ScoreboardEntry** out_entry) {
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (1) {
        ScoreboardEntry* e = scoreboard_find(scripting_scoreboard, job_id);
        if (e) {
            if (e->status == SCOREBOARD_JOB_COMPLETED
                || e->status == SCOREBOARD_JOB_FAILED
                || e->status == SCOREBOARD_JOB_KILLED) {
                ScoreboardJobStatus s = e->status;
                if (out_entry) {
                    *out_entry = e;
                } else {
                    scoreboard_entry_free(e);
                }
                return s;
            }
            scoreboard_entry_free(e);
        }
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000
                        + (now.tv_nsec - start.tv_nsec) / 1000000;
        if (elapsed_ms >= POLL_TIMEOUT_MS) {
            TEST_FAIL_MESSAGE("Timed out waiting for job to reach terminal status");
        }
        usleep(POLL_SLEEP_USEC);
    }
}

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
    mock_logging_reset_all();
    scripting_init_state();
}

void tearDown(void) {
    scripting_workers_destroy();
    scripting_cleanup_state();
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
    app_config = NULL;
}

// H.set_current_state is a function, not a table. Phase 9 replaces
// the Phase 3 placeholder sub-table.
void test_set_current_state_is_a_function_on_H(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "set_current_state");
    TEST_ASSERT_TRUE_MESSAGE(lua_isfunction(L, -1),
        "H.set_current_state must be a function");
    lua_pop(L, 2);

    H_lua_destroy_context(L);
}

// Direct C-side call: build a state with a job context, run a Lua
// chunk that calls H.set_current_state, find the entry, assert the
// field is set.
void test_set_current_state_direct_call_updates_scoreboard(void) {
    Scoreboard* sb = scoreboard_create();
    TEST_ASSERT_NOT_NULL(sb);
    char* id = scoreboard_submit(sb, "direct_set", NULL);
    TEST_ASSERT_NOT_NULL(id);

    // Build a lua_State with a real job context pointing at the
    // scoreboard. This mirrors what the worker does.
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    int rc = H_lua_run_string(L,
        "H.set_current_state('Loading customer data')",
        "[phase9:direct]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc,
        "H.set_current_state from a chunk should succeed");

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->current_state);
    TEST_ASSERT_EQUAL_STRING("Loading customer data", e->current_state);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// The Orchestrator's lua_State (or a bare test state) has no job
// context. H.set_current_state must be a clean no-op: no crash, no
// log, no raise, no scoreboard side effect.
void test_set_current_state_no_context_is_noop(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    // Deliberately do NOT set a job context.

    int rc = H_lua_run_string(L,
        "H.set_current_state('orchestrator running')",
        "[phase9:noctx]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc,
        "no-context call must not raise a Lua error");

    H_lua_destroy_context(L);
}

// A non-string first argument must not raise a Lua error. The host
// path is a leaf, in keeping with the Phase 6 rule.
void test_set_current_state_non_string_arg_logs_and_does_not_raise(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "set_current_state");
    lua_newtable(L); // {} is not a string
    int rc = lua_pcall(L, 1, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc,
        "non-string first arg must not raise a Lua error");

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "must be a string"));

    H_lua_destroy_context(L);
}

void test_set_current_state_missing_arg_logs_and_does_not_raise(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "set_current_state");
    int rc = lua_pcall(L, 0, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc,
        "missing arg must not raise a Lua error");

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "missing string argument"));

    H_lua_destroy_context(L);
}

// The function returns no values. A script that calls it without
// assignment must not leave anything on the stack. We use a chunk that
// captures select('#', ...) before and after and returns the count.
void test_set_current_state_returns_no_values(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_run_string(L,
        "local before = select('#', ...)\n"
        "H.set_current_state('hi')\n"
        "local after = select('#', ...)\n"
        "return before, after\n",
        "[phase9:return-count]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "return-count chunk should run");
    // Two return values: both 0 (no varargs were ever passed).
    int n_results = lua_gettop(L);
    TEST_ASSERT_EQUAL_INT(2, n_results);
    int before_val = (int)lua_tointeger(L, -2);
    int after_val = (int)lua_tointeger(L, -1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, before_val, "vararg count before call");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, after_val, "vararg count after call");
    lua_pop(L, 2);

    H_lua_destroy_context(L);
}

// Multiple calls in the same job overwrite the prior value (the
// scoreboard update frees the old string before installing the new).
void test_set_current_state_multiple_calls_overwrite(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "multi", NULL);

    lua_State* L = H_lua_create_context();
    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    int rc = H_lua_run_string(L,
        "H.set_current_state('Day 1 of 300')\n"
        "H.set_current_state('Day 47 of 300')\n"
        "H.set_current_state('Day 300 of 300')\n",
        "[phase9:multi]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "multi-call chunk should run");

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->current_state);
    TEST_ASSERT_EQUAL_STRING("Day 300 of 300", e->current_state);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// An empty string is treated as "clear the field" (the C side
// converts it to NULL inside scoreboard_update_current_state).
void test_set_current_state_empty_string_clears_field(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "empty", NULL);
    scoreboard_update_current_state(sb, id, "initial");

    lua_State* L = H_lua_create_context();
    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    int rc = H_lua_run_string(L, "H.set_current_state('')\n",
                              "[phase9:empty]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "empty-string call should run");

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NULL(e->current_state);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// End-to-end via the worker pool: a submitted job that calls
// H.set_current_state leaves a non-NULL current_state on its
// scoreboard entry when the job completes.
void test_set_current_state_end_to_end_through_worker_pool(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    char* id = scripting_submit_job_with_source("phase9_worker",
        "H.set_current_state('Step 1')\n"
        "H.set_current_state('Step 2')\n"
        "H.set_current_state('Step 3 - done')\n"
        "return 0\n",
        NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, s);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->current_state);
    TEST_ASSERT_EQUAL_STRING("Step 3 - done", e->current_state);
    scoreboard_entry_free(e);
    free(id);
}

// The string is copied to C-owned memory on the C side (UAF
// discipline from Phase 1). A script that builds a string, reports
// it, and then immediately drops the only Lua reference must still
// see the same value in the scoreboard.
void test_set_current_state_string_survives_lua_gc(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "gc_test", NULL);

    lua_State* L = H_lua_create_context();
    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    int rc = H_lua_run_string(L,
        // Build a string in a tight scope so it goes out of scope
        // before we look at the scoreboard.
        "do\n"
        "  local s = string.rep('A', 256) .. '-tail'\n"
        "  H.set_current_state(s)\n"
        "end\n"
        "collectgarbage('collect')\n",
        "[phase9:gc]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "gc-survival chunk should run");

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->current_state);
    // We don't assert exact length because the format is
    // implementation-defined for the test, only the tail.
    TEST_ASSERT_NOT_NULL(strstr(e->current_state, "-tail"));
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_set_current_state_is_a_function_on_H);
    RUN_TEST(test_set_current_state_direct_call_updates_scoreboard);
    RUN_TEST(test_set_current_state_no_context_is_noop);
    RUN_TEST(test_set_current_state_non_string_arg_logs_and_does_not_raise);
    RUN_TEST(test_set_current_state_missing_arg_logs_and_does_not_raise);
    RUN_TEST(test_set_current_state_returns_no_values);
    RUN_TEST(test_set_current_state_multiple_calls_overwrite);
    RUN_TEST(test_set_current_state_empty_string_clears_field);
    RUN_TEST(test_set_current_state_end_to_end_through_worker_pool);
    RUN_TEST(test_set_current_state_string_survives_lua_gc);

    return UNITY_END();
}
