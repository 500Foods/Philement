/*
 * Unity Test File: scripting_api_llm_test_wait.c
 *
 * Tests H_lua_llm_wait_one() handle error paths and list operations:
 *   - NULL handle returns error
 *   - Consumed handle returns error
 *   - Handle with pre-set error returns error
 *   - Non-LLM handle returns error
 *   - LLM handle missing model name returns error
 *   - LLM list operation returns models table
 *   - LLM call operation with model not found returns error
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>

#define USE_MOCK_AUTH_CHAT_DEPS
#include <unity/mocks/mock_auth_chat_deps.h>

#define USE_MOCK_CHAT_ENGINE_CACHE
#include <unity/mocks/mock_chat_engine_cache.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/scripting_handle.h>
#include <src/scripting/scripting_api.h>
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/proxy.h>

/* Declare global variables as extern */
extern DatabaseQueueManager* global_queue_manager;

/* Forward declarations */
void test_llm_wait_null_handle_returns_error(void);
void test_llm_wait_consumed_handle_returns_error(void);
void test_llm_wait_handle_with_pre_set_error(void);
void test_llm_wait_non_llm_handle_returns_error(void);
void test_llm_wait_llm_handle_missing_model_name(void);
void test_llm_wait_llm_model_not_found(void);
void test_llm_wait_llm_list_returns_models_table(void);
void test_llm_handle_with_llm_error_returns_error(void);

/* Helper to create a test LLM handle */
static H_Handle* create_test_llm_handle(const char* model_name, const char* prompt) {
    H_Handle* h = (H_Handle*)calloc(1, sizeof(H_Handle));
    if (!h) return NULL;
    h->kind = H_HK_LLM;
    h->consumed = false;
    if (model_name) {
        h->llm_model_name = strdup(model_name);
    }
    if (prompt) {
        h->llm_prompt = strdup(prompt);
    }
    h->llm_max_tokens = 100;
    h->llm_temperature = 0.7;
    return h;
}

/* DatabaseQueue mock for tests */
static DatabaseQueue g_test_dbq = {0};

/* App config for tests */
static AppConfig g_test_app_config = {0};

/* Engine config objects */
static ChatEngineConfig g_test_engine = {0};

void setUp(void) {
    memset(&g_test_dbq, 0, sizeof(g_test_dbq));
    memset(&g_test_app_config, 0, sizeof(g_test_app_config));
    memset(&g_test_engine, 0, sizeof(g_test_engine));

    mock_dbqueue_reset_all();
    mock_auth_chat_deps_reset_all();
    mock_chat_engine_cache_reset_all();

    g_test_app_config.scripting.DefaultDatabase = strdup("test-db");
    app_config = &g_test_app_config;

    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    /* Set up engine that will be returned by lookup_by_name */
    snprintf(g_test_engine.name, sizeof(g_test_engine.name), "test-model");
    g_test_engine.provider = CEC_PROVIDER_OPENAI;

    /* Set chat_engine_cache BEFORE setting mock return,
       so resolves in resolve_llm_engine work */
    g_test_dbq.chat_engine_cache = (ChatEngineCache*)0x1;

    mock_dbqueue_set_get_database_result(&g_test_dbq);
    mock_chat_engine_cache_set_lookup_by_name_result(&g_test_engine);
    mock_auth_chat_deps_set_proxy_success(true);
    mock_auth_chat_deps_set_proxy_response_body("{\"response\": \"test response\"}");
}

void tearDown(void) {
    g_test_dbq.chat_engine_cache = NULL;
    free(g_test_app_config.scripting.DefaultDatabase);
    g_test_app_config.scripting.DefaultDatabase = NULL;
}

/* H_lua_llm_wait_one with NULL handle returns error */
void test_llm_wait_null_handle_returns_error(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);

    int result = H_lua_llm_wait_one(L, NULL);

    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_NOT_NULL(strstr(err, "invalid handle"));

    lua_pop(L, 2);
    lua_close(L);
}

/* H_lua_llm_wait_one with consumed handle returns error */
void test_llm_wait_consumed_handle_returns_error(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);

    H_Handle* h = create_test_llm_handle("test-model", "hello");
    h->consumed = true;

    int result = H_lua_llm_wait_one(L, h);

    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_NOT_NULL(strstr(err, "consumed"));

    lua_pop(L, 2);
    free(h->llm_model_name);
    free(h->llm_prompt);
    free(h);
    lua_close(L);
}

/* H_lua_llm_wait_one with pre-set error returns error */
void test_llm_wait_handle_with_pre_set_error(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);

    H_Handle* h = create_test_llm_handle("test-model", "hello");
    h->error = strdup("pre-set error message");

    int result = H_lua_llm_wait_one(L, h);

    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_EQUAL_STRING("pre-set error message", err);
    TEST_ASSERT_TRUE(h->consumed);

    lua_pop(L, 2);
    free(h->llm_model_name);
    free(h->llm_prompt);
    free(h->error);
    free(h);
    lua_close(L);
}

/* H_lua_llm_wait_one with non-LLM handle returns error */
void test_llm_wait_non_llm_handle_returns_error(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);

    H_Handle* h = (H_Handle*)calloc(1, sizeof(H_Handle));
    TEST_ASSERT_NOT_NULL(h);
    h->kind = H_HK_QUERY;
    h->consumed = false;

    int result = H_lua_llm_wait_one(L, h);

    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_NOT_NULL(strstr(err, "not an LLM handle"));

    lua_pop(L, 2);
    free(h);
    lua_close(L);
}

/* H_lua_llm_wait_one with LLM handle but missing model name returns error */
void test_llm_wait_llm_handle_missing_model_name(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);

    H_Handle* h = create_test_llm_handle(NULL, NULL);

    int result = H_lua_llm_wait_one(L, h);

    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_NOT_NULL(strstr(err, "missing model name"));
    TEST_ASSERT_TRUE(h->consumed);

    lua_pop(L, 2);
    free(h);
    lua_close(L);
}

/* H_lua_llm_wait_one with model not found returns error */
void test_llm_wait_llm_model_not_found(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);

    /* Make lookup return NULL to trigger "not found" path */
    mock_chat_engine_cache_set_lookup_by_name_result(NULL);
    g_test_dbq.chat_engine_cache = NULL;

    H_Handle* h = create_test_llm_handle("nonexistent-model", "hello");

    int result = H_lua_llm_wait_one(L, h);

    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_NOT_NULL(strstr(err, "not found"));
    TEST_ASSERT_TRUE(h->consumed);

    lua_pop(L, 2);
    free(h->llm_model_name);
    free(h->llm_prompt);
    free(h);
    lua_close(L);
}

/* H_lua_llm_wait_one with llm_error returns error */
void test_llm_handle_with_llm_error_returns_error(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);

    H_Handle* h = create_test_llm_handle("test-model", "hello");
    h->llm_error = strdup("LLM error message");

    int result = H_lua_llm_wait_one(L, h);

    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    const char* err = lua_tostring(L, 2);
    TEST_ASSERT_EQUAL_STRING("LLM error message", err);
    TEST_ASSERT_TRUE(h->consumed);

    lua_pop(L, 2);
    free(h->llm_model_name);
    free(h->llm_prompt);
    free(h->llm_error);
    free(h);
    lua_close(L);
}

/* H_lua_llm_wait_one with llm_list returns models table */
void test_llm_wait_llm_list_returns_models_table(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    
    H_Handle* h = (H_Handle*)calloc(1, sizeof(H_Handle));
    TEST_ASSERT_NOT_NULL(h);
    h->kind = H_HK_LLM;
    h->consumed = false;
    h->llm_list = true;
    h->llm_model_name = strdup("test-model");
    
    int result = H_lua_llm_wait_one(L, h);
    
    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, 1));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, 2));
    
    lua_pop(L, 2);
    free(h->llm_model_name);
    free(h);
    lua_close(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_llm_wait_null_handle_returns_error);
    RUN_TEST(test_llm_wait_consumed_handle_returns_error);
    RUN_TEST(test_llm_wait_handle_with_pre_set_error);
    RUN_TEST(test_llm_wait_non_llm_handle_returns_error);
    RUN_TEST(test_llm_wait_llm_handle_missing_model_name);
    RUN_TEST(test_llm_wait_llm_model_not_found);
    RUN_TEST(test_llm_handle_with_llm_error_returns_error);
    RUN_TEST(test_llm_wait_llm_list_returns_models_table);

    return UNITY_END();
}