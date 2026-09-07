#include <src/hydrogen.h>
#include <unity.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/mdns/mdns_client.h>
#include <src/scripting/scripting_api.h>

static lua_State *L;
static MDNSClientConfig g_cfg;
static mdns_client_t *g_client;

void test_H_lua_mdns_list_empty(void);
void test_H_lua_mdns_list_one(void);

void setUp(void) {
    memset(&g_cfg, 0, sizeof g_cfg);
    g_cfg.max_services = 4;
    g_cfg.own_services = true;
    g_client = mdns_client_create(&g_cfg);
    TEST_ASSERT_NOT_NULL(g_client);
    mdns_client_instance = g_client;
    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
}

void tearDown(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
    mdns_client_instance = NULL;
    mdns_client_destroy(g_client);
    g_client = NULL;
}

void test_H_lua_mdns_list_empty(void) {
    TEST_ASSERT_EQUAL_INT(1, H_lua_mdns_list(L));
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    TEST_ASSERT_EQUAL_INT(0, (int)lua_rawlen(L, -1));
}

void test_H_lua_mdns_list_one(void) {
    TEST_ASSERT_NOT_NULL(mdns_client_insert_instance(g_client, "A._http._tcp.local", "_http._tcp.local"));
    TEST_ASSERT_EQUAL_INT(1, H_lua_mdns_list(L));
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    TEST_ASSERT_EQUAL_INT(1, (int)lua_rawlen(L, -1));
    lua_rawgeti(L, -1, 1);
    lua_getfield(L, -1, "name");
    TEST_ASSERT_EQUAL_STRING("A._http._tcp.local", lua_tostring(L, -1));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_H_lua_mdns_list_empty);
    RUN_TEST(test_H_lua_mdns_list_one);
    return UNITY_END();
}
