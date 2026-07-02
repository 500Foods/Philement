/*
 * Unity Test File: scripting_api_test_require.c
 *
 * Phase 11g of the LUA_PLAN. Validates the DB-backed `require`
 * package.searchers hook without requiring a live database:
 *
 *   - The searcher is installed iff AllowDBModuleLoad is true.
 *   - require("group.script") resolves a cache hit to a compiled chunk.
 *   - require("Missing.Script") with an empty cache and no DB returns
 *     a clear "module not found" error.
 *   - require("NoDot") returns a clear "invalid name" error.
 *   - package.searchers length changes by exactly one when the hook
 *     is installed.
 *
 * The DB-fetch path (cache miss that actually hits the database) is
 * exercised by the blackbox test test_43_scripting.sh (Phase 11h);
 * this Unity file covers the in-memory/cache-only paths.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>

// Third-party includes
#include <lua.h>
#include <lauxlib.h>

// Modules under test
#include <src/scripting/lua_context.h>
#include <src/scripting/scripting.h>
#include <src/scripting/source_cache.h>

// Mock log helpers
#include "mock_logging.h"

// Forward declarations (required for -Wmissing-prototypes)
void test_searcher_installed_when_allowed(void);
void test_searcher_not_installed_when_denied(void);
void test_require_cache_hit_returns_module_value(void);
void test_require_missing_returns_error(void);
void test_require_invalid_name_returns_error(void);

void setUp(void) {
    mock_logging_reset_all();
    scripting_source_cache = source_cache_create();
}

void tearDown(void) {
    source_cache_destroy(scripting_source_cache);
    scripting_source_cache = NULL;
}

// Create a Lua context with the given AllowDBModuleLoad setting.
// Saves/restores app_config around the call.
static lua_State* make_context(bool allow_db_module_load) {
    AppConfig* saved = app_config;
    static AppConfig mock;
    memset(&mock, 0, sizeof(mock));
    mock.scripting.AllowDBModuleLoad = allow_db_module_load;
    app_config = &mock;

    lua_State* L = H_lua_create_context();

    app_config = saved;
    return L;
}

void test_searcher_installed_when_allowed(void) {
    lua_State* L = make_context(true);
    TEST_ASSERT_NOT_NULL(L);

    int rc = luaL_dostring(L, "return #package.searchers");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    TEST_ASSERT_TRUE(lua_isinteger(L, -1));
    lua_Integer count = lua_tointeger(L, -1);
    TEST_ASSERT_EQUAL(5, count); // 4 default + our DB-backed searcher

    H_lua_destroy_context(L);
}

void test_searcher_not_installed_when_denied(void) {
    lua_State* L = make_context(false);
    TEST_ASSERT_NOT_NULL(L);

    int rc = luaL_dostring(L, "return #package.searchers");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    TEST_ASSERT_TRUE(lua_isinteger(L, -1));
    lua_Integer count = lua_tointeger(L, -1);
    TEST_ASSERT_EQUAL(4, count); // default searchers only

    H_lua_destroy_context(L);
}

void test_require_cache_hit_returns_module_value(void) {
    lua_State* L = make_context(true);
    TEST_ASSERT_NOT_NULL(L);

    TEST_ASSERT_TRUE(source_cache_put(scripting_source_cache,
                                      "TestGroup", "TestScript",
                                      "return 42"));

    const char* code =
        "local ok, mod = pcall(require, 'TestGroup.TestScript')\n"
        "if not ok then error(mod) end\n"
        "return mod\n";
    int rc = luaL_dostring(L, code);
    if (rc != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        TEST_FAIL_MESSAGE(err ? err : "require failed");
    }
    TEST_ASSERT_TRUE(lua_isinteger(L, -1));
    TEST_ASSERT_EQUAL(42, lua_tointeger(L, -1));

    H_lua_destroy_context(L);
}

void test_require_missing_returns_error(void) {
    lua_State* L = make_context(true);
    TEST_ASSERT_NOT_NULL(L);

    // Ensure DefaultDatabase is empty so the fetch path returns quickly.
    AppConfig* saved = app_config;
    static AppConfig mock;
    memset(&mock, 0, sizeof(mock));
    mock.scripting.AllowDBModuleLoad = true;
    app_config = &mock;

    const char* code =
        "local ok, err = pcall(require, 'Missing.Script')\n"
        "if ok then error('expected require to fail') end\n"
        "return err\n";
    int rc = luaL_dostring(L, code);

    app_config = saved;

    if (rc != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        TEST_FAIL_MESSAGE(err ? err : "pcall(require) raised unexpectedly");
    }
    TEST_ASSERT_TRUE(lua_isstring(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(err, "Missing.Script"), err);

    H_lua_destroy_context(L);
}

void test_require_invalid_name_returns_error(void) {
    lua_State* L = make_context(true);
    TEST_ASSERT_NOT_NULL(L);

    const char* code =
        "local ok, err = pcall(require, 'NoDot')\n"
        "if ok then error('expected require to fail') end\n"
        "return err\n";
    int rc = luaL_dostring(L, code);
    if (rc != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        TEST_FAIL_MESSAGE(err ? err : "pcall(require) raised unexpectedly");
    }
    TEST_ASSERT_TRUE(lua_isstring(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(err, "invalid name"), err);

    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_searcher_installed_when_allowed);
    RUN_TEST(test_searcher_not_installed_when_denied);
    RUN_TEST(test_require_cache_hit_returns_module_value);
    RUN_TEST(test_require_missing_returns_error);
    RUN_TEST(test_require_invalid_name_returns_error);

    return UNITY_END();
}
