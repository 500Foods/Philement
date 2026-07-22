/*
 * Unity Test File: scripting_api_test_query.c
 *
 * Phase 13 of the LUA_PLAN. Tests the H.query / H.altquery /
 * H.authquery / H.wait host functions:
 *   - H.query without app_config or DefaultDatabase returns an
 *     error handle
 *   - H.query with a NULL sql argument returns an error handle
 *   - H.altquery without a database name returns an error handle
 *   - H.authquery with an empty token returns an error handle
 *   - H.authquery with a malformed token returns an error handle
 *   - H.wait on an error handle returns nil + error string
 *   - H.wait on a consumed handle returns nil + error string
 *   - H.wait on a non-handle argument returns nil + error string
 *   - H.wait with no arguments returns nothing
 *
 * These tests exercise the error paths only; the success paths
 * require a live database and are covered by the blackbox test
 * (test_43_scripting.sh in a later sub-phase).
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>
#include <stdlib.h>

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Module under test
#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/lua_context.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_query_no_default_database_returns_error_handle(void);
void test_query_null_sql_returns_error_handle(void);
void test_query_returns_userdata_handle(void);
void test_altquery_empty_db_name_returns_error_handle(void);
void test_authquery_empty_token_returns_error_handle(void);
void test_authquery_malformed_token_returns_error_handle(void);
void test_wait_on_error_handle_returns_error(void);
void test_wait_on_consumed_handle_returns_error(void);
void test_wait_on_non_handle_returns_error(void);
void test_wait_with_no_args_returns_nothing(void);
void test_query_sync_propagates_error(void);
void test_query_with_params_table(void);
void test_query_with_integer_params(void);
void test_query_with_string_params(void);
void test_query_with_boolean_params(void);
void test_query_with_float_params(void);
void test_query_with_nil_params(void);

static lua_State* L = NULL;

void setUp(void) {
    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    H_Handle_install_metatable(L);
    // Create a minimal H table with the query functions installed.
    // We call the install function which expects H to exist as a
    // global; create it first.
    lua_newtable(L);
    lua_setglobal(L, "H");
    H_lua_install_query(L);
}

void tearDown(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
    app_config = NULL;
}

// Helper: get a global function from H
static void get_h_function(const char* name) {
    lua_getglobal(L, "H");
    lua_getfield(L, -1, name);
    lua_remove(L, -2); // remove H
}

// H.query with no DefaultDatabase returns an error handle
void test_query_no_default_database_returns_error_handle(void) {
    // app_config is NULL; H.query should return an error handle
    get_h_function("query");
    lua_pushstring(L, "SELECT 1");
    int rc = lua_pcall(L, 1, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    // The result is a userdata (handle)
    TEST_ASSERT_NOT_NULL(lua_touserdata(L, -1));
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);
    lua_pop(L, 1);
}

// H.query with NULL sql returns an error handle
void test_query_null_sql_returns_error_handle(void) {
    get_h_function("query");
    // Push nothing for the sql arg; luaL_checkstring would raise
    // a Lua error. Our H_lua_query checks nargs < 1 and returns
    // an error handle.
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);
    lua_pop(L, 1);
}

// H.query returns a userdata handle (even on error)
void test_query_returns_userdata_handle(void) {
    get_h_function("query");
    lua_pushstring(L, "SELECT * FROM users");
    int rc = lua_pcall(L, 1, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    // Must be a userdata
    TEST_ASSERT_NOT_NULL(lua_touserdata(L, -1));
    lua_pop(L, 1);
}

// H.altquery with empty database name returns an error handle
void test_altquery_empty_db_name_returns_error_handle(void) {
    get_h_function("altquery");
    lua_pushstring(L, "");   // empty db name
    lua_pushstring(L, "SELECT 1");
    int rc = lua_pcall(L, 2, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);
    lua_pop(L, 1);
}

// H.authquery with empty token returns an error handle
void test_authquery_empty_token_returns_error_handle(void) {
    get_h_function("authquery");
    lua_pushstring(L, "");   // empty token
    lua_pushstring(L, "SELECT 1");
    int rc = lua_pcall(L, 2, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);
    lua_pop(L, 1);
}

// H.authquery with a malformed token returns an error handle
void test_authquery_malformed_token_returns_error_handle(void) {
    get_h_function("authquery");
    lua_pushstring(L, "not.a.valid.jwt.token");
    lua_pushstring(L, "SELECT 1");
    int rc = lua_pcall(L, 2, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);
    lua_pop(L, 1);
}

// H.wait on an error handle returns nil + error string
void test_wait_on_error_handle_returns_error(void) {
    // Create an error handle via H.query
    get_h_function("query");
    lua_pushstring(L, "SELECT 1");
    int rc = lua_pcall(L, 1, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);

    // Now call H.wait on this handle
    get_h_function("wait");
    // The handle is on the stack; pass it to wait
    lua_insert(L, -2); // move wait function above the handle
    rc = lua_pcall(L, 1, 2, 0); // wait takes 1 arg, returns 2
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    // Result is nil
    TEST_ASSERT_NULL(lua_touserdata(L, -2));
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    // Error is a string
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_GREATER_THAN(0, strlen(err));
    lua_pop(L, 2);

    // Handle should now be consumed
    TEST_ASSERT_TRUE(h->consumed);
}

// H.wait on a consumed handle returns "already consumed"
void test_wait_on_consumed_handle_returns_error(void) {
    get_h_function("query");
    lua_pushstring(L, "SELECT 1");
    int rc = lua_pcall(L, 1, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);

    // Keep a Lua reference to the handle so it is not GC'd between
    // the two waits. We store it in a Lua global.
    lua_setglobal(L, "_test_handle_ref");

    // First wait: consumes the handle (returns the original error)
    get_h_function("wait");
    lua_getglobal(L, "_test_handle_ref");
    rc = lua_pcall(L, 1, 2, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    const char* err1 = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(err1);
    lua_pop(L, 2);

    // Verify the handle was consumed
    TEST_ASSERT_TRUE(h->consumed);

    // Second wait: handle is consumed, should return "already consumed"
    get_h_function("wait");
    lua_getglobal(L, "_test_handle_ref");
    rc = lua_pcall(L, 1, 2, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    const char* err2 = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(err2);
    TEST_ASSERT_NOT_NULL(strstr(err2, "consumed"));
    lua_pop(L, 2);

    // Clean up the global
    lua_pushnil(L);
    lua_setglobal(L, "_test_handle_ref");
}

// H.wait on a non-handle argument returns "not a valid handle"
void test_wait_on_non_handle_returns_error(void) {
    get_h_function("wait");
    lua_pushstring(L, "not a handle");
    int rc = lua_pcall(L, 1, 2, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(strstr(err, "not a valid handle"));
    lua_pop(L, 2);
}

// H.wait with no arguments returns nothing
void test_wait_with_no_args_returns_nothing(void) {
    get_h_function("wait");
    int rc = lua_pcall(L, 0, 0, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    TEST_ASSERT_EQUAL(0, lua_gettop(L));
}

// H.query_sync propagates the error from the handle
void test_query_sync_propagates_error(void) {
    get_h_function("query_sync");
    lua_pushstring(L, "SELECT 1");
    int rc = lua_pcall(L, 1, 2, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    // Result should be nil, error should be a string
    TEST_ASSERT_EQUAL(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL(LUA_TSTRING, lua_type(L, -1));
    lua_pop(L, 2);
}

// H.query with an empty params table exercises H_lua_params_to_json with no iterations
void test_query_with_params_table(void) {
    get_h_function("query");
    lua_pushstring(L, "SELECT 1");
    lua_newtable(L);  // empty params table
    int rc = lua_pcall(L, 2, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);
    lua_pop(L, 1);
}

// H.query with integer params exercises H_lua_params_to_json integer path
void test_query_with_integer_params(void) {
    get_h_function("query");
    lua_pushstring(L, "SELECT 1");
    lua_newtable(L);
    lua_pushinteger(L, 42);
    lua_setfield(L, -2, "id");
    int rc = lua_pcall(L, 2, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);
    lua_pop(L, 1);
}

// H.query with string params exercises H_lua_params_to_json string path
void test_query_with_string_params(void) {
    get_h_function("query");
    lua_pushstring(L, "SELECT 1");
    lua_newtable(L);
    lua_pushstring(L, "test_value");
    lua_setfield(L, -2, "name");
    int rc = lua_pcall(L, 2, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);
    lua_pop(L, 1);
}

// H.query with boolean params exercises H_lua_params_to_json boolean path
void test_query_with_boolean_params(void) {
    get_h_function("query");
    lua_pushstring(L, "SELECT 1");
    lua_newtable(L);
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "active");
    int rc = lua_pcall(L, 2, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);
    lua_pop(L, 1);
}

// H.query with float params exercises H_lua_params_to_json real path
void test_query_with_float_params(void) {
    get_h_function("query");
    lua_pushstring(L, "SELECT 1");
    lua_newtable(L);
    lua_pushnumber(L, 3.14159);
    lua_setfield(L, -2, "price");
    int rc = lua_pcall(L, 2, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);
    lua_pop(L, 1);
}

// H.query with nil value in params exercises H_lua_params_to_json null path
void test_query_with_nil_params(void) {
    get_h_function("query");
    lua_pushstring(L, "SELECT 1");
    lua_newtable(L);
    lua_pushnil(L);
    lua_setfield(L, -2, "optional");
    int rc = lua_pcall(L, 2, 1, 0);
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_NOT_NULL(h->error);
    lua_pop(L, 1);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_query_no_default_database_returns_error_handle);
    RUN_TEST(test_query_null_sql_returns_error_handle);
    RUN_TEST(test_query_returns_userdata_handle);
    RUN_TEST(test_altquery_empty_db_name_returns_error_handle);
    RUN_TEST(test_authquery_empty_token_returns_error_handle);
    RUN_TEST(test_authquery_malformed_token_returns_error_handle);
    RUN_TEST(test_wait_on_error_handle_returns_error);
    RUN_TEST(test_wait_on_consumed_handle_returns_error);
    RUN_TEST(test_wait_on_non_handle_returns_error);
    RUN_TEST(test_wait_with_no_args_returns_nothing);
    RUN_TEST(test_query_sync_propagates_error);
    RUN_TEST(test_query_with_params_table);
    RUN_TEST(test_query_with_integer_params);
    RUN_TEST(test_query_with_string_params);
    RUN_TEST(test_query_with_boolean_params);
    RUN_TEST(test_query_with_float_params);
    RUN_TEST(test_query_with_nil_params);

    return UNITY_END();
}
