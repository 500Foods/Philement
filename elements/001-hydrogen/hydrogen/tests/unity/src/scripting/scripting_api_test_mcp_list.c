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

static char* list_empty(void) {
    return strdup("[]");
}

static char* list_two(void) {
    return strdup(
        "["
        "{\"group_name\":\"Mcp\",\"script_name\":\"Echo\","
        "\"summary\":\"echo\","
        "\"mcp_schema\":\"{\\\"inputSchema\\\":{\\\"type\\\":\\\"object\\\"}}\","
        "\"mcp_annotations\":\"{\\\"readOnlyHint\\\":true}\"},"
        "{\"group_name\":\"Mcp\",\"script_name\":\"Bare\","
        "\"summary\":\"bare\"}"
        "]");
}

void test_list_empty(void);
void test_list_two_with_and_without_schema(void);
void test_list_pagination(void);
void test_install_null(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
    H_lua_mcp_clear_hooks();
}

void tearDown(void) {
    H_lua_mcp_clear_hooks();
    app_config = NULL;
}

void test_install_null(void) {
    H_lua_install_mcp(NULL);
    TEST_PASS();
}

void test_list_empty(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_list_rows_hook = list_empty;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L, "rows, cursor = H.mcp.list(); return rows, cursor");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    TEST_ASSERT_TRUE(lua_istable(L, -2));
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
    lua_len(L, -2);
    TEST_ASSERT_EQUAL_INT(0, (int)lua_tointeger(L, -1));
    H_lua_destroy_context(L);
}

void test_list_two_with_and_without_schema(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_list_rows_hook = list_two;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "rows, cursor = H.mcp.list()\n"
        "assert(cursor == nil)\n"
        "assert(rows[1].name == 'Mcp.Echo')\n"
        "assert(rows[1].schema.inputSchema.type == 'object')\n"
        "assert(rows[1].annotations.readOnlyHint == true)\n"
        "assert(rows[2].name == 'Mcp.Bare')\n"
        "assert(rows[2].schema == nil)\n"
        "assert(rows[2].annotations == nil)\n"
        "return #rows\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    TEST_ASSERT_EQUAL_INT(2, (int)lua_tointeger(L, -1));
    H_lua_destroy_context(L);
}

void test_list_pagination(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_list_rows_hook = list_two;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "rows, next = H.mcp.list(1, 1)\n"
        "assert(#rows == 1 and rows[1].name == 'Mcp.Echo')\n"
        "assert(next == 2)\n"
        "rows2, next2 = H.mcp.list(next, 1)\n"
        "assert(#rows2 == 1 and rows2[1].name == 'Mcp.Bare')\n"
        "assert(next2 == nil)\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_install_null);
    RUN_TEST(test_list_empty);
    RUN_TEST(test_list_two_with_and_without_schema);
    RUN_TEST(test_list_pagination);
    return UNITY_END();
}
