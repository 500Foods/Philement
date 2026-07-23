/*
 * Unity Test File: scripting_api_query_wait_test_sync_wrappers
 *
 * Tests uncovered lines in scripting_api_query_wait.c for sync wrappers.
 * Covers: query_sync, altquery_sync, authquery_sync error propagation.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <unity/mocks/mock_dbqueue.h>
#include <unity/mocks/mock_generate_query_id.h>
#include <unity/mocks/mock_auth_service_jwt.h>

#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/lua_context.h>
#include <src/api/auth/auth_service.h>

extern DatabaseQueueManager* global_queue_manager;

void test_H_lua_query_sync_propagates_error(void);
void test_H_lua_altquery_sync_propagates_error(void);
void test_H_lua_authquery_sync_propagates_error(void);

static lua_State* L = NULL;
static DatabaseQueue test_db_queue;
static DatabaseQueueManager test_dqm;

void setUp(void) {
    mock_dbqueue_reset_all();
    mock_generate_query_id_reset();
    mock_auth_service_jwt_reset_all();

    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);

    lua_newtable(L);
    lua_setglobal(L, "H");

    memset(&test_db_queue, 0, sizeof(test_db_queue));
    memset(&test_dqm, 0, sizeof(test_dqm));
}

void tearDown(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
    app_config = NULL;
    global_queue_manager = NULL;
}

void test_H_lua_query_sync_propagates_error(void) {
    int n = H_lua_query_sync(L);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    lua_pop(L, 2);
}

void test_H_lua_altquery_sync_propagates_error(void) {
    lua_pushstring(L, "testdb");
    lua_pushstring(L, "SELECT 1");
    int n = H_lua_altquery_sync(L);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    lua_pop(L, 2);
}

void test_H_lua_authquery_sync_propagates_error(void) {
    jwt_claims_t claims = {0};
    claims.database = strdup("testdb");

    jwt_validation_result_t result = {0};
    result.valid = true;
    result.claims = &claims;

    mock_auth_service_jwt_set_validation_result(result);

    lua_pushstring(L, "valid.jwt.token");
    lua_pushstring(L, "SELECT 1");
    int n = H_lua_authquery_sync(L);
    TEST_ASSERT_EQUAL_INT(2, n);

    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    lua_pop(L, 2);

    free(claims.database);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_H_lua_query_sync_propagates_error);
    RUN_TEST(test_H_lua_altquery_sync_propagates_error);
    RUN_TEST(test_H_lua_authquery_sync_propagates_error);

    return UNITY_END();
}