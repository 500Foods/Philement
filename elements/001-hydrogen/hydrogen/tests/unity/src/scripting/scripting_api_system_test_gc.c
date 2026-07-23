/*
 * Unity Test File: scripting_api_system_test_gc.c
 *
 * Phase 8 of the LUA_PLAN. Tests H.gc.{collect, step, count, isrunning}
 * - the explicit GC control functions exposed to Lua scripts.
 *
 * Also tests H_lua_install_gc error paths (H table missing,
 * H.gc not a table) and the normal installation path.
 *
 * Validates:
 *   - H.gc.collect() returns a number
 *   - H.gc.step() returns a number
 *   - H.gc.count() returns a non-negative number
 *   - H.gc.isrunning() returns a boolean
 *   - All four functions can be called from a Lua chunk
 *   - H_lua_install_gc logs an error when H table is missing
 *   - H_lua_install_gc logs an error when H.gc is not a table
 *   - H_lua_install_gc installs all four functions on normal path
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

// Modules under test
#include <src/scripting/lua_context.h>
#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>

// Mock logging introspection
#include <tests/unity/mocks/mock_logging.h>

// Mock app_config (zeroed = no special defaults needed for this suite)
static AppConfig mock_app_config_storage = {0};

// Forward declarations
void test_gc_collect_returns_number(void);
void test_gc_step_returns_number(void);
void test_gc_count_returns_number(void);
void test_gc_isrunning_returns_boolean(void);
void test_gc_functions_via_lua_chunk(void);
void test_gc_install_h_table_missing(void);
void test_gc_install_h_gc_not_a_table(void);
void test_gc_install_normal_installation(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
    mock_logging_reset_all();
}

void tearDown(void) {
    app_config = NULL;
}

// H.gc.collect() returns a number (the KB value from lua_gc).
void test_gc_collect_returns_number(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_gc(L);
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "gc");
    lua_getfield(L, -1, "collect");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "gc.collect should succeed");
    TEST_ASSERT_TRUE(lua_isnumber(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// H.gc.step() returns a number (0 or 1 from lua_gc).
void test_gc_step_returns_number(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_gc(L);
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "gc");
    lua_getfield(L, -1, "step");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "gc.step should succeed");
    TEST_ASSERT_TRUE(lua_isnumber(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// H.gc.count() returns a non-negative number (current memory in KB).
void test_gc_count_returns_number(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_gc(L);
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "gc");
    lua_getfield(L, -1, "count");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "gc.count should succeed");
    TEST_ASSERT_TRUE(lua_isnumber(L, -1));
    lua_Number v = lua_tonumber(L, -1);
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, v);
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// H.gc.isrunning() returns a boolean.
void test_gc_isrunning_returns_boolean(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_gc(L);
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "gc");
    lua_getfield(L, -1, "isrunning");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "gc.isrunning should succeed");
    TEST_ASSERT_TRUE(lua_isboolean(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// All four GC functions can be called from a Lua chunk via H_lua_run_string.
void test_gc_functions_via_lua_chunk(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_gc(L);

    const char* code =
        "gc_collect_result = H.gc.collect()\n"
        "gc_step_result = H.gc.step()\n"
        "gc_count_result = H.gc.count()\n"
        "gc_isrunning_result = H.gc.isrunning()\n"
        ;

    int rc = H_lua_run_string(L, code, "[phase8:gc-all]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "gc chunk should run successfully");

    lua_getglobal(L, "gc_collect_result");
    TEST_ASSERT_TRUE(lua_isnumber(L, -1));
    lua_pop(L, 1);

    lua_getglobal(L, "gc_step_result");
    TEST_ASSERT_TRUE(lua_isnumber(L, -1));
    lua_pop(L, 1);

    lua_getglobal(L, "gc_count_result");
    TEST_ASSERT_TRUE(lua_isnumber(L, -1));
    lua_pop(L, 1);

    lua_getglobal(L, "gc_isrunning_result");
    TEST_ASSERT_TRUE(lua_isboolean(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// H_lua_install_gc with no H table: should log "H table missing" and return.
void test_gc_install_h_table_missing(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    // Remove the H global so lua_getglobal returns nil.
    lua_pushnil(L);
    lua_setglobal(L, "H");

    H_lua_install_gc(L);

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "H table missing"));

    H_lua_destroy_context(L);
}

// H_lua_install_gc with H.gc not a table: should log "H.gc not a table" and return.
void test_gc_install_h_gc_not_a_table(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    // Replace H.gc with a non-table value.
    lua_getglobal(L, "H");
    lua_pushnumber(L, 42);
    lua_setfield(L, -2, "gc");
    lua_pop(L, 1);

    H_lua_install_gc(L);

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "H.gc not a table"));

    H_lua_destroy_context(L);
}

// Normal installation: H_lua_install_gc installs all four functions.
void test_gc_install_normal_installation(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_gc(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "gc");
    TEST_ASSERT_TRUE(lua_istable(L, -1));

    lua_getfield(L, -1, "collect");
    TEST_ASSERT_TRUE(lua_isfunction(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "step");
    TEST_ASSERT_TRUE(lua_isfunction(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "count");
    TEST_ASSERT_TRUE(lua_isfunction(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "isrunning");
    TEST_ASSERT_TRUE(lua_isfunction(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 2);

    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_gc_collect_returns_number);
    RUN_TEST(test_gc_step_returns_number);
    RUN_TEST(test_gc_count_returns_number);
    RUN_TEST(test_gc_isrunning_returns_boolean);
    RUN_TEST(test_gc_functions_via_lua_chunk);
    RUN_TEST(test_gc_install_h_table_missing);
    RUN_TEST(test_gc_install_h_gc_not_a_table);
    RUN_TEST(test_gc_install_normal_installation);

    return UNITY_END();
}
