#include <src/hydrogen.h>
#include <unity.h>

#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include <src/scripting/lua_context.h>
#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/scripting_handle.h>

static AppConfig mock_app_config_storage;

static char* fetch_echo(const char* group, const char* name) {
    if (group && name && strcmp(group, "Mcp") == 0 && strcmp(name, "Echo") == 0) {
        return strdup("H.set_result_json({ ok = true })\n");
    }
    return NULL;
}

static char* submit_ok(const char* script_name, const char* source, const char* params_json) {
    (void)script_name;
    (void)source;
    (void)params_json;
    return strdup("job-mcp-1");
}

static int wait_ok(lua_State* L, H_Handle* h) {
    h->consumed = true;
    lua_newtable(L);
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "ok");
    lua_pushnil(L);
    return 2;
}

void test_call_async_and_wait(void);
void test_call_async_denied(void);
void test_call_async_name_required(void);
void test_call_async_parse_fail(void);
void test_call_async_reserved(void);
void test_call_async_submit_fail(void);
void test_wait_one_error_paths(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
    H_lua_mcp_clear_hooks();
}

void tearDown(void) {
    H_lua_mcp_clear_hooks();
    app_config = NULL;
}

void test_call_async_and_wait(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_fetch_source_hook = fetch_echo;
    H_lua_mcp_submit_job_hook = submit_ok;
    H_lua_mcp_wait_job_hook = wait_ok;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "h = H.mcp.call_async('Mcp.Echo', { x = 1 })\n"
        "r, e = H.wait(h)\n"
        "assert(e == nil)\n"
        "assert(r.ok == true)\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    TEST_ASSERT_EQUAL_INT(1, H_lua_mcp_submit_count);
    H_lua_destroy_context(L);
}

void test_call_async_denied(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_fetch_source_hook = fetch_echo;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "h = H.mcp.call_async('Mcp.Nope', {})\n"
        "r, e = H.wait(h)\n"
        "assert(r == nil)\n"
        "assert(e == 'not found')\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    TEST_ASSERT_EQUAL_INT(0, H_lua_mcp_submit_count);
    H_lua_destroy_context(L);
}

void test_call_async_name_required(void) {
    lua_State* L;
    int rc;

    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "h = H.mcp.call_async()\n"
        "r, e = H.wait(h)\n"
        "assert(r == nil)\n"
        "assert(e:find('name required', 1, true))\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    H_lua_destroy_context(L);
}

void test_call_async_parse_fail(void) {
    lua_State* L;
    int rc;

    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "h = H.mcp.call_async('badname')\n"
        "r, e = H.wait(h)\n"
        "assert(r == nil and e == 'not found')\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    H_lua_destroy_context(L);
}

void test_call_async_reserved(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_fetch_source_hook = fetch_echo;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "h = H.mcp.call_async('Mcp.Echo', { _hydrogen = {} })\n"
        "r, e = H.wait(h)\n"
        "assert(r == nil and e == 'reserved _hydrogen')\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    H_lua_destroy_context(L);
}

static char* submit_fail(const char* script_name, const char* source, const char* params_json) {
    (void)script_name;
    (void)source;
    (void)params_json;
    return NULL;
}

void test_call_async_submit_fail(void) {
    lua_State* L;
    int rc;

    H_lua_mcp_fetch_source_hook = fetch_echo;
    H_lua_mcp_submit_job_hook = submit_fail;
    L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "h = H.mcp.call_async('Mcp.Echo', {})\n"
        "r, e = H.wait(h)\n"
        "assert(r == nil)\n"
        "assert(e:find('submit failed', 1, true))\n"
        "return true\n");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
    H_lua_destroy_context(L);
}

void test_wait_one_error_paths(void) {
    lua_State* L = luaL_newstate();
    H_Handle h;
    int n;

    TEST_ASSERT_NOT_NULL(L);
    n = H_lua_mcp_wait_one(L, NULL);
    TEST_ASSERT_EQUAL_INT(2, n);

    memset(&h, 0, sizeof(h));
    h.consumed = true;
    n = H_lua_mcp_wait_one(L, &h);
    TEST_ASSERT_EQUAL_INT(2, n);

    h.consumed = false;
    h.error = strdup("boom");
    n = H_lua_mcp_wait_one(L, &h);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_TRUE(h.consumed);
    free(h.error);

    memset(&h, 0, sizeof(h));
    n = H_lua_mcp_wait_one(L, &h);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_TRUE(h.consumed);
    lua_close(L);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_call_async_and_wait);
    RUN_TEST(test_call_async_denied);
    RUN_TEST(test_call_async_name_required);
    RUN_TEST(test_call_async_parse_fail);
    RUN_TEST(test_call_async_reserved);
    RUN_TEST(test_call_async_submit_fail);
    RUN_TEST(test_wait_one_error_paths);
    return UNITY_END();
}
