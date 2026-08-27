#include <src/hydrogen.h>
#include <unity.h>

#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include <src/scripting/lua_context.h>
#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>

static AppConfig mock_app_config_storage;

static char* fetch_echo(const char* group, const char* name) {
    if (group && name && strcmp(group, "Mcp") == 0 && strcmp(name, "Echo") == 0) {
        return strdup(
            "H.set_result_json({ echoed = params.msg, has_h = params._hydrogen ~= nil })\n");
    }
    return NULL;
}

static char* fetch_boom(const char* group, const char* name) {
    (void)group;
    (void)name;
    return strdup("error('tool boom')\n");
}

void test_call_denied(void);
void test_call_inline_ok_no_submit(void);
void test_call_reserved_key(void);
void test_call_child_error_destroys(void);
void test_call_name_required_and_parse_fail(void);
void test_call_no_args_table(void);
void test_call_no_set_result(void);
void test_call_parent_hydrogen_nontable(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
    H_lua_mcp_clear_hooks();
}

void tearDown(void) {
    H_lua_mcp_clear_hooks();
    app_config = NULL;
}

void test_call_denied(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_fetch_source_hook = fetch_echo;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "r, e = H.mcp.call('Mcp.Missing', {})\n"
        "assert(r == nil)\n"
        "assert(e == 'not found')\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    TEST_ASSERT_EQUAL_INT(0, H_lua_mcp_submit_count);
    H_lua_destroy_context(L);
}

void test_call_inline_ok_no_submit(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_fetch_source_hook = fetch_echo;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "params = { _hydrogen = { sub = 'u1' } }\n"
        "r, e = H.mcp.call('Mcp.Echo', { msg = 'hi' })\n"
        "assert(e == nil)\n"
        "assert(r.echoed == 'hi')\n"
        "assert(r.has_h == true)\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    TEST_ASSERT_EQUAL_INT(0, H_lua_mcp_submit_count);
    H_lua_destroy_context(L);
}

void test_call_reserved_key(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_fetch_source_hook = fetch_echo;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "r, e = H.mcp.call('Mcp.Echo', { _hydrogen = { sub = 'evil' } })\n"
        "assert(r == nil)\n"
        "assert(e == 'reserved _hydrogen')\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    H_lua_destroy_context(L);
}

void test_call_child_error_destroys(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_fetch_source_hook = fetch_boom;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "r, e = H.mcp.call('Mcp.Echo', {})\n"
        "assert(r == nil)\n"
        "assert(type(e) == 'string')\n"
        "assert(e:find('tool boom', 1, true))\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    TEST_ASSERT_EQUAL_INT(0, H_lua_mcp_submit_count);
    H_lua_destroy_context(L);
}

void test_call_name_required_and_parse_fail(void) {
    lua_State* L;
    int rc;

    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "r, e = H.mcp.call()\n"
        "assert(r == nil and e:find('name required', 1, true))\n"
        "r, e = H.mcp.call('noperiod')\n"
        "assert(r == nil and e == 'not found')\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    H_lua_destroy_context(L);
}

void test_call_no_args_table(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_fetch_source_hook = fetch_echo;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "r, e = H.mcp.call('Mcp.Echo')\n"
        "assert(e == nil)\n"
        "assert(r.has_h == false)\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    H_lua_destroy_context(L);
}

static char* fetch_no_result(const char* group, const char* name) {
    (void)group;
    (void)name;
    return strdup("return 1\n");
}

void test_call_no_set_result(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_fetch_source_hook = fetch_no_result;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "r, e = H.mcp.call('Mcp.Echo', {})\n"
        "assert(e == nil)\n"
        "assert(type(r) == 'table')\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    H_lua_destroy_context(L);
}

void test_call_parent_hydrogen_nontable(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_fetch_source_hook = fetch_echo;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "params = { _hydrogen = 'not-a-table' }\n"
        "r, e = H.mcp.call('Mcp.Echo', { msg = 'x' })\n"
        "assert(e == nil)\n"
        "assert(r.has_h == false)\n"
        "params = 'nope'\n"
        "r, e = H.mcp.call('Mcp.Echo', { msg = 'y' })\n"
        "assert(e == nil)\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_call_denied);
    RUN_TEST(test_call_inline_ok_no_submit);
    RUN_TEST(test_call_reserved_key);
    RUN_TEST(test_call_child_error_destroys);
    RUN_TEST(test_call_name_required_and_parse_fail);
    RUN_TEST(test_call_no_args_table);
    RUN_TEST(test_call_no_set_result);
    RUN_TEST(test_call_parent_hydrogen_nontable);
    return UNITY_END();
}
