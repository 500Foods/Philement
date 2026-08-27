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

static char* list_native_schema(void) {
    return strdup(
        "[{\"group_name\":\"Mcp\",\"script_name\":\"Obj\","
        "\"mcp_schema\":{\"type\":\"object\"},\"mcp_annotations\":1}]");
}

static char* list_not_array(void) {
    return strdup("{\"nope\":true}");
}

static char* list_null(void) {
    return NULL;
}

void test_list_empty(void);
void test_list_two_with_and_without_schema(void);
void test_list_pagination(void);
void test_list_string_cursor_and_clamps(void);
void test_list_fetch_null_and_bad_json(void);
void test_list_native_schema_object(void);
void test_install_null(void);
void test_install_missing_h(void);
void test_capture_result_not_table(void);
void test_fetch_list_no_hook_no_db(void);

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

void test_list_string_cursor_and_clamps(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_list_rows_hook = list_two;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "rows, n = H.mcp.list('1', 0)\n"
        "assert(#rows == 2)\n"
        "rows2 = H.mcp.list(0, 999)\n"
        "assert(#rows2 == 2)\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    H_lua_destroy_context(L);
}

void test_list_fetch_null_and_bad_json(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_list_rows_hook = list_null;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "rows, c = H.mcp.list()\n"
        "assert(c == nil)\n"
        "assert(type(rows) == 'table')\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    H_lua_destroy_context(L);

    H_lua_mcp_list_rows_hook = list_not_array;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "rows, c = H.mcp.list()\n"
        "assert(c == nil)\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    H_lua_destroy_context(L);
}

void test_list_native_schema_object(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_list_rows_hook = list_native_schema;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "rows = H.mcp.list()\n"
        "assert(rows[1].schema.type == 'object')\n"
        "assert(rows[1].annotations == nil)\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    H_lua_destroy_context(L);
}

void test_install_missing_h(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_lua_install_mcp(L);
    lua_close(L);
}

void test_capture_result_not_table(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    TEST_ASSERT_EQUAL_INT(0, H_lua_mcp_capture_result_json(NULL));
    lua_pushstring(L, "nope");
    TEST_ASSERT_EQUAL_INT(0, H_lua_mcp_capture_result_json(L));
    lua_close(L);
}

void test_fetch_list_no_hook_no_db(void) {
    TEST_ASSERT_NULL(H_lua_mcp_fetch_list_rows_json());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_install_null);
    RUN_TEST(test_list_empty);
    RUN_TEST(test_list_two_with_and_without_schema);
    RUN_TEST(test_list_pagination);
    RUN_TEST(test_list_string_cursor_and_clamps);
    RUN_TEST(test_list_fetch_null_and_bad_json);
    RUN_TEST(test_list_native_schema_object);
    RUN_TEST(test_install_missing_h);
    RUN_TEST(test_capture_result_not_table);
    RUN_TEST(test_fetch_list_no_hook_no_db);
    return UNITY_END();
}
