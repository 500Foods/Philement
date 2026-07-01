/*
 * Unity Test File: scripting_api_test_system.c
 *
 * Phase 6 of the LUA_PLAN. Tests H.system.{uptime, now, now_iso,
 * instance_id, version} - the five utility functions exposed via the
 * H.system sub-table.
 *
 * Validates:
 *   - uptime() returns a non-negative number
 *   - now() returns a reasonable epoch value (post-Unix-epoch, pre-2100)
 *   - now_iso() returns an ISO 8601 UTC string with 'T' and 'Z'
 *   - instance_id() returns a 36-char UUID-shaped string
 *   - instance_id() returns the same value across multiple calls
 *   - version() returns the VERSION macro as a string
 *   - All five functions can be called from a Lua chunk through
 *     H_lua_run_string
 *   - Each function leaves exactly one value on the stack
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Lua headers
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Module under test
#include <src/scripting/lua_context.h>
#include <src/scripting/scripting_api.h>

// Mock app_config
static AppConfig mock_app_config_storage = {0};

// Forward declarations
void test_system_uptime_is_nonnegative(void);
void test_system_now_is_reasonable_epoch(void);
void test_system_now_iso_has_t_and_z(void);
void test_system_instance_id_is_uuid_shaped(void);
void test_system_instance_id_is_stable_across_calls(void);
void test_system_version_returns_version_macro(void);
void test_system_functions_via_lua_chunk(void);
void test_system_functions_each_return_one_value(void);
void test_system_uptime_does_not_fail_when_start_unset(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

// uptime() returns a non-negative number. In a Unity test the server start
// time may or may not be set; if not, the function returns 0 (no negative
// duration). Either way it's a valid number, not NaN, not nil.
void test_system_uptime_is_nonnegative(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_system(L);
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "system");
    lua_getfield(L, -1, "uptime");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "uptime should succeed");
    TEST_ASSERT_TRUE(lua_isnumber(L, -1));
    lua_Number v = lua_tonumber(L, -1);
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, v);
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// now() returns an epoch second value in a reasonable range.
void test_system_now_is_reasonable_epoch(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_system(L);
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "system");
    lua_getfield(L, -1, "now");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "now should succeed");
    TEST_ASSERT_TRUE(lua_isnumber(L, -1));
    lua_Number v = lua_tonumber(L, -1);
    // Sanity: epoch seconds should be >= 1,000,000,000 (2001-09-09)
    // and < 4,000,000,000 (2096-08-10).
    TEST_ASSERT_GREATER_OR_EQUAL(1.0e9, v);
    TEST_ASSERT_LESS_THAN(4.0e9, v);
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// now_iso() returns an ISO 8601 UTC string ending in 'Z' with a 'T' separator.
void test_system_now_iso_has_t_and_z(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_system(L);
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "system");
    lua_getfield(L, -1, "now_iso");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "now_iso should succeed");
    TEST_ASSERT_TRUE(lua_isstring(L, -1));
    const char* iso = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(iso);
    TEST_ASSERT_NOT_NULL(strstr(iso, "T"));
    TEST_ASSERT_NOT_NULL(strstr(iso, "Z"));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// instance_id() returns a 36-char string with the UUID v4 shape
// (8-4-4-4-12 in lowercase hex with dashes).
void test_system_instance_id_is_uuid_shaped(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_system(L);
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "system");
    lua_getfield(L, -1, "instance_id");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "instance_id should succeed");
    TEST_ASSERT_TRUE(lua_isstring(L, -1));
    const char* id = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(id);
    TEST_ASSERT_EQUAL_INT_MESSAGE(36, (int)strlen(id),
        "UUID must be 36 chars (8-4-4-4-12)");

    // Structural check: dashes at indexes 8, 13, 18, 23
    TEST_ASSERT_EQUAL_CHAR('-', id[8]);
    TEST_ASSERT_EQUAL_CHAR('-', id[13]);
    TEST_ASSERT_EQUAL_CHAR('-', id[18]);
    TEST_ASSERT_EQUAL_CHAR('-', id[23]);

    // Version (index 14) is '4'.
    TEST_ASSERT_EQUAL_CHAR('4', id[14]);

    // Variant (index 19) is one of '8', '9', 'a', 'b'.
    TEST_ASSERT_TRUE(id[19] == '8' || id[19] == '9' ||
                     id[19] == 'a' || id[19] == 'b');

    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

// instance_id() must return the same value across multiple calls and
// even across different Lua contexts, since it is cached at process
// lifetime. Two calls from different contexts share the cache.
void test_system_instance_id_is_stable_across_calls(void) {
    lua_State* L1 = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L1);
    lua_State* L2 = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L2);

    H_lua_install_system(L1);
    H_lua_install_system(L2);

    lua_getglobal(L1, "H");
    lua_getfield(L1, -1, "system");
    lua_getfield(L1, -1, "instance_id");
    int rc1 = lua_pcall(L1, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc1, "instance_id from L1 should succeed");
    TEST_ASSERT_TRUE(lua_isstring(L1, -1));
    const char* id1 = lua_tostring(L1, -1);

    lua_getglobal(L2, "H");
    lua_getfield(L2, -1, "system");
    lua_getfield(L2, -1, "instance_id");
    int rc2 = lua_pcall(L2, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc2, "instance_id from L2 should succeed");
    TEST_ASSERT_TRUE(lua_isstring(L2, -1));
    const char* id2 = lua_tostring(L2, -1);

    TEST_ASSERT_EQUAL_STRING_MESSAGE(id1, id2,
        "instance_id must be stable across two contexts");

    H_lua_destroy_context(L1);
    H_lua_destroy_context(L2);
}

// version() returns the VERSION macro value.
void test_system_version_returns_version_macro(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_system(L);
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "system");
    lua_getfield(L, -1, "version");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "version should succeed");
    TEST_ASSERT_TRUE(lua_isstring(L, -1));
    TEST_ASSERT_EQUAL_STRING(VERSION, lua_tostring(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// End-to-end: a Lua chunk that reads all five values and stashes them
// in globals. Verifies that H_lua_run_string can drive H.system.* all
// the way through with no host-side plumbing.
void test_system_functions_via_lua_chunk(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    const char* code =
        "phase6_upt       = H.system.uptime()\n"
        "phase6_now       = H.system.now()\n"
        "phase6_iso       = H.system.now_iso()\n"
        "phase6_id        = H.system.instance_id()\n"
        "phase6_ver       = H.system.version()\n"
        // The chunk runs to the end silently; the test asserts the
        // globals afterward by reading them back as primitives.
        ;

    int rc = H_lua_run_string(L, code, "[phase6:system-all]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "system chunk should run successfully");

    lua_getglobal(L, "phase6_upt");
    TEST_ASSERT_TRUE(lua_isnumber(L, -1));
    lua_pop(L, 1);

    lua_getglobal(L, "phase6_now");
    TEST_ASSERT_TRUE(lua_isnumber(L, -1));
    lua_pop(L, 1);

    lua_getglobal(L, "phase6_iso");
    TEST_ASSERT_TRUE(lua_isstring(L, -1));
    lua_pop(L, 1);

    lua_getglobal(L, "phase6_id");
    TEST_ASSERT_TRUE(lua_isstring(L, -1));
    // The id must be a 36-char UUID-shaped string. Read it as a string+
    // length via lua_tolstring (safer than lua_tostring here because we
    // want to assert the exact length).
    {
        size_t id_len = 0;
        const char* id_str = lua_tolstring(L, -1, &id_len);
        TEST_ASSERT_EQUAL_INT(36, (int)id_len);
        TEST_ASSERT_NOT_NULL(id_str);
    }
    lua_pop(L, 1);

    lua_getglobal(L, "phase6_ver");
    TEST_ASSERT_TRUE(lua_isstring(L, -1));
    TEST_ASSERT_EQUAL_STRING(VERSION, lua_tostring(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// Each function pushes exactly one value onto the stack on success.
void test_system_functions_each_return_one_value(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_system(L);

    const char* funcs[] = { "uptime", "now", "now_iso", "instance_id", "version", NULL };
    for (int i = 0; funcs[i] != NULL; i++) {
        // Use a fresh, clean stack per function so the test does not
        // depend on what the previous iteration left behind. We call
        // each H.system.* from a tiny Lua chunk that simply returns
        // its result.
        char chunk[256];
        snprintf(chunk, sizeof(chunk),
                 "local f = H.system['%s']; return f()", funcs[i]);
        int rc = luaL_dostring(L, chunk);
        TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, funcs[i]);
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, lua_gettop(L),
            "each system function should push exactly one value");
        lua_pop(L, 1);
    }

    H_lua_destroy_context(L);
}

// When the server start time has not been set (e.g. in tests that
// don't call set_server_start_time), uptime() must not produce
// a negative number and must not raise a Lua error.
void test_system_uptime_does_not_fail_when_start_unset(void) {
    // Force get_server_start_time() to report 0 by zeroing the global.
    // The utils_time server_start_time is internal to utils_time.c but
    // set_server_start_time() resets both server_start_time and
    // original_start_time; if neither is called the start is 0.
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_install_system(L);
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "system");
    lua_getfield(L, -1, "uptime");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc,
        "uptime with unset start time must not raise");
    TEST_ASSERT_TRUE(lua_isnumber(L, -1));
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, lua_tonumber(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_system_uptime_is_nonnegative);
    RUN_TEST(test_system_now_is_reasonable_epoch);
    RUN_TEST(test_system_now_iso_has_t_and_z);
    RUN_TEST(test_system_instance_id_is_uuid_shaped);
    RUN_TEST(test_system_instance_id_is_stable_across_calls);
    RUN_TEST(test_system_version_returns_version_macro);
    RUN_TEST(test_system_functions_via_lua_chunk);
    RUN_TEST(test_system_functions_each_return_one_value);
    RUN_TEST(test_system_uptime_does_not_fail_when_start_unset);

    return UNITY_END();
}
