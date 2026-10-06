/*
 * Unity Test File: H_lua_nats_instances
 *
 * singleton, self, and peers. Presence off returns an empty peer
 * list. This test does not start the retry thread and does not dial.
 * Peer ids are not logged.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/config/config_nats.h>
#include <src/nats/nats.h>
#include <src/nats/nats_internal.h>
#include <src/scripting/scripting_api.h>

#include "mock_logging.h"

#include <stdlib.h>
#include <string.h>

static AppConfig cfg;
static AppConfig *saved_config;
static lua_State *L;

static int fake_connect(void *ctx, const char *host, int port, int timeout_seconds) {
    (void)ctx;
    (void)host;
    (void)port;
    (void)timeout_seconds;
    return 0;
}

static int fake_read(void *ctx, void *buf, size_t len) {
    (void)ctx;
    (void)buf;
    (void)len;
    return 0;
}

static int fake_write(void *ctx, const void *buf, size_t len) {
    (void)ctx;
    (void)buf;
    (void)len;
    return 0;
}

static void fake_close(void *ctx) {
    (void)ctx;
}

static void install_fake(void) {
    NatsIo io;

    io.connect_fn = fake_connect;
    io.read_fn = fake_read;
    io.write_fn = fake_write;
    io.close_fn = fake_close;
    io.ctx = NULL;
    nats_io_install(&io);
}

static void enable_nats(void) {
    nats_config_apply_defaults(&cfg.nats);
    cfg.nats.Enabled = true;
    free(cfg.nats.InstanceId);
    cfg.nats.InstanceId = strdup("hydrogen-01");
    app_config = &cfg;
}

static json_t *make_peer(const char *id, const char *state, int connections) {
    json_t *root = json_object();
    json_t *body = json_object();

    json_object_set_new(body, "state", json_string(state));
    if (connections >= 0) {
        json_object_set_new(body, "websocket_connections", json_integer(connections));
    }
    json_object_set_new(root, "event", json_string("app_state"));
    json_object_set_new(root, "subject",
                        json_string("cluster.philement.instance.app_state"));
    json_object_set_new(root, "timestamp", json_string("2026-10-05T00:00:00Z"));
    json_object_set_new(root, "source", json_string(id));
    json_object_set_new(root, "instance_id", json_string(id));
    json_object_set_new(root, "data", body);
    return root;
}

static void apply_peer(const char *id, const char *state, int connections) {
    json_t *root = make_peer(id, state, connections);

    TEST_ASSERT_NOT_NULL(root);
    nats_registry_apply(root);
    json_decref(root);
}

static void call_instances(void) {
    lua_settop(L, 0);
    TEST_ASSERT_EQUAL_INT(1, H_lua_nats_instances(L));
    TEST_ASSERT_TRUE(lua_istable(L, -1));
}

static void require_peer(int index, const char *id, const char *state, int connections) {
    lua_getfield(L, -1, "peers");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_rawgeti(L, -1, index);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "id");
    TEST_ASSERT_EQUAL_STRING(id, lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "state");
    TEST_ASSERT_EQUAL_STRING(state, lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "websocket_connections");
    TEST_ASSERT_EQUAL_INT(connections, (int)lua_tointeger(L, -1));
    lua_pop(L, 3);
}

void test_H_lua_nats_instances_presence_off(void);
void test_H_lua_nats_instances_peers(void);
void test_H_lua_nats_instances_hides_peers_when_presence_off(void);
void test_H_lua_nats_instances_self(void);

void setUp(void) {
    saved_config = app_config;
    memset(&cfg, 0, sizeof(cfg));
    mock_logging_reset_all();
    nats_link_set(NATS_LINK_DOWN);
    nats_client_reset();
    install_fake();
    app_config = &cfg;
    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
}

void tearDown(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
    nats_link_set(NATS_LINK_DOWN);
    nats_io_install(NULL);
    nats_client_reset();
    cleanup_nats_config(&cfg.nats);
    app_config = saved_config;
}

void test_H_lua_nats_instances_presence_off(void) {
    enable_nats();
    call_instances();
    lua_getfield(L, -1, "singleton");
    TEST_ASSERT_EQUAL_INT(0, lua_toboolean(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "self");
    TEST_ASSERT_EQUAL_STRING("none", lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "peers");
    TEST_ASSERT_EQUAL_INT(0, (int)lua_rawlen(L, -1));
}

void test_H_lua_nats_instances_peers(void) {
    enable_nats();
    cfg.nats.Presence.Enabled = true;
    apply_peer("peer-a", "Starting", -1);
    mock_logging_reset_all();
    call_instances();
    lua_getfield(L, -1, "peers");
    TEST_ASSERT_EQUAL_INT(1, (int)lua_rawlen(L, -1));
    lua_pop(L, 1);
    require_peer(1, "peer-a", "Starting", 0);
    TEST_ASSERT_FALSE(mock_logging_message_contains("peer-a"));

    apply_peer("peer-a", "Alive", 4);
    apply_peer("peer-b", "Alive", 2);
    call_instances();
    lua_getfield(L, -1, "peers");
    TEST_ASSERT_EQUAL_INT(2, (int)lua_rawlen(L, -1));
    lua_pop(L, 1);
    require_peer(1, "peer-a", "Alive", 4);
    require_peer(2, "peer-b", "Alive", 2);
    lua_getfield(L, -1, "singleton");
    TEST_ASSERT_EQUAL_INT(0, lua_toboolean(L, -1));
}

void test_H_lua_nats_instances_hides_peers_when_presence_off(void) {
    NatsRegistryCopy copied[4];

    enable_nats();
    cfg.nats.Presence.Enabled = true;
    apply_peer("peer-a", "Alive", 1);
    cfg.nats.Presence.Enabled = false;
    call_instances();
    lua_getfield(L, -1, "singleton");
    TEST_ASSERT_EQUAL_INT(0, lua_toboolean(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "peers");
    TEST_ASSERT_EQUAL_INT(0, (int)lua_rawlen(L, -1));
    TEST_ASSERT_EQUAL(1, nats_registry_copy(copied, 4));
    TEST_ASSERT_EQUAL_STRING("peer-a", copied[0].id);
}

void test_H_lua_nats_instances_self(void) {
    enable_nats();
    cfg.nats.Presence.Enabled = true;
    nats_link_set(NATS_LINK_DOWN);
    TEST_ASSERT_EQUAL(0, nats_registry_publish("Starting"));
    mock_logging_reset_all();
    call_instances();
    lua_getfield(L, -1, "self");
    TEST_ASSERT_EQUAL_STRING("Starting", lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "singleton");
    TEST_ASSERT_EQUAL_INT(0, lua_toboolean(L, -1));
    TEST_ASSERT_FALSE(mock_logging_message_contains("hydrogen-01"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_H_lua_nats_instances_presence_off);
    RUN_TEST(test_H_lua_nats_instances_peers);
    RUN_TEST(test_H_lua_nats_instances_hides_peers_when_presence_off);
    RUN_TEST(test_H_lua_nats_instances_self);
    return UNITY_END();
}
