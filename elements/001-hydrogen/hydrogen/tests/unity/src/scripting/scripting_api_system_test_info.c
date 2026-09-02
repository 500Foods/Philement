/*
  * Unity Test File: scripting_api_system_test_info.c
  *
  * Phase 6 of CHAT_FINALE. Tests H.system.info() - the Lua host function
  * that returns the same JSON as the authenticated REST GET
  * /api/system/info, built from the shared system_info_build_json helper.
  *
  * Validates:
  *   - H.system.info is installed as a function on the H.system table
  * - Calling H.system.info() returns a table (Lua) on success
  *   - The returned table includes "scripting" sub-table (include_scripting=true)
  *   - Calling H.system.info() from a Lua chunk works via H_lua_run_string
  *   - H_lua_install_system installs "info" alongside the other system functions
  */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/lua_context.h>
#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>

#include <tests/unity/mocks/mock_logging.h>

static AppConfig mock_app_config_storage = {0};

void test_system_info_is_installed(void);
void test_system_info_returns_table(void);
void test_system_info_includes_scripting(void);
void test_system_info_includes_status(void);
void test_system_info_via_lua_chunk(void);
void test_system_info_install_h_table_missing(void);
void test_system_info_install_h_system_not_a_table(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
    mock_logging_reset_all();
}

void tearDown(void) {
    app_config = NULL;
}

void test_system_info_is_installed(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_system(L);

    lua_getglobal(L, "H");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "system");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "info");
    TEST_ASSERT_TRUE(lua_isfunction(L, -1));
    lua_pop(L, 3);

    H_lua_destroy_context(L);
}

void test_system_info_returns_table(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_system(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "system");
    lua_getfield(L, -1, "info");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.system.info() should succeed");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_pop(L, 3);

    H_lua_destroy_context(L);
}

void test_system_info_includes_scripting(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_system(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "system");
    lua_getfield(L, -1, "info");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.system.info() should succeed");
    TEST_ASSERT_TRUE(lua_istable(L, -1));

    /* H.system.info always includes scripting (include_scripting=true). */
    lua_getfield(L, -1, "scripting");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    /* The scripting snapshot carries the "enabled" field. */
    lua_getfield(L, -1, "enabled");
    TEST_ASSERT_TRUE(lua_isboolean(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 2);
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_system_info_includes_status(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_system(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "system");
    lua_getfield(L, -1, "info");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.system.info() should succeed");
    TEST_ASSERT_TRUE(lua_istable(L, -1));

    /* The system status JSON includes a "status" key. */
    lua_getfield(L, -1, "status");
    TEST_ASSERT_FALSE(lua_isnil(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 3);
    H_lua_destroy_context(L);
}

void test_system_info_via_lua_chunk(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_system(L);

    const char* code =
        "phase6_info = H.system.info()\n"
        ;

    int rc = H_lua_run_string(L, code, "[phase6:sysinfo]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "info chunk should run successfully");

    lua_getglobal(L, "phase6_info");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

void test_system_info_install_h_table_missing(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    /* Remove the H global so lua_getglobal returns nil. */
    lua_pushnil(L);
    lua_setglobal(L, "H");

    H_lua_install_system(L);

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "H table missing"));

    H_lua_destroy_context(L);
}

void test_system_info_install_h_system_not_a_table(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    /* Replace H.system with a non-table value. */
    lua_getglobal(L, "H");
    lua_pushnumber(L, 42);
    lua_setfield(L, -2, "system");
    lua_pop(L, 1);

    H_lua_install_system(L);

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "H.system not a table"));

    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_system_info_is_installed);
    RUN_TEST(test_system_info_returns_table);
    RUN_TEST(test_system_info_includes_scripting);
    RUN_TEST(test_system_info_includes_status);
    RUN_TEST(test_system_info_via_lua_chunk);
    RUN_TEST(test_system_info_install_h_table_missing);
    RUN_TEST(test_system_info_install_h_system_not_a_table);

    return UNITY_END();
}
