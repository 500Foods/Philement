/*
 * Unity Test File: lua_hook_test_kill.c
 *
 * Phase 10 of the LUA_PLAN. Tests the per-tick kill checks in
 * the progress hook:
 *   - max_runtime_seconds trips the kill when the deadline passes
 *     (via the snapshotted started_at + clock_gettime comparison).
 *   - kill_requested (set on the scoreboard, read live on every
 *     tick) trips the kill when the next tick fires.
 *   - Both checks are gated by enforce_limits: the Orchestrator
 *     (enforce=false) is never killed by either.
 *   - INT_MAX / 0 max_runtime_seconds means "no limit"; the hook
 *     does not raise.
 *   - The kill error string is left on the Lua stack for the
 *     caller to consume (same contract as the hard memory limit).
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>
#include <limits.h>
#include <time.h>

// Lua headers
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Module under test
#include <src/scripting/lua_context.h>
#include <src/scripting/lua_hook.h>
#include <src/scripting/scoreboard.h>

// Mock app_config with known defaults. The memory fields don't
// matter for these tests; we just need the submit path to succeed.
static AppConfig mock_app_config_storage = {0};

// Forward declarations (required for -Wmissing-prototypes)
void test_hook_kill_max_runtime_trips_after_deadline(void);
void test_hook_kill_requested_flag_trips_immediately(void);
void test_hook_kill_enforce_false_never_kills(void);
void test_hook_kill_int_max_means_no_limit(void);
void test_hook_kill_zero_max_runtime_means_no_limit(void);
void test_hook_kill_error_string_is_on_stack(void);

// Build a fresh lua_State with a job context already set. The
// started_at is set to the value passed in so we can simulate "this
// job has been running for N seconds already".
static lua_State* make_state_with_context(Scoreboard* sb,
                                          const char* job_id,
                                          int hook_interval,
                                          int max_runtime_seconds,
                                          struct timespec started_at,
                                          bool enforce) {
    lua_State* L = H_lua_create_context();
    if (!L) return NULL;
    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", job_id);
    ctx.scoreboard = sb;
    ctx.hook_interval = hook_interval;
    ctx.soft_limit_kb = 0;
    ctx.hard_limit_kb = 0;
    ctx.enforce_limits = enforce;
    ctx.max_runtime_seconds = max_runtime_seconds;
    ctx.started_at = started_at;
    H_lua_set_job_context(L, &ctx);
    return L;
}

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    mock_app_config_storage.scripting.InstructionHookInterval = 5000;
    mock_app_config_storage.scripting.MemorySampleEveryNHooks = 1;
    mock_app_config_storage.scripting.MemorySoftLimitKB = 32768;
    mock_app_config_storage.scripting.MemoryHardLimitKB = 65536;
    mock_app_config_storage.scripting.DefaultMaxRuntime = 60;
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

// A job whose started_at is "10 seconds ago" and whose
// max_runtime_seconds=5 must be killed on the first hook tick:
// the hook raises luaL_error and the worker's pcall returns
// LUA_ERRRUN with "max runtime exceeded" on the stack.
void test_hook_kill_max_runtime_trips_after_deadline(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "timeout", NULL);

    struct timespec started_at;
    clock_gettime(CLOCK_MONOTONIC, &started_at);
    started_at.tv_sec -= 10;  // pretend we started 10s ago

    // hook_interval=100: the hook fires every 100 VM instructions,
    // so the busy loop below is guaranteed to trip on its first tick.
    lua_State* L = make_state_with_context(sb, id, 100, 5, started_at, true);
    TEST_ASSERT_NOT_NULL(L);
    H_lua_install_progress_hook(L);

    int rc = H_lua_run_string(L, "for i = 1, 1000 do end", "[hook:kill:rt]");
    TEST_ASSERT_EQUAL(LUA_ERRRUN, rc);
    const char* msg = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_NOT_NULL(strstr(msg, "max runtime"));
    lua_pop(L, 1);

    H_lua_uninstall_progress_hook(L);
    H_lua_destroy_context(L);
    free(id);
    scoreboard_destroy(sb);
}

// A job whose kill_requested is set externally is killed on the
// next tick (within a few VM instructions). The error message
// identifies the kill as "requested" so operators can tell it
// apart from a runtime error.
void test_hook_kill_requested_flag_trips_immediately(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "extkill", NULL);

    // Set the kill flag BEFORE the script starts.
    TEST_ASSERT_TRUE(scoreboard_request_kill(sb, id));

    struct timespec started_at;
    clock_gettime(CLOCK_MONOTONIC, &started_at);

    lua_State* L = make_state_with_context(sb, id, 100, 3600, started_at, true);
    TEST_ASSERT_NOT_NULL(L);
    H_lua_install_progress_hook(L);

    int rc = H_lua_run_string(L, "for i = 1, 1000 do end", "[hook:kill:ext]");
    TEST_ASSERT_EQUAL(LUA_ERRRUN, rc);
    const char* msg = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_NOT_NULL(strstr(msg, "killed: requested"));
    lua_pop(L, 1);

    H_lua_uninstall_progress_hook(L);
    H_lua_destroy_context(L);
    free(id);
    scoreboard_destroy(sb);
}

// enforce_limits=false opts out of both kill paths. The hook
// still fires and still samples, but it never trips the kill.
void test_hook_kill_enforce_false_never_kills(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "noenf", NULL);

    struct timespec started_at;
    clock_gettime(CLOCK_MONOTONIC, &started_at);
    started_at.tv_sec -= 100;  // way past any reasonable deadline

    // Externally request kill + set a tight max_runtime. With
    // enforce_limits=false, neither should trip.
    TEST_ASSERT_TRUE(scoreboard_request_kill(sb, id));

    lua_State* L = make_state_with_context(sb, id, 100, 1, started_at, false);
    TEST_ASSERT_NOT_NULL(L);
    H_lua_install_progress_hook(L);

    int rc = H_lua_run_string(L, "for i = 1, 1000 do end", "[hook:kill:noop]");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    lua_pop(L, lua_gettop(L));

    H_lua_uninstall_progress_hook(L);
    H_lua_destroy_context(L);
    free(id);
    scoreboard_destroy(sb);
}

// INT_MAX means "no limit" - the hook never trips on time, even
// with a fake "started a million years ago" started_at.
void test_hook_kill_int_max_means_no_limit(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "norlim", NULL);

    struct timespec started_at;
    clock_gettime(CLOCK_MONOTONIC, &started_at);
    started_at.tv_sec -= 1000000;  // absurdly old

    lua_State* L = make_state_with_context(sb, id, 100, INT_MAX, started_at, true);
    TEST_ASSERT_NOT_NULL(L);
    H_lua_install_progress_hook(L);

    int rc = H_lua_run_string(L, "for i = 1, 1000 do end", "[hook:kill:no-rt]");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    lua_pop(L, lua_gettop(L));

    H_lua_uninstall_progress_hook(L);
    H_lua_destroy_context(L);
    free(id);
    scoreboard_destroy(sb);
}

// 0 max_runtime_seconds also means "no limit" (the field default
// before resolve_from_config fills it; the hook treats 0 as
// "off" so a half-initialised context never trips spuriously).
void test_hook_kill_zero_max_runtime_means_no_limit(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "zrt", NULL);

    struct timespec started_at;
    clock_gettime(CLOCK_MONOTONIC, &started_at);
    started_at.tv_sec -= 10000;

    lua_State* L = make_state_with_context(sb, id, 100, 0, started_at, true);
    TEST_ASSERT_NOT_NULL(L);
    H_lua_install_progress_hook(L);

    int rc = H_lua_run_string(L, "for i = 1, 1000 do end", "[hook:kill:zero]");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    lua_pop(L, lua_gettop(L));

    H_lua_uninstall_progress_hook(L);
    H_lua_destroy_context(L);
    free(id);
    scoreboard_destroy(sb);
}

// The kill error string is left on the stack and contains the
// job_id, so operators can grep for it. The job_id is auto-
// generated by the scoreboard, so we capture it from the submit
// and check the error string against that value.
void test_hook_kill_error_string_is_on_stack(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "name_only", NULL);
    TEST_ASSERT_NOT_NULL(id);
    TEST_ASSERT_TRUE(scoreboard_request_kill(sb, id));

    struct timespec started_at;
    clock_gettime(CLOCK_MONOTONIC, &started_at);

    lua_State* L = make_state_with_context(sb, id, 100, 3600, started_at, true);
    TEST_ASSERT_NOT_NULL(L);
    H_lua_install_progress_hook(L);

    int rc = H_lua_run_string(L, "for i = 1, 1000 do end", "[hook:kill:id]");
    TEST_ASSERT_EQUAL(LUA_ERRRUN, rc);
    const char* msg = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(msg);
    // The error mentions the actual job_id (auto-generated), so
    // an operator can grep for the job and find the kill reason.
    TEST_ASSERT_NOT_NULL(strstr(msg, id));
    lua_pop(L, 1);

    H_lua_uninstall_progress_hook(L);
    H_lua_destroy_context(L);
    free(id);
    scoreboard_destroy(sb);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_hook_kill_max_runtime_trips_after_deadline);
    RUN_TEST(test_hook_kill_requested_flag_trips_immediately);
    RUN_TEST(test_hook_kill_enforce_false_never_kills);
    RUN_TEST(test_hook_kill_int_max_means_no_limit);
    RUN_TEST(test_hook_kill_zero_max_runtime_means_no_limit);
    RUN_TEST(test_hook_kill_error_string_is_on_stack);

    return UNITY_END();
}
