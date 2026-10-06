/*
 * Unity Test File: H_lua_nats_broadcast
 *
 * Async publish from Lua. The socket is a fake NatsIo. This test
 * does not start the retry thread and does not dial.
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

typedef struct FakeNats {
    char written[8192];
    size_t written_len;
} FakeNats;

static FakeNats fake;
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
    FakeNats *io = ctx;

    if (!io || (!buf && len > 0)) {
        return -1;
    }
    if (len == 0) {
        return 0;
    }
    if (io->written_len + len >= sizeof(io->written)) {
        return -1;
    }
    memcpy(io->written + io->written_len, buf, len);
    io->written_len += len;
    io->written[io->written_len] = '\0';
    return 0;
}

static void fake_close(void *ctx) {
    (void)ctx;
}

static void install_fake(void) {
    NatsIo io;

    memset(&fake, 0, sizeof(fake));
    io.connect_fn = fake_connect;
    io.read_fn = fake_read;
    io.write_fn = fake_write;
    io.close_fn = fake_close;
    io.ctx = &fake;
    nats_io_install(&io);
}

static void enable_nats(void) {
    nats_config_apply_defaults(&cfg.nats);
    cfg.nats.Enabled = true;
    free(cfg.nats.InstanceId);
    cfg.nats.InstanceId = strdup("hydrogen-01");
    app_config = &cfg;
}

static void field_string(const char *key, const char *value) {
    lua_pushstring(L, value);
    lua_setfield(L, -2, key);
}

static void field_int(const char *key, int value) {
    lua_pushinteger(L, value);
    lua_setfield(L, -2, key);
}

static void push_cache(void) {
    lua_settop(L, 0);
    lua_pushstring(L, "cache.invalidate_by_ref");
    lua_newtable(L);
    field_string("database", "Acuranzo");
    field_int("query_ref", 127);
    field_string("reason", "mutation");
}

static void push_job(void) {
    lua_settop(L, 0);
    lua_pushstring(L, "jobs.done");
    lua_newtable(L);
    field_string("note", "secret-token");
}

static void assert_error(int n, const char *message) {
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_TRUE(lua_isnil(L, -2));
    TEST_ASSERT_EQUAL_STRING(message, lua_tostring(L, -1));
}

static void assert_true(int n) {
    TEST_ASSERT_EQUAL_INT(1, n);
    TEST_ASSERT_TRUE(lua_isboolean(L, -1));
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
}

static json_t *take_envelope(const char *header) {
    const char *at;
    char *end;
    unsigned long size;
    char *copy;
    json_t *obj;

    at = strstr(fake.written, header);
    if (!at) {
        return NULL;
    }
    at += strlen(header);
    size = strtoul(at, &end, 10);
    if (!end || end == at || end[0] != '\r' || end[1] != '\n' || size == 0 || size > 8000) {
        return NULL;
    }
    at = end + 2;
    if (strlen(at) < size + 2 || at[size] != '\r' || at[size + 1] != '\n') {
        return NULL;
    }
    copy = malloc(size + 1);
    if (!copy) {
        return NULL;
    }
    memcpy(copy, at, size);
    copy[size] = '\0';
    obj = json_loads(copy, 0, NULL);
    free(copy);
    return obj;
}

void test_H_lua_nats_broadcast_disabled(void);
void test_H_lua_nats_broadcast_bad_event(void);
void test_H_lua_nats_broadcast_reserved(void);
void test_H_lua_nats_broadcast_bad_data(void);
void test_H_lua_nats_broadcast_cache_queues(void);
void test_H_lua_nats_broadcast_other_event(void);
void test_H_lua_nats_broadcast_publish_failed(void);
void test_H_lua_nats_install(void);

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

void test_H_lua_nats_broadcast_disabled(void) {
    push_job();
    assert_error(H_lua_nats_broadcast(L), "disabled");
    TEST_ASSERT_EQUAL(0, fake.written_len);
}

void test_H_lua_nats_broadcast_bad_event(void) {
    enable_nats();
    lua_settop(L, 0);
    lua_pushinteger(L, 1);
    lua_newtable(L);
    assert_error(H_lua_nats_broadcast(L), "bad event");

    lua_settop(L, 0);
    lua_pushstring(L, "");
    lua_newtable(L);
    assert_error(H_lua_nats_broadcast(L), "bad event");

    lua_settop(L, 0);
    lua_pushstring(L, "cluster.philement.jobs");
    lua_newtable(L);
    assert_error(H_lua_nats_broadcast(L), "bad event");

    lua_settop(L, 0);
    lua_pushstring(L, "bad event");
    lua_newtable(L);
    assert_error(H_lua_nats_broadcast(L), "bad event");

    lua_settop(L, 0);
    lua_pushstring(L, "a*b");
    lua_newtable(L);
    assert_error(H_lua_nats_broadcast(L), "bad event");
    TEST_ASSERT_EQUAL(0, fake.written_len);
}

void test_H_lua_nats_broadcast_reserved(void) {
    enable_nats();
    nats_link_set(NATS_LINK_UP);
    lua_settop(L, 0);
    lua_pushstring(L, "app_state");
    lua_newtable(L);
    field_string("state", "Alive");
    assert_error(H_lua_nats_broadcast(L), "reserved");
    TEST_ASSERT_EQUAL(0, fake.written_len);
}

void test_H_lua_nats_broadcast_bad_data(void) {
    enable_nats();
    nats_link_set(NATS_LINK_UP);
    lua_settop(L, 0);
    lua_pushstring(L, "jobs.done");
    lua_pushstring(L, "nope");
    assert_error(H_lua_nats_broadcast(L), "bad event");

    lua_settop(L, 0);
    lua_pushstring(L, "jobs.done");
    lua_newtable(L);
    lua_pushstring(L, "x");
    lua_rawseti(L, -2, 1);
    assert_error(H_lua_nats_broadcast(L), "bad event");
    TEST_ASSERT_EQUAL(0, fake.written_len);
}

void test_H_lua_nats_broadcast_cache_queues(void) {
    enable_nats();
    nats_link_set(NATS_LINK_DOWN);
    push_cache();
    assert_true(H_lua_nats_broadcast(L));
    TEST_ASSERT_EQUAL(0, fake.written_len);
    nats_link_set(NATS_LINK_UP);
    TEST_ASSERT_EQUAL(0, nats_client_flush_outbound());
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "PUB cluster.philement.cache.invalidate "));
}

void test_H_lua_nats_broadcast_other_event(void) {
    json_t *env;
    const json_t *body;

    enable_nats();
    nats_link_set(NATS_LINK_UP);
    mock_logging_reset_all();
    push_job();
    assert_true(H_lua_nats_broadcast(L));
    TEST_ASSERT_FALSE(mock_logging_message_contains("secret-token"));
    env = take_envelope("PUB cluster.philement.jobs.done ");
    TEST_ASSERT_NOT_NULL(env);
    TEST_ASSERT_EQUAL_STRING("jobs.done", json_string_value(json_object_get(env, "event")));
    body = json_object_get(env, "data");
    TEST_ASSERT_EQUAL_STRING("secret-token", json_string_value(json_object_get(body, "note")));
    json_decref(env);
}

void test_H_lua_nats_broadcast_publish_failed(void) {
    enable_nats();
    nats_link_set(NATS_LINK_UP);
    cfg.nats.Test.FailNextPublishOnLaunch = true;
    push_job();
    assert_error(H_lua_nats_broadcast(L), "publish failed");
    TEST_ASSERT_EQUAL(0, fake.written_len);
    TEST_ASSERT_FALSE(cfg.nats.Test.FailNextPublishOnLaunch);
}

void test_H_lua_nats_install(void) {
    H_lua_install_nats(NULL);

    mock_logging_reset_all();
    H_lua_install_nats(L);
    TEST_ASSERT_TRUE(mock_logging_message_contains("H table missing"));

    lua_newtable(L);
    lua_setglobal(L, "H");
    H_lua_install_nats(L);
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "nats");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "broadcast");
    TEST_ASSERT_TRUE(lua_iscfunction(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "broadcast_sync");
    TEST_ASSERT_TRUE(lua_iscfunction(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "status");
    TEST_ASSERT_TRUE(lua_iscfunction(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "instances");
    TEST_ASSERT_TRUE(lua_iscfunction(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "subscribe");
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "unsubscribe");
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_H_lua_nats_broadcast_disabled);
    RUN_TEST(test_H_lua_nats_broadcast_bad_event);
    RUN_TEST(test_H_lua_nats_broadcast_reserved);
    RUN_TEST(test_H_lua_nats_broadcast_bad_data);
    RUN_TEST(test_H_lua_nats_broadcast_cache_queues);
    RUN_TEST(test_H_lua_nats_broadcast_other_event);
    RUN_TEST(test_H_lua_nats_broadcast_publish_failed);
    RUN_TEST(test_H_lua_nats_install);
    return UNITY_END();
}
