/*
 * Unity Test File: lua_context_test_script_options.c
 *
 * scripts.options JSON: sandbox flags and limit clamp.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <string.h>

#include <src/scripting/lua_context.h>
#include <src/scripting/scoreboard.h>

static AppConfig mock_app_config_storage;

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    mock_app_config_storage.scripting.Sandbox.AllowIo = false;
    mock_app_config_storage.scripting.Sandbox.AllowDebug = false;
    mock_app_config_storage.scripting.Sandbox.AllowLoadlib = false;
    mock_app_config_storage.scripting.Sandbox.AllowOsTime = true;
    mock_app_config_storage.scripting.Sandbox.AllowOsExecute = false;
    mock_app_config_storage.scripting.MemorySoftLimitKB = 32768;
    mock_app_config_storage.scripting.MemoryHardLimitKB = 65536;
    mock_app_config_storage.scripting.DefaultMaxRuntime = 3600;
    mock_app_config_storage.scripting.InstructionHookInterval = 5000;
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}


void test_parse_empty_object(void);
void test_parse_malformed_returns_false(void);
void test_parse_sandbox_io_true(void);
void test_create_context_io_nil_by_default(void);
void test_create_context_ex_enables_io(void);
void test_limits_clamp_to_memory_hard_ceiling(void);

void test_parse_empty_object(void) {
    H_LuaScriptOptions opt;
    TEST_ASSERT_TRUE(H_lua_parse_script_options("{}", &opt));
    TEST_ASSERT_FALSE(opt.sandbox_io);
    TEST_ASSERT_FALSE(opt.has_memory_hard_kb);
}

void test_parse_malformed_returns_false(void) {
    H_LuaScriptOptions opt;
    TEST_ASSERT_FALSE(H_lua_parse_script_options("{not json", &opt));
    TEST_ASSERT_FALSE(opt.sandbox_io);
}

void test_parse_sandbox_io_true(void) {
    H_LuaScriptOptions opt;
    TEST_ASSERT_TRUE(H_lua_parse_script_options(
        "{\"sandbox\":{\"io\":true}}", &opt));
    TEST_ASSERT_TRUE(opt.sandbox_io);
}

void test_create_context_io_nil_by_default(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    lua_getglobal(L, "io");
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_create_context_ex_enables_io(void) {
    lua_State* L = H_lua_create_context_ex("{\"sandbox\":{\"io\":true}}");
    TEST_ASSERT_NOT_NULL(L);
    lua_getglobal(L, "io");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_limits_clamp_to_memory_hard_ceiling(void) {
    H_LuaScriptOptions opt;
    ScoreboardJobLimits lim;
    memset(&lim, 0, sizeof(lim));
    TEST_ASSERT_TRUE(H_lua_parse_script_options(
        "{\"limits\":{\"memoryHardKB\":999999,\"memorySoftKB\":999999}}", &opt));
    H_lua_limits_from_script_options(&opt, &lim);
    TEST_ASSERT_EQUAL_UINT(65536, (unsigned)lim.memory_hard_limit_kb);
    TEST_ASSERT_EQUAL_UINT(65536, (unsigned)lim.memory_soft_limit_kb);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_empty_object);
    RUN_TEST(test_parse_malformed_returns_false);
    RUN_TEST(test_parse_sandbox_io_true);
    RUN_TEST(test_create_context_io_nil_by_default);
    RUN_TEST(test_create_context_ex_enables_io);
    RUN_TEST(test_limits_clamp_to_memory_hard_ceiling);
    return UNITY_END();
}
