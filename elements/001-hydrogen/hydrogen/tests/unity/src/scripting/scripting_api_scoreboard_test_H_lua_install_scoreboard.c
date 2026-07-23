/*
 * Unity Test File: scripting_api_scoreboard_test_H_lua_install_scoreboard.c
 *
 * Tests H_lua_install_scoreboard:
 *   - NULL state is a no-op
 *   - H table missing logs error
 *   - H.scoreboard not a table logs error
 *   - Normal case installs list/get/submit/cancel functions
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

void test_install_scoreboard_null_state_is_noop(void);
void test_install_scoreboard_h_table_missing_logs_error(void);
void test_install_scoreboard_scoreboard_not_table_logs_error(void);
void test_install_scoreboard_normal_installs_functions(void);

void setUp(void) {
    mock_logging_reset_all();
}

void tearDown(void) {
    mock_logging_reset_all();
}

void test_install_scoreboard_null_state_is_noop(void) {
    H_lua_install_scoreboard(NULL);
    TEST_PASS();
}

void test_install_scoreboard_h_table_missing_logs_error(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_pushnil(L);
    lua_setglobal(L, "H");

    H_lua_install_scoreboard(L);

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "H table missing"));

    H_lua_destroy_context(L);
}

void test_install_scoreboard_scoreboard_not_table_logs_error(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_pushstring(L, "not_a_table");
    lua_setfield(L, -2, "scoreboard");
    lua_pop(L, 1);

    H_lua_install_scoreboard(L);

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "H.scoreboard not a table"));

    H_lua_destroy_context(L);
}

void test_install_scoreboard_normal_installs_functions(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_scoreboard(L);

    int rc = luaL_dostring(L,
        "return type(H) == 'table' and type(H.scoreboard) == 'table' "
        "and type(H.scoreboard.list) == 'function' "
        "and type(H.scoreboard.get) == 'function' "
        "and type(H.scoreboard.submit) == 'function' "
        "and type(H.scoreboard.cancel) == 'function'");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_install_scoreboard_null_state_is_noop);
    RUN_TEST(test_install_scoreboard_h_table_missing_logs_error);
    RUN_TEST(test_install_scoreboard_scoreboard_not_table_logs_error);
    RUN_TEST(test_install_scoreboard_normal_installs_functions);

    return UNITY_END();
}
