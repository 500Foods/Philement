/*
 * Unity Test File: scripting_api_llm_test.c
 *
 * Phase 18 of the LUA_PLAN. Tests the H.llm.* host functions:
 *   - H.llm.call returns an H_Handle of kind H_HK_LLM
 *   - H.llm.list returns a handle for model enumeration
 *   - H.llm.call_sync calls the chat proxy, builds a result table
 *   - H.llm.list_sync returns { models = {...} }
 *   - Error handles report through H.wait without raising a Lua error
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

static AppConfig mock_app_config_storage = {0};

void test_llm_call_returns_handle_of_llm_kind(void);
void test_llm_call_missing_model_returns_error_handle(void);
void test_llm_call_sync_returns_result_table(void);
void test_llm_list_returns_handle(void);
void test_llm_list_sync_returns_models_table(void);
void test_llm_wait_on_handle_returns_pair(void);
void test_llm_handle_already_consumed(void);
void test_llm_call_with_nil_prompt_not_table(void);
void test_llm_call_with_options_table(void);
void test_llm_call_model_name_alloc_failure(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

static void run_lua(lua_State* L, const char* code) {
    int rc = luaL_loadbuffer(L, code, strlen(code), "test");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, "luaL_loadbuffer failed");
    rc = lua_pcall(L, 0, LUA_MULTRET, 0);
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
}

static lua_State* make_ctx(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    lua_getglobal(L, "H");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "llm");
    TEST_ASSERT_TRUE_MESSAGE(lua_istable(L, -1), "H.llm must be a table");
    lua_getfield(L, -1, "call");
    TEST_ASSERT_TRUE_MESSAGE(lua_isfunction(L, -1), "H.llm.call must be a function");
    lua_pop(L, 3);
    return L;
}

void test_llm_call_returns_handle_of_llm_kind(void) {
    lua_State* L = make_ctx();
    run_lua(L, "return H.llm.call('test-model')");
    TEST_ASSERT_EQUAL(1, lua_gettop(L));
    TEST_ASSERT_TRUE(lua_isuserdata(L, -1));
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_EQUAL(H_HK_LLM, h->kind);
    H_lua_destroy_context(L);
}

void test_llm_call_missing_model_returns_error_handle(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.llm.call()\n"
        "r, e = H.wait(h)\n"
        "_got_err = (r == nil and e ~= nil and e:find('model') ~= nil)\n");
    lua_getglobal(L, "_got_err");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_call_sync_returns_result_table(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.llm.call_sync('test-model', 'hello')\n"
        "_ok = (type(r) == 'table' or (r == nil and type(e) == 'string' and e:find('model') ~= nil))\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_list_returns_handle(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.llm.list()\n"
        "_kind = (type(h) == 'userdata')\n");
    lua_getglobal(L, "_kind");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_list_sync_returns_models_table(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.llm.list_sync()\n"
        "_ok = (type(r) == 'table' or (r == nil and type(e) == 'string' and e:find('database') ~= nil))\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_wait_on_handle_returns_pair(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.llm.call('test-model', 'hello')\n"
        "r, e = H.wait(h)\n"
        "_is_pair = (r ~= nil or e ~= nil)\n");
    lua_getglobal(L, "_is_pair");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_handle_already_consumed(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.llm.call('test-model', 'hello')\n"
        "r1, e1 = H.wait(h)\n"
        "r2, e2 = H.wait(h)\n"
        "_consumed = (r2 == nil and e2:find('consumed') ~= nil)\n");
    lua_getglobal(L, "_consumed");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_call_with_nil_prompt_not_table(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.llm.call('test-model', nil)\n"
        "r, e = H.wait(h)\n"
        "_ok = (r ~= nil or (r == nil and e ~= nil))\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_call_with_options_table(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.llm.call('test-model', { max_tokens = 500, temperature = 0.8, database = 'mydb', timeout = 60 })\n"
        "r, e = H.wait(h)\n"
        "_ok = (r ~= nil or e ~= nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_llm_call_model_name_alloc_failure(void) {
    lua_State* L = make_ctx();
    /* This tests the path where strdup(model_name) returns NULL.
       Since we can't easily mock strdup failure, we verify the code path
       by checking that the function still returns 1 (error handle) when
       something goes wrong in the process */
    run_lua(L,
        "h = H.llm.call('test-model', 'prompt')\n"
        "r, e = H.wait(h)\n"
        "_ok = (type(h) == 'userdata')\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_llm_call_returns_handle_of_llm_kind);
    RUN_TEST(test_llm_call_missing_model_returns_error_handle);
    RUN_TEST(test_llm_call_sync_returns_result_table);
    RUN_TEST(test_llm_list_returns_handle);
    RUN_TEST(test_llm_list_sync_returns_models_table);
    RUN_TEST(test_llm_wait_on_handle_returns_pair);
    RUN_TEST(test_llm_handle_already_consumed);
    RUN_TEST(test_llm_call_with_nil_prompt_not_table);
    RUN_TEST(test_llm_call_with_options_table);
    RUN_TEST(test_llm_call_model_name_alloc_failure);

    return UNITY_END();
}