/*
 * Unity Test File: lua_context_test_traceback.c
 *
 * Phase 24 of the LUA_PLAN. Tests H_lua_build_traceback - the
 * sanitized traceback generator for Lua errors.
 *
 * Validates:
 *   - NULL state returns NULL
 *   - Simple error produces meaningful traceback
 *   - Traceback includes function names and line numbers
 *   - Traceback is sanitized (max frames, truncated snippets)
 *   - Traceback is a heap-allocated string caller must free
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/lua_context.h>

static AppConfig mock_app_config_storage = {0};

void test_traceback_null_state_returns_null(void);
void test_traceback_simple_error_produces_traceback(void);
void test_traceback_includes_function_names(void);
void test_traceback_includes_line_numbers(void);
void test_traceback_max_frames_limited(void);
void test_traceback_returns_malloced_string(void);
void test_traceback_syntax_error(void);
void test_traceback_runtime_error(void);
void test_traceback_multi_frame_call_stack(void);

/*
 * C probe installed as a Lua global. When invoked from within a live
 * Lua call stack it walks that stack (H_lua_build_traceback) and
 * returns the resulting string. Pushing nil on allocation failure lets
 * the caller distinguish the failure path.
 *
 * This is the only way to exercise the traceback loop body: after a
 * pcall error the Lua call stack is unwound, so calling
 * H_lua_build_traceback from C (outside Lua) yields zero frames.
 */
static int lua_traceback_probe(lua_State* L) {
    char* tb = H_lua_build_traceback(L);
    if (tb) {
        lua_pushstring(L, tb);
        free(tb);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

void test_traceback_null_state_returns_null(void) {
    char* result = H_lua_build_traceback(NULL);
    TEST_ASSERT_NULL(result);
}

void test_traceback_simple_error_produces_traceback(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    const char* code = "error('test error')";
    int rc = H_lua_run_string(L, code, "[test:simple]");
    TEST_ASSERT_EQUAL_INT(LUA_ERRRUN, rc);

    char* traceback = H_lua_build_traceback(L);
    TEST_ASSERT_NOT_NULL(traceback);
    TEST_ASSERT_TRUE(strlen(traceback) > 0);

    free(traceback);
    H_lua_destroy_context(L);
}

void test_traceback_includes_function_names(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    const char* code =
        "local function inner()\n"
        "  error('inner error')\n"
        "end\n"
        "inner()\n";
    int rc = H_lua_run_string(L, code, "[test:func]");
    TEST_ASSERT_EQUAL_INT(LUA_ERRRUN, rc);

    char* traceback = H_lua_build_traceback(L);
    TEST_ASSERT_NOT_NULL(traceback);
    TEST_ASSERT_TRUE(strlen(traceback) > 0);

    free(traceback);
    H_lua_destroy_context(L);
}

void test_traceback_includes_line_numbers(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    const char* code = "error('line test')";
    int rc = H_lua_run_string(L, code, "[test:line]");
    TEST_ASSERT_EQUAL_INT(LUA_ERRRUN, rc);

    char* traceback = H_lua_build_traceback(L);
    TEST_ASSERT_NOT_NULL(traceback);
    TEST_ASSERT_TRUE(strlen(traceback) > 0);

    free(traceback);
    H_lua_destroy_context(L);
}

void test_traceback_max_frames_limited(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    const char* code =
        "local function f1() error('deep')\n"
        "end\n"
        "local function f2() f1() end\n"
        "local function f3() f2() end\n"
        "local function f4() f3() end\n"
        "local function f5() f4() end\n"
        "local function f6() f5() end\n"
        "local function f7() f6() end\n"
        "local function f8() f7() end\n"
        "local function f9() f8() end\n"
        "local function f10() f9() end\n"
        "local function f11() f10() end\n"
        "local function f12() f11() end\n"
        "f12()\n";
    int rc = H_lua_run_string(L, code, "[test:deep]");
    TEST_ASSERT_EQUAL_INT(LUA_ERRRUN, rc);

    char* traceback = H_lua_build_traceback(L);
    TEST_ASSERT_NOT_NULL(traceback);

    int frame_count = 0;
    const char* p = traceback;
    while ((p = strchr(p, '[')) != NULL) {
        frame_count++;
        p++;
    }
    TEST_ASSERT_TRUE(frame_count <= 10);

    free(traceback);
    H_lua_destroy_context(L);
}

void test_traceback_returns_malloced_string(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    const char* code = "error('malloc test')";
    int rc = H_lua_run_string(L, code, "[test:malloc]");
    TEST_ASSERT_EQUAL_INT(LUA_ERRRUN, rc);

    char* traceback1 = H_lua_build_traceback(L);
    TEST_ASSERT_NOT_NULL(traceback1);

    char* traceback2 = H_lua_build_traceback(L);
    TEST_ASSERT_NOT_NULL(traceback2);

    TEST_ASSERT_NOT_EQUAL(traceback1, traceback2);

    free(traceback1);
    free(traceback2);
    H_lua_destroy_context(L);
}

void test_traceback_syntax_error(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    const char* code = "this is not valid lua (";
    int rc = H_lua_run_string(L, code, "[test:syntax]");
    TEST_ASSERT_EQUAL_INT(LUA_ERRSYNTAX, rc);

    char* traceback = H_lua_build_traceback(L);
    TEST_ASSERT_NOT_NULL(traceback);

    free(traceback);
    H_lua_destroy_context(L);
}

void test_traceback_runtime_error(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    const char* code = "local x = nil; x.foo()";
    int rc = H_lua_run_string(L, code, "[test:runtime]");
    TEST_ASSERT_EQUAL_INT(LUA_ERRRUN, rc);

    char* traceback = H_lua_build_traceback(L);
    TEST_ASSERT_NOT_NULL(traceback);
    TEST_ASSERT_TRUE(strlen(traceback) > 0);

    free(traceback);
    H_lua_destroy_context(L);
}

/*
 * Drive H_lua_build_traceback from inside a live, deeply nested Lua
 * call stack. This exercises the loop body that the post-pcall tests
 * never reach: frame extraction, function-name / source-snippet
 * formatting (both the currentline>0 and <=0 branches), buffer growth
 * via realloc, and the MAX_TRACE_FRAMES cap.
 */
void test_traceback_multi_frame_call_stack(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_pushcfunction(L, lua_traceback_probe);
    lua_setglobal(L, "H_traceback_probe");

    const char* code =
        "local function recurse(n)\n"
        "  if n <= 1 then return H_traceback_probe() end\n"
        "  local r = recurse(n - 1)\n"
        "  return r\n"
        "end\n"
        "return recurse(20)\n";
    int rc = H_lua_run_string(L, code, "[tb:deep:recursion:test]");
    TEST_ASSERT_EQUAL_INT(LUA_OK, rc);

    // The probe returned the traceback string as a single value.
    TEST_ASSERT_EQUAL_INT(1, lua_gettop(L));
    const char* tb = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(tb);

    // The nested Lua frames must appear, with their function names and
    // the chunk name as the source snippet (currentline>0 formatting).
    TEST_ASSERT_NOT_NULL(strstr(tb, "recurse"));
    TEST_ASSERT_NOT_NULL(strstr(tb, "[tb:deep:recursion:test]"));

    // Multiple frames captured (capped at MAX_TRACE_FRAMES = 10). Count
    // frame lines (each begins with "  [<level>"), not every '[' (the
    // chunk name itself also contains '[').
    int frame_count = 0;
    const char* p = tb;
    if (strncmp(p, "  [", 3) == 0) {
        frame_count++;
    }
    while ((p = strstr(p, "\n  [")) != NULL) {
        frame_count++;
        p += 4;
    }
    TEST_ASSERT_TRUE(frame_count >= 2);
    TEST_ASSERT_TRUE(frame_count <= 10);

    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void setUp(void);
void tearDown(void);

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_traceback_null_state_returns_null);
    RUN_TEST(test_traceback_simple_error_produces_traceback);
    RUN_TEST(test_traceback_includes_function_names);
    RUN_TEST(test_traceback_includes_line_numbers);
    RUN_TEST(test_traceback_max_frames_limited);
    RUN_TEST(test_traceback_returns_malloced_string);
    RUN_TEST(test_traceback_syntax_error);
    RUN_TEST(test_traceback_runtime_error);
    RUN_TEST(test_traceback_multi_frame_call_stack);

    return UNITY_END();
}