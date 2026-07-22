/*
 * Unity Test File: scripting_api_llm_test_call_error_paths.c
 *
 * Tests H_lua_llm_call() error paths and parameter validation:
 *   - empty model name
 *   - invalid prompt type (boolean)
 *   - table prompt with fields
 *   - allocation failure
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/lua_context.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/scripting_api.h>

#include <unity/mocks/mock_dbqueue.h>

#include <unity/mocks/mock_chat_engine_cache.h>

#include <unity/mocks/mock_auth_chat_deps.h>

static AppConfig mock_app_config_storage = {0};

static lua_State* make_llm_context(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    lua_getglobal(L, "H");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "llm");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_pop(L, 2);
    return L;
}

static void run_lua(lua_State* L, const char* code) {
    int rc = luaL_loadbuffer(L, code, strlen(code), "test");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, "luaL_loadbuffer failed");
    rc = lua_pcall(L, 0, LUA_MULTRET, 0);
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
}

/* Forward declarations */
void test_llm_call_empty_model_name(void);
void test_llm_call_invalid_prompt_type_boolean(void);
void test_llm_call_invalid_prompt_type_function(void);
void test_llm_call_table_prompt(void);
void test_llm_call_allocation_failure(void);

void setUp(void) {
    mock_system_reset_all();
    mock_dbqueue_reset_all();
    mock_chat_engine_cache_reset_all();
    mock_auth_chat_deps_reset_all();
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

void test_llm_call_empty_model_name(void) {
    lua_State* L = make_llm_context();
    run_lua(L,
        "h = H.llm.call('')\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil and e:find('model') ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_call_invalid_prompt_type_boolean(void) {
    lua_State* L = make_llm_context();
    run_lua(L,
        "h = H.llm.call('test-model', true)\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil and e:find('prompt') ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_call_invalid_prompt_type_function(void) {
    lua_State* L = make_llm_context();
    run_lua(L,
        "h = H.llm.call('test-model', function() end)\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil and e:find('prompt') ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_call_table_prompt(void) {
    lua_State* L = make_llm_context();
    run_lua(L,
        "h = H.llm.call('test-model', {max_tokens=100, temperature=0.5})\n"
        "_kind = (type(h) == 'userdata' and h ~= nil)\n");
    lua_getglobal(L, "_kind");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_call_allocation_failure(void) {
    lua_State* L = make_llm_context();
    run_lua(L,
        "r, e = H.llm.call_sync('test-model', 'hello')\n"
        "_got_err = (r == nil and e ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_llm_call_empty_model_name);
    RUN_TEST(test_llm_call_invalid_prompt_type_boolean);
    RUN_TEST(test_llm_call_invalid_prompt_type_function);
    RUN_TEST(test_llm_call_table_prompt);
    RUN_TEST(test_llm_call_allocation_failure);

    return UNITY_END();
}
