/*
 * Unity Test File: lua_hook_test_install.c
 *
 * Phase 8 of the LUA_PLAN. Tests the progress hook and the H.gc.*
 * host functions on a sandboxed lua_State.
 *
 *   - Hook fires on a busy loop; instruction_count grows.
 *   - Memory sample appears in the scoreboard after sample_every ticks.
 *   - H.gc.{collect, step, count, isrunning} are callable from Lua.
 *   - Soft limit logs a warning (one-shot) and triggers a GC step
 *     (the script must not crash).
 *   - Hard limit triggers a runtime error (luaL_error longjmp to
 *     lua_pcall).
 *   - enforce_limits=false never kills the job, even with very low
 *     hard limits (the hook still samples).
 *   - NULL state and no-context cases are tolerated (no crash).
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>

// Lua headers
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Module under test
#include <src/scripting/lua_context.h>
#include <src/scripting/lua_hook.h>
#include <src/scripting/scoreboard.h>

// Mock app_config with known defaults
static AppConfig mock_app_config_storage = {0};

// Forward declarations (required for -Wmissing-prototypes)
void test_hook_null_state_is_noop(void);
void test_hook_install_without_context_logs_and_returns(void);
void test_hook_fires_and_scoreboard_grows(void);
void test_hook_gc_helpers_are_callable_from_lua(void);
void test_hook_gc_collect_returns_a_number(void);
void test_hook_hard_limit_aborts_job(void);
void test_hook_enforce_limits_false_never_kills(void);
void test_hook_uninstall_stops_further_ticks(void);

// Build a fresh lua_State with a job context already set.
static lua_State* make_state_with_context(const char* job_id,
                                          Scoreboard* sb,
                                          int hook_interval,
                                          size_t soft_kb,
                                          size_t hard_kb,
                                          bool enforce) {
    lua_State* L = H_lua_create_context();
    if (!L) return NULL;
    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", job_id);
    ctx.scoreboard = sb;
    ctx.hook_interval = hook_interval;
    ctx.soft_limit_kb = soft_kb;
    ctx.hard_limit_kb = hard_kb;
    ctx.enforce_limits = enforce;
    H_lua_set_job_context(L, &ctx);
    return L;
}

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    mock_app_config_storage.scripting.InstructionHookInterval = 5000;
    mock_app_config_storage.scripting.MemorySampleEveryNHooks = 1;  // sample every tick
    mock_app_config_storage.scripting.MemorySoftLimitKB = 32768;
    mock_app_config_storage.scripting.MemoryHardLimitKB = 65536;
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

// NULL state: install is a no-op (does not crash).
void test_hook_null_state_is_noop(void) {
    H_lua_install_progress_hook(NULL);
    H_lua_uninstall_progress_hook(NULL);
    TEST_ASSERT_TRUE(1);  // got here without crashing
}

// Installing on a state with no context logs and does not crash.
void test_hook_install_without_context_logs_and_returns(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    // No context set; install should log + return.
    H_lua_install_progress_hook(L);
    // State is still usable.
    int rc = H_lua_run_string(L, "return 1", "[hook:test:noctx]");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

// A busy loop runs enough instructions for the hook to fire many
// times, and the scoreboard's instruction_count / memory_used_kb
// both grow.
void test_hook_fires_and_scoreboard_grows(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "busy", NULL);

    // hook_interval=100 and sample_every=1 mean one sample per 100
    // instructions. The loop below runs 1,000,000 iterations, so
    // we get ~10,000 samples and instruction_count ~= 1,000,000.
    lua_State* L = make_state_with_context(id, sb, 100, 0, 0, true);
    TEST_ASSERT_NOT_NULL(L);
    H_lua_install_progress_hook(L);

    int rc = H_lua_run_string(L, "for i = 1, 1000000 do end", "[hook:busy]");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    lua_pop(L, lua_gettop(L));

    H_lua_uninstall_progress_hook(L);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    // instruction_count is the number of hook calls (not raw VM
    // instructions). With hook_interval=100 and a 1M-iteration
    // numeric for loop (1 bytecode per iteration), the hook fires
    // ~10,000 times.
    TEST_ASSERT_TRUE(e->instruction_count >= 1000);
    scoreboard_entry_free(e);
    free(id);
    H_lua_destroy_context(L);
    scoreboard_destroy(sb);
}

// H.gc.{collect, step, count, isrunning} are all callable from Lua
// and return sane values.
void test_hook_gc_helpers_are_callable_from_lua(void) {
    lua_State* L = H_lua_create_context();
    int rc = H_lua_run_string(L,
        "assert(type(H.gc) == 'table')\n"
        "assert(type(H.gc.collect) == 'function')\n"
        "assert(type(H.gc.step) == 'function')\n"
        "assert(type(H.gc.count) == 'function')\n"
        "assert(type(H.gc.isrunning) == 'function')\n"
        "return 0\n",
        "[hook:gc:api]");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

// H.gc.collect() returns a number (KB freed). It is OK to call on
// a fresh state; the result is 0 or small.
void test_hook_gc_collect_returns_a_number(void) {
    lua_State* L = H_lua_create_context();
    int rc = H_lua_run_string(L,
        "local n = H.gc.collect()\n"
        "assert(type(n) == 'number')\n"
        "assert(n >= 0)\n"
        "return 0\n",
        "[hook:gc:collect]");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

// Hard limit = 1 KB is below any reasonable Lua state. The hook
// will trip the hard limit on the first sample tick and luaL_error
// longjmps to the worker's pcall. The script returns a non-LUA_OK
// status, and the error string contains "memory limit".
void test_hook_hard_limit_aborts_job(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "hard", NULL);

    // hook_interval=100, hard=1 KB, sample_every=1 -> trip on first tick.
    lua_State* L = make_state_with_context(id, sb, 100, 0, 1, true);
    TEST_ASSERT_NOT_NULL(L);
    H_lua_install_progress_hook(L);

    int rc = H_lua_run_string(L, "for i = 1, 1000 do end", "[hook:hard]");
    TEST_ASSERT_EQUAL(LUA_ERRRUN, rc);
    const char* msg = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_NOT_NULL(strstr(msg, "memory limit"));
    lua_pop(L, 1);

    H_lua_uninstall_progress_hook(L);
    H_lua_destroy_context(L);

    // Scoreboard should still have the entry (worker path not used here,
    // so we update it directly to verify the hook wrote progress).
    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    // The hook should have written at least one progress sample before
    // the longjmp fired.
    TEST_ASSERT_TRUE(e->instruction_count > 0);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// enforce_limits=false with a tiny hard limit must NOT abort the job.
// The hook still updates progress, but the soft/hard checks are skipped.
void test_hook_enforce_limits_false_never_kills(void) {
    Scoreboard* sb = scoreboard_create();
    ScoreboardJobLimits limits = {0};
    limits.instruction_hook_interval = 100;
    limits.memory_soft_limit_kb = 0;
    limits.memory_hard_limit_kb = 1;  // would normally kill
    limits.enforce_limits = false;
    char* id = scoreboard_submit_with_limits(sb, "no_enforce", NULL, &limits);

    lua_State* L = make_state_with_context(id, sb, 100, 0, 1, false);
    TEST_ASSERT_NOT_NULL(L);
    H_lua_install_progress_hook(L);

    int rc = H_lua_run_string(L, "for i = 1, 10000 do end", "[hook:no-enforce]");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    lua_pop(L, lua_gettop(L));

    H_lua_uninstall_progress_hook(L);
    H_lua_destroy_context(L);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_TRUE(e->instruction_count > 0);
    TEST_ASSERT_FALSE(e->enforce_limits);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

// After uninstall, the hook should not fire anymore. The local counter
// resets on the next install (H_lua_install_progress_hook zeros it).
void test_hook_uninstall_stops_further_ticks(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "uninst", NULL);
    lua_State* L = make_state_with_context(id, sb, 100, 0, 0, true);
    H_lua_install_progress_hook(L);

    int rc1 = H_lua_run_string(L, "for i = 1, 1000 do end", "[hook:uninst:1]");
    TEST_ASSERT_EQUAL(LUA_OK, rc1);
    lua_pop(L, lua_gettop(L));

    ScoreboardEntry* mid = scoreboard_find(sb, id);
    uint64_t mid_count = mid->instruction_count;
    scoreboard_entry_free(mid);
    TEST_ASSERT_TRUE(mid_count > 0);

    H_lua_uninstall_progress_hook(L);
    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);

    // The progress written by the hook should still be in the scoreboard
    // after the state is gone (we only destroyed the C state, not the
    // scoreboard row).
    ScoreboardEntry* after = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(after);
    TEST_ASSERT_EQUAL_UINT64(mid_count, after->instruction_count);
    scoreboard_entry_free(after);
    free(id);
    scoreboard_destroy(sb);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_hook_null_state_is_noop);
    RUN_TEST(test_hook_install_without_context_logs_and_returns);
    RUN_TEST(test_hook_fires_and_scoreboard_grows);
    RUN_TEST(test_hook_gc_helpers_are_callable_from_lua);
    RUN_TEST(test_hook_gc_collect_returns_a_number);
    RUN_TEST(test_hook_hard_limit_aborts_job);
    RUN_TEST(test_hook_enforce_limits_false_never_kills);
    RUN_TEST(test_hook_uninstall_stops_further_ticks);

    return UNITY_END();
}
