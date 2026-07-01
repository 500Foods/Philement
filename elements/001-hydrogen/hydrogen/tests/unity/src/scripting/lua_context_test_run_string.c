/*
 * Unity Test File: lua_context_test_run_string.c
 *
 * Phase 4 of the LUA_PLAN. Tests H_lua_run_string - the minimal wrapper
 * that compiles and executes a Lua chunk from a NUL-terminated string.
 *
 * Validates:
 *   - Successful script execution (sets a global, returns a value)
 *   - Multiple return values stay on the stack for the caller
 *   - Syntax error returns LUA_ERRSYNTAX and leaves an error string
 *   - Runtime error returns LUA_ERRRUN and leaves an error string
 *   - NULL state is rejected (LUA_ERRRUN, no crash)
 *   - NULL code is rejected (LUA_ERRRUN, no crash, error on stack)
 *   - NULL name falls back to "?" without error
 *   - Empty string is a valid (no-op) chunk
 *   - Return value can be consumed by the caller via lua_gettop/pop
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Lua headers
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Module under test
#include <src/scripting/lua_context.h>

// Mock app_config (zeroed = safest defaults for this test).
static AppConfig mock_app_config_storage = {0};

// Forward declarations
void test_run_string_executes_simple_script(void);
void test_run_string_returns_values_on_stack(void);
void test_run_string_returns_multret_values(void);
void test_run_string_syntax_error_returns_erryntax(void);
void test_run_string_runtime_error_returns_errrun(void);
void test_run_string_null_state_returns_errrun(void);
void test_run_string_null_code_returns_errrun(void);
void test_run_string_null_name_uses_default(void);
void test_run_string_empty_string_is_valid(void);
void test_run_string_sets_global_state(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

// Successful script that just sets a global. No return values.
void test_run_string_executes_simple_script(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    const char* code = "phase4_simple = 42";
    int rc = H_lua_run_string(L, code, "[test:simple]");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, "simple script should succeed");
    TEST_ASSERT_EQUAL_INT(0, lua_gettop(L));

    // Side effect visible in state
    lua_getglobal(L, "phase4_simple");
    TEST_ASSERT_TRUE(lua_isnumber(L, -1));
    TEST_ASSERT_EQUAL_INT(42, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// A chunk that returns a single value: caller sees it on the stack.
void test_run_string_returns_values_on_stack(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_run_string(L, "return 7 * 6", "[test:return-one]");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, "return-one should succeed");
    TEST_ASSERT_EQUAL_INT(1, lua_gettop(L));
    TEST_ASSERT_TRUE(lua_isnumber(L, -1));
    TEST_ASSERT_EQUAL_INT(42, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// LUA_MULTRET: the chunk can return any number of values.
void test_run_string_returns_multret_values(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_run_string(L, "return 1, 2, 3, 'four'", "[test:return-many]");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, "return-many should succeed");
    TEST_ASSERT_EQUAL_INT(4, lua_gettop(L));

    TEST_ASSERT_EQUAL_INT(1, (int)lua_tointeger(L, -4));
    TEST_ASSERT_EQUAL_INT(2, (int)lua_tointeger(L, -3));
    TEST_ASSERT_EQUAL_INT(3, (int)lua_tointeger(L, -2));
    TEST_ASSERT_TRUE(lua_isstring(L, -1));
    TEST_ASSERT_EQUAL_STRING("four", lua_tostring(L, -1));

    lua_pop(L, 4);
    H_lua_destroy_context(L);
}

// Syntax error: LUA_ERRSYNTAX, single error string left on the stack.
void test_run_string_syntax_error_returns_erryntax(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_run_string(L, "this is not valid lua (", "[test:syntax]");
    TEST_ASSERT_EQUAL_INT(LUA_ERRSYNTAX, rc);
    TEST_ASSERT_EQUAL_INT(1, lua_gettop(L));
    TEST_ASSERT_TRUE(lua_isstring(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// Runtime error: LUA_ERRRUN, single error string left on the stack.
void test_run_string_runtime_error_returns_errrun(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_run_string(L, "error('phase4 boom')", "[test:runtime]");
    TEST_ASSERT_EQUAL_INT(LUA_ERRRUN, rc);
    TEST_ASSERT_EQUAL_INT(1, lua_gettop(L));
    TEST_ASSERT_TRUE(lua_isstring(L, -1));
    // The runtime error we raised should be in the message
    const char* msg = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_NOT_NULL(strstr(msg, "phase4 boom"));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// NULL state: must return LUA_ERRRUN, must not crash.
void test_run_string_null_state_returns_errrun(void) {
    int rc = H_lua_run_string(NULL, "return 1", "[test:null-state]");
    TEST_ASSERT_EQUAL_INT(LUA_ERRRUN, rc);
}

// NULL code: must return LUA_ERRRUN, error string left on the stack.
void test_run_string_null_code_returns_errrun(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_run_string(L, NULL, "[test:null-code]");
    TEST_ASSERT_EQUAL_INT(LUA_ERRRUN, rc);
    TEST_ASSERT_EQUAL_INT(1, lua_gettop(L));
    TEST_ASSERT_TRUE(lua_isstring(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// NULL name: should fall back to "?" without raising an error.
void test_run_string_null_name_uses_default(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_run_string(L, "return 99", NULL);
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, "NULL name should be tolerated");
    TEST_ASSERT_EQUAL_INT(1, lua_gettop(L));
    TEST_ASSERT_EQUAL_INT(99, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// Empty string: a valid no-op chunk, returns LUA_OK with no return values.
void test_run_string_empty_string_is_valid(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_run_string(L, "", "[test:empty]");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, "empty string is a valid chunk");
    TEST_ASSERT_EQUAL_INT(0, lua_gettop(L));

    H_lua_destroy_context(L);
}

// Confirm state persists across multiple H_lua_run_string calls on the
// same context - the worker model is "one state per job, one chunk per
// job", but a debug/admin context could chain chunks in principle.
void test_run_string_sets_global_state(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc1 = H_lua_run_string(L, "phase4_counter = 0", "[test:chain:1]");
    TEST_ASSERT_EQUAL_INT(LUA_OK, rc1);

    int rc2 = H_lua_run_string(L, "phase4_counter = phase4_counter + 1", "[test:chain:2]");
    TEST_ASSERT_EQUAL_INT(LUA_OK, rc2);

    int rc3 = H_lua_run_string(L, "phase4_counter = phase4_counter + 10", "[test:chain:3]");
    TEST_ASSERT_EQUAL_INT(LUA_OK, rc3);

    lua_getglobal(L, "phase4_counter");
    TEST_ASSERT_EQUAL_INT(11, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_run_string_executes_simple_script);
    RUN_TEST(test_run_string_returns_values_on_stack);
    RUN_TEST(test_run_string_returns_multret_values);
    RUN_TEST(test_run_string_syntax_error_returns_erryntax);
    RUN_TEST(test_run_string_runtime_error_returns_errrun);
    RUN_TEST(test_run_string_null_state_returns_errrun);
    RUN_TEST(test_run_string_null_code_returns_errrun);
    RUN_TEST(test_run_string_null_name_uses_default);
    RUN_TEST(test_run_string_empty_string_is_valid);
    RUN_TEST(test_run_string_sets_global_state);

    return UNITY_END();
}
