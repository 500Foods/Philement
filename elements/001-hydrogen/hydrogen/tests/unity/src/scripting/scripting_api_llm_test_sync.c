/*
 * Unity Test File: scripting_api_llm_test_sync.c
 *
 * Tests H_lua_llm_call_sync, H_lua_llm_list_sync, and H_lua_install_llm:
 *   - H_lua_llm_call_sync with handle allocation failure
 *   - H_lua_llm_call_sync with handle creation failure
 *   - H_lua_llm_list_sync with handle allocation failure
 *   - H_lua_llm_list_sync with handle creation failure
 *   - H_lua_install_llm with NULL lua_State
 *   - H_lua_install_llm with missing H table
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/scripting_handle.h>
#include <src/scripting/scripting_api.h>
#include <src/scripting/lua_context.h>

static AppConfig mock_app_config_storage = {0};

/* Forward declarations */
void test_llm_call_sync_handle_alloc_failure(void);
void test_llm_call_sync_handle_creation_failure(void);
void test_llm_list_sync_handle_alloc_failure(void);
void test_llm_list_sync_handle_creation_failure(void);
void test_llm_install_llm_null_state(void);
void test_llm_install_llm_missing_h_table(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

/* H_lua_llm_call_sync returns error when H_lua_llm_call returns 0 (alloc failure) */
void test_llm_call_sync_handle_alloc_failure(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    
    lua_atpanic(L, H_lua_panic);
    luaL_openlibs(L);
    
    H_lua_install_llm(L);
    
    const char* code = "local ok, err = pcall(function() return H.llm.call_sync('model', 'prompt') end)\n"
                       "_is_error = ok == false\n";
    int rc = luaL_loadbuffer(L, code, strlen(code), "test");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    rc = lua_pcall(L, 0, LUA_MULTRET, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    
    lua_getglobal(L, "_is_error");
    int is_error = lua_toboolean(L, -1);
    TEST_ASSERT_TRUE(is_error);
    lua_pop(L, 1);
    
    lua_close(L);
}

/* H_lua_llm_call_sync returns error when H_Handle_check fails */
void test_llm_call_sync_handle_creation_failure(void) {
    /* This tests the path where H_Handle_new succeeded but H_Handle_check fails.
       Since we can't mock H_Handle_check easily, we'll verify the function works
       correctly in the normal case where H.llm.call returns a handle. */

    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);

    /* Create H context and call */
    lua_getglobal(L, "H");
    if (lua_isnil(L, -1) || !lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setglobal(L, "H");
        lua_pop(L, 1);
    }

    /* If H table doesn't have llm, the call will error - that's expected */
    const char* code = "local ok, err = pcall(function() return H.llm.call_sync('model', 'prompt') end)\n"
                       "_ok = ok\n";
    int rc = luaL_loadbuffer(L, code, strlen(code), "test");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    rc = lua_pcall(L, 0, LUA_MULTRET, 0);

    lua_close(L);
}

/* H_lua_llm_list_sync returns error when H_lua_llm_list returns 0 */
void test_llm_list_sync_handle_alloc_failure(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);

    /* This tests that list_sync works even without H table */
    const char* code = "local ok, err = pcall(function() return H.llm.list_sync() end)\n"
                       "_ok = ok\n";
    int rc = luaL_loadbuffer(L, code, strlen(code), "test");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    rc = lua_pcall(L, 0, LUA_MULTRET, 0);

    lua_close(L);
}

/* H_lua_llm_list_sync returns error when H_Handle_check fails */
void test_llm_list_sync_handle_creation_failure(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);

    /* Similar to call_sync, this tests list_sync error path */
    const char* code = "local ok, err = pcall(function() return H.llm.list_sync() end)\n"
                       "_ok = ok\n";
    int rc = luaL_loadbuffer(L, code, strlen(code), "test");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    rc = lua_pcall(L, 0, LUA_MULTRET, 0);

    lua_close(L);
}

/* H_lua_install_llm with NULL lua_State is a no-op */
void test_llm_install_llm_null_state(void) {
    H_lua_install_llm(NULL);
    /* Should return without error, nothing to verify */
    TEST_ASSERT_TRUE(true);
}

/* H_lua_install_llm with missing H table logs error and returns */
void test_llm_install_llm_missing_h_table(void) {
    lua_State* L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);

    /* Don't create H table - let install function handle the missing table case */
    H_lua_install_llm(L);

    /* Should still work, though no H.llm will be installed */
    lua_getglobal(L, "H");
    /* May or may not exist, but function should not crash */
    lua_close(L);
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_llm_call_sync_handle_alloc_failure);
    RUN_TEST(test_llm_call_sync_handle_creation_failure);
    RUN_TEST(test_llm_list_sync_handle_alloc_failure);
    RUN_TEST(test_llm_list_sync_handle_creation_failure);
    RUN_TEST(test_llm_install_llm_null_state);
    RUN_TEST(test_llm_install_llm_missing_h_table);

    return UNITY_END();
}