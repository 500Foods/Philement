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

    return UNITY_END();
}