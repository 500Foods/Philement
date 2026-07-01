/*
 * Unity Test File: lua_context_test_create_destroy.c
 *
 * Phase 3 of the LUA_PLAN. Tests the H_lua_create_context /
 * H_lua_destroy_context lifecycle, including the sandbox policy
 * and the basic H table install.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Lua headers (needed for minimal scripting in the test)
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Module under test
#include <src/scripting/lua_context.h>

// Mock app_config (zeroed = safest defaults for this test).
// The sandbox policy reads app_config->scripting.Sandbox; when it is
// not set the create-path falls back to conservative defaults.
static AppConfig mock_app_config_storage = {0};

// Forward declarations
void test_lua_create_context_returns_non_null(void);
void test_lua_destroy_context_null_is_safe(void);
void test_lua_create_and_destroy_roundtrip(void);
void test_lua_create_context_installs_H_table(void);

void setUp(void) {
    // Start each test with a zeroed config
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

// H_lua_create_context must return a usable state
void test_lua_create_context_returns_non_null(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    H_lua_destroy_context(L);
}

// H_lua_destroy_context(NULL) must be a no-op
void test_lua_destroy_context_null_is_safe(void) {
    H_lua_destroy_context(NULL);
    TEST_PASS();
}

// Full round-trip: create, run a minimal script, destroy
void test_lua_create_and_destroy_roundtrip(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    // Run a minimal script that assigns a global
    const char* script = "phase3_ok = true";
    int load_rc = luaL_loadbuffer(L, script, strlen(script), "test");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, load_rc, "luaL_loadbuffer failed");

    int call_rc = lua_pcall(L, 0, 0, 0);
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, call_rc, "lua_pcall failed");

    lua_getglobal(L, "phase3_ok");
    TEST_ASSERT_TRUE(lua_isboolean(L, -1));
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// Phase 3 installs an H table with placeholder sub-tables. Validate
// each of the agreed sub-table names is present.
void test_lua_create_context_installs_H_table(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    TEST_ASSERT_TRUE(lua_istable(L, -1));

    static const char* subtables[] = {
        "log", "system",
        "query", "altquery", "authquery", "wait",
        "http", "llm", "mail", "notify",
        "sleep", "shutdown_requested", "set_current_state",
        "scoreboard",
        NULL
    };

    for (int i = 0; subtables[i] != NULL; i++) {
        lua_getfield(L, -1, subtables[i]);
        TEST_ASSERT_TRUE_MESSAGE(lua_istable(L, -1), subtables[i]);
        lua_pop(L, 1);
    }

    lua_pop(L, 1); // pop H table
    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_lua_create_context_returns_non_null);
    RUN_TEST(test_lua_destroy_context_null_is_safe);
    RUN_TEST(test_lua_create_and_destroy_roundtrip);
    RUN_TEST(test_lua_create_context_installs_H_table);

    return UNITY_END();
}
