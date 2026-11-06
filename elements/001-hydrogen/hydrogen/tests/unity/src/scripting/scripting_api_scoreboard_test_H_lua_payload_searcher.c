/*
 * Unity Test File: H_lua_payload_searcher
 *
 * require loads lua/<module>.lua, then lua/<module>/init.lua, from the
 * in-memory payload cache. A payload file wins over a scripts-table row.
 * A compile error is not handed to the next searcher. A compressed
 * .br entry is not compiled as Lua. The searcher does not need
 * AllowDBModuleLoad.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdio.h>

#include <lua.h>
#include <lauxlib.h>

#include <src/payload/payload_cache.h>
#include <src/scripting/lua_context.h>
#include <src/scripting/scripting.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/source_cache.h>

#include "mock_logging.h"

void setUp(void);
void tearDown(void);
void test_payload_lua_path_accepts_dotted_names(void);
void test_payload_lua_path_rejects_escape(void);
void test_require_payload_module_without_db(void);
void test_payload_wins_over_scripts_row(void);
void test_payload_syntax_error_is_not_swallowed(void);
void test_payload_skips_brotli_entry(void);

static uint8_t tar_buf[8192];
static size_t tar_len;

void setUp(void) {
    mock_logging_reset_all();
    cleanup_payload_cache();
    scripting_source_cache = source_cache_create();
    memset(tar_buf, 0, sizeof(tar_buf));
    tar_len = 0;
}

void tearDown(void) {
    cleanup_payload_cache();
    source_cache_destroy(scripting_source_cache);
    scripting_source_cache = NULL;
}

static bool tar_add(const char *name, const char *body) {
    size_t size = strlen(body);
    size_t blocks = (size + 511) / 512;
    size_t need = 512 + (blocks * 512);
    uint8_t *header;
    char size_field[13];
    size_t name_len = strlen(name);

    if (tar_len + need + 512 >= sizeof(tar_buf)) return false;
    if (name_len == 0 || name_len >= 100) return false;
    header = tar_buf + tar_len;
    memset(header, 0, 512);
    memcpy(header, name, name_len);
    if (snprintf(size_field, sizeof(size_field), "%011zo", size) != 11) return false;
    memcpy(header + 124, size_field, 12);
    header[156] = '0';
    memcpy(header + 512, body, size);
    tar_len += need;
    return true;
}

static bool seed_library(void) {
    if (!tar_add("lua/zint/init.lua",
                 "local M = {}\n"
                 "function M.barcode(opts)\n"
                 "  return 'ok:' .. tostring(opts and opts.data or '')\n"
                 "end\n"
                 "return M\n")) return false;
    if (!tar_add("lua/zint/barcode.lua", "return require('zint').barcode\n")) return false;
    if (!tar_add("lua/Shadow/Mod.lua", "return 7\n")) return false;
    if (!tar_add("lua/broken.lua", "!!!\n")) return false;
    if (!tar_add("lua/skip.lua.br", "return 1\n")) return false;
    tar_len += 512;
    return parse_tar_into_cache(tar_buf, tar_len);
}

static lua_State *make_context(bool allow_db) {
    AppConfig *saved = app_config;
    static AppConfig mock;
    lua_State *L;

    memset(&mock, 0, sizeof(mock));
    mock.scripting.AllowDBModuleLoad = allow_db;
    app_config = &mock;
    L = H_lua_create_context();
    app_config = saved;
    return L;
}

void test_payload_lua_path_accepts_dotted_names(void) {
    char buf[256];
    const PayloadFile *file;

    TEST_ASSERT_TRUE(payload_lua_path("zint.barcode", buf, sizeof(buf), false));
    TEST_ASSERT_EQUAL_STRING("lua/zint/barcode.lua", buf);
    TEST_ASSERT_TRUE(payload_lua_path("zint", buf, sizeof(buf), true));
    TEST_ASSERT_EQUAL_STRING("lua/zint/init.lua", buf);
    TEST_ASSERT_TRUE(payload_lua_path("luatz.tzcache", buf, sizeof(buf), false));
    TEST_ASSERT_EQUAL_STRING("lua/luatz/tzcache.lua", buf);

    TEST_ASSERT_TRUE(seed_library());
    file = payload_cache_find("lua/zint/init.lua");
    TEST_ASSERT_NOT_NULL(file);
    TEST_ASSERT_FALSE(file->is_compressed);
    TEST_ASSERT_NOT_NULL(payload_lua_open("lua/zint/barcode.lua"));
    TEST_ASSERT_NULL(payload_lua_open("lua/skip.lua.br"));
    TEST_ASSERT_NULL(payload_lua_open("lua/missing.lua"));
}

void test_payload_lua_path_rejects_escape(void) {
    char buf[8];
    char long_name[182];

    TEST_ASSERT_FALSE(payload_lua_path(NULL, buf, sizeof(buf), false));
    TEST_ASSERT_FALSE(payload_lua_path("", buf, sizeof(buf), false));
    TEST_ASSERT_FALSE(payload_lua_path("..", buf, sizeof(buf), false));
    TEST_ASSERT_FALSE(payload_lua_path(".zint", buf, sizeof(buf), false));
    TEST_ASSERT_FALSE(payload_lua_path("zint.", buf, sizeof(buf), false));
    TEST_ASSERT_FALSE(payload_lua_path("1z", buf, sizeof(buf), false));
    TEST_ASSERT_FALSE(payload_lua_path("a/b", buf, sizeof(buf), false));
    TEST_ASSERT_FALSE(payload_lua_path("zint", buf, sizeof(buf), false));
    memset(long_name, 'a', 181);
    long_name[181] = '\0';
    TEST_ASSERT_FALSE(payload_lua_path(long_name, buf, sizeof(buf), false));
}

void test_require_payload_module_without_db(void) {
    lua_State *L;
    int rc;
    const char *code =
        "local fn = require('zint.barcode')\n"
        "local mod = require('zint')\n"
        "return fn({ data = '123' }) .. ':' .. tostring(mod.barcode == fn)\n";

    TEST_ASSERT_TRUE(seed_library());
    L = make_context(false);
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L, "return #package.searchers");
    TEST_ASSERT_EQUAL(LUA_OK, rc);
    TEST_ASSERT_EQUAL(5, lua_tointeger(L, -1));
    lua_pop(L, 1);

    rc = luaL_dostring(L, code);
    if (rc != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        TEST_FAIL_MESSAGE(err ? err : "require failed");
    }
    TEST_ASSERT_EQUAL_STRING("ok:123:true", lua_tostring(L, -1));
    H_lua_destroy_context(L);
}

void test_payload_wins_over_scripts_row(void) {
    lua_State *L;
    int rc;

    TEST_ASSERT_TRUE(seed_library());
    TEST_ASSERT_TRUE(source_cache_put(scripting_source_cache, "Shadow", "Mod", "return 99"));
    L = make_context(true);
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L,
        "local value, origin = require('Shadow.Mod')\n"
        "if origin ~= 'lua/Shadow/Mod.lua' then error(tostring(origin)) end\n"
        "return value\n");
    if (rc != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        TEST_FAIL_MESSAGE(err ? err : "require failed");
    }
    TEST_ASSERT_TRUE(lua_isinteger(L, -1));
    TEST_ASSERT_EQUAL(7, lua_tointeger(L, -1));
    H_lua_destroy_context(L);
}

void test_payload_syntax_error_is_not_swallowed(void) {
    lua_State *L;
    int rc;
    const char *err;
    const char *code =
        "local ok, message = pcall(require, 'broken')\n"
        "if ok then error('expected a compile error') end\n"
        "return tostring(message)\n";

    TEST_ASSERT_TRUE(seed_library());
    L = make_context(false);
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L, code);
    if (rc != LUA_OK) {
        err = lua_tostring(L, -1);
        TEST_FAIL_MESSAGE(err ? err : "pcall failed");
    }
    err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(err, "unexpected"), err);
    TEST_ASSERT_NULL_MESSAGE(strstr(err, "no file"), err);
    H_lua_destroy_context(L);
}

void test_payload_skips_brotli_entry(void) {
    lua_State *L;
    int rc;
    const char *err;
    const char *code =
        "local ok, message = pcall(require, 'skip')\n"
        "if ok then error('expected a miss') end\n"
        "return tostring(message)\n";

    TEST_ASSERT_TRUE(seed_library());
    L = make_context(false);
    TEST_ASSERT_NOT_NULL(L);
    rc = luaL_dostring(L, code);
    if (rc != LUA_OK) {
        err = lua_tostring(L, -1);
        TEST_FAIL_MESSAGE(err ? err : "pcall failed");
    }
    err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(err, "skip"), err);
    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_payload_lua_path_accepts_dotted_names);
    RUN_TEST(test_payload_lua_path_rejects_escape);
    RUN_TEST(test_require_payload_module_without_db);
    RUN_TEST(test_payload_wins_over_scripts_row);
    RUN_TEST(test_payload_syntax_error_is_not_swallowed);
    RUN_TEST(test_payload_skips_brotli_entry);
    return UNITY_END();
}
