/*
 * Unity Test File: scripting_api_scoreboard_test_H_lua_install_package.c
 *
 * Tests H_lua_install_package:
 *   - NULL state is a no-op
 *   - AllowDBModuleLoad false is a no-op
 *   - package table missing logs error
 *   - package.searchers not a table logs error
 *   - Normal case installs searcher
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/lua_context.h>
#include <src/scripting/scripting_api.h>
#include <tests/unity/mocks/mock_logging.h>

void test_install_package_null_state_is_noop(void);
void test_install_package_disabled_is_noop(void);
void test_install_package_package_table_missing_logs_error(void);
void test_install_package_searchers_not_table_logs_error(void);
void test_install_package_normal_installs_searcher(void);

static AppConfig mock_app_config_storage;

void setUp(void) {
    mock_logging_reset_all();
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
    mock_logging_reset_all();
}

void test_install_package_null_state_is_noop(void) {
    mock_app_config_storage.scripting.AllowDBModuleLoad = true;
    H_lua_install_package(NULL);
    TEST_PASS();
}

void test_install_package_disabled_is_noop(void) {
    mock_app_config_storage.scripting.AllowDBModuleLoad = false;

    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_package(L);

    int rc = luaL_dostring(L, "return #package.searchers");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    lua_Integer count = lua_tointeger(L, -1);
    TEST_ASSERT_EQUAL(4, count);
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

void test_install_package_package_table_missing_logs_error(void) {
    mock_app_config_storage.scripting.AllowDBModuleLoad = true;

    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_pushnil(L);
    lua_setglobal(L, "package");

    H_lua_install_package(L);

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "package table missing"));

    H_lua_destroy_context(L);
}

void test_install_package_searchers_not_table_logs_error(void) {
    mock_app_config_storage.scripting.AllowDBModuleLoad = true;

    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "package");
    lua_pushstring(L, "not_a_table");
    lua_setfield(L, -2, "searchers");
    lua_pop(L, 1);

    H_lua_install_package(L);

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "package.searchers not a table"));

    H_lua_destroy_context(L);
}

void test_install_package_normal_installs_searcher(void) {
    mock_app_config_storage.scripting.AllowDBModuleLoad = true;

    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = luaL_dostring(L, "return #package.searchers");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    lua_Integer count = lua_tointeger(L, -1);
    TEST_ASSERT_EQUAL(5, count);
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_install_package_null_state_is_noop);
    RUN_TEST(test_install_package_disabled_is_noop);
    RUN_TEST(test_install_package_package_table_missing_logs_error);
    RUN_TEST(test_install_package_searchers_not_table_logs_error);
    RUN_TEST(test_install_package_normal_installs_searcher);

    return UNITY_END();
}
