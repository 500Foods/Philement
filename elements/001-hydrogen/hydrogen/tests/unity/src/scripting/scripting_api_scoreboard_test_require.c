/*
 * Unity Test File: scripting_api_test_require.c
 *
 * Phase 11g and Phase 21 of the LUA_PLAN. Validates the DB-backed `require`
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
 * Phase 21: Bytecode Caching tests:
 *   - When bytecode is cached, it is used (mode "b") instead of source.
 *   - After source compilation, bytecode is cached for future use.
 *   - Bytecode and source can coexist; bytecode takes precedence.
 *   - Replacing source clears bytecode (forces recompilation).
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
 void test_require_uses_bytecode_when_cached(void);
 void test_require_bytecode_takes_precedence_over_source(void);
 void test_require_clears_bytecode_on_source_replace(void);

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

 // Bytecode dump helper: collects Lua bytecode into a growable buffer
 // via lua_dump. Used to generate real bytecode for the precedence test.
 typedef struct {
     char*  data;
     size_t len;
     size_t cap;
 } DumpCtx;

 static int dump_writer_fn(lua_State* L, const void* data, size_t size, void* ud) {
     (void)L;
     DumpCtx* ctx = (DumpCtx*)ud;
     if (ctx->len + size > ctx->cap) {
         size_t new_cap = ctx->cap ? ctx->cap * 2 : 256;
         while (new_cap < ctx->len + size) {
             new_cap *= 2;
         }
         char* new_data = realloc(ctx->data, new_cap);
         if (!new_data) {
             return 1;
         }
         ctx->data = new_data;
         ctx->cap = new_cap;
     }
     memcpy(ctx->data + ctx->len, data, size);
     ctx->len += size;
     return 0;
 }

 void test_require_uses_bytecode_when_cached(void) {
     lua_State* L = make_context(true);
     TEST_ASSERT_NOT_NULL(L);

     TEST_ASSERT_TRUE(source_cache_put(scripting_source_cache,
                                       "BytecodeTest", "Module",
                                       "return 123"));

     const char* code =
         "local ok, result = pcall(require, 'BytecodeTest.Module')\n"
         "if not ok then error(result) end\n"
         "return result\n";

     // First require: compiles from source, searcher dumps bytecode to cache
     int rc = luaL_dostring(L, code);
     TEST_ASSERT_EQUAL(LUA_OK, rc);
     TEST_ASSERT_TRUE(lua_isinteger(L, -1));
     TEST_ASSERT_EQUAL(123, lua_tointeger(L, -1));
     lua_pop(L, 1);

     // Verify bytecode was cached by the searcher
     size_t bytecode_len = 0;
     const void* bytecode = source_cache_get_bytecode(
         scripting_source_cache, "BytecodeTest", "Module", &bytecode_len);
     TEST_ASSERT_NOT_NULL(bytecode);
     TEST_ASSERT_GREATER_THAN(0, (int)bytecode_len);

     // Second require: searcher loads from cached bytecode path (mode "b")
     // New state exercises the bytecode-first lookup in the searcher
     H_lua_destroy_context(L);
     L = make_context(true);
     TEST_ASSERT_NOT_NULL(L);

     rc = luaL_dostring(L, code);
     TEST_ASSERT_EQUAL(LUA_OK, rc);
     TEST_ASSERT_TRUE(lua_isinteger(L, -1));
     TEST_ASSERT_EQUAL(123, lua_tointeger(L, -1));

     H_lua_destroy_context(L);
 }

 void test_require_bytecode_takes_precedence_over_source(void) {
     lua_State* L = make_context(true);
     TEST_ASSERT_NOT_NULL(L);

     // Generate real bytecode for "return 123" via a temp standalone state
     lua_State* tmp = luaL_newstate();
     TEST_ASSERT_NOT_NULL(tmp);
     TEST_ASSERT_EQUAL(LUA_OK, luaL_loadstring(tmp, "return 123"));

     DumpCtx ctx = {0};
     TEST_ASSERT_EQUAL(LUA_OK, lua_dump(tmp, dump_writer_fn, &ctx, 0));
     lua_close(tmp);

     // Source says "return 999"; cached bytecode says "return 123"
     TEST_ASSERT_TRUE(source_cache_put(scripting_source_cache,
                                       "PrecTest", "Module",
                                       "return 999"));
     TEST_ASSERT_TRUE(source_cache_put_bytecode(scripting_source_cache,
                                                "PrecTest", "Module",
                                                ctx.data, ctx.len));
     free(ctx.data);

     // Searcher should hit cached bytecode first (mode "b") and return 123
     const char* code =
         "local ok, result = pcall(require, 'PrecTest.Module')\n"
         "if not ok then error(result) end\n"
         "return result\n";
     int rc = luaL_dostring(L, code);
     TEST_ASSERT_EQUAL(LUA_OK, rc);
     TEST_ASSERT_TRUE(lua_isinteger(L, -1));
     TEST_ASSERT_EQUAL(123, lua_tointeger(L, -1));

     H_lua_destroy_context(L);
 }

 void test_require_clears_bytecode_on_source_replace(void) {
     lua_State* L = make_context(true);
     TEST_ASSERT_NOT_NULL(L);

     // Store source and a small (intentionally invalid) bytecode blob
     TEST_ASSERT_TRUE(source_cache_put(scripting_source_cache,
                                       "ClearTest", "Module",
                                       "return 1"));
     uint8_t garbage[] = {0xAA, 0xBB};
     TEST_ASSERT_TRUE(source_cache_put_bytecode(scripting_source_cache,
                                                "ClearTest", "Module",
                                                garbage, sizeof(garbage)));

     size_t bytecode_len = 0;
     const void* bytecode = source_cache_get_bytecode(
         scripting_source_cache, "ClearTest", "Module", &bytecode_len);
     TEST_ASSERT_NOT_NULL(bytecode);
     TEST_ASSERT_GREATER_THAN(0, (int)bytecode_len);

     // Replace source: put() clears the cached bytecode
     TEST_ASSERT_TRUE(source_cache_put(scripting_source_cache,
                                       "ClearTest", "Module",
                                       "return 2"));

     bytecode = source_cache_get_bytecode(
         scripting_source_cache, "ClearTest", "Module", &bytecode_len);
     TEST_ASSERT_NULL(bytecode);
     TEST_ASSERT_EQUAL(0, bytecode_len);

     const char* source = source_cache_get(
         scripting_source_cache, "ClearTest", "Module");
     TEST_ASSERT_NOT_NULL(source);
     TEST_ASSERT_EQUAL_STRING("return 2", source);

     H_lua_destroy_context(L);
 }

int main(void) {
     UNITY_BEGIN();

     RUN_TEST(test_searcher_installed_when_allowed);
     RUN_TEST(test_searcher_not_installed_when_denied);
     RUN_TEST(test_require_cache_hit_returns_module_value);
     RUN_TEST(test_require_missing_returns_error);
     RUN_TEST(test_require_invalid_name_returns_error);
     RUN_TEST(test_require_uses_bytecode_when_cached);
     RUN_TEST(test_require_bytecode_takes_precedence_over_source);
     RUN_TEST(test_require_clears_bytecode_on_source_replace);

     return UNITY_END();
 }
