/*
 * Unity Test File: H_lua_nats_broadcast_sync
 *
 * Sync publish returns when the bytes are written. A down link does
 * not queue. The socket is a fake NatsIo. This test does not start
 * the retry thread and does not dial.
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

void test_H_lua_nats_broadcast_sync_disabled(void);
void test_H_lua_nats_broadcast_sync_bad_event_first(void);
void test_H_lua_nats_broadcast_sync_link_down(void);
void test_H_lua_nats_broadcast_sync_degraded(void);
void test_H_lua_nats_broadcast_sync_cache_writes(void);
void test_H_lua_nats_broadcast_sync_other_event(void);
void test_H_lua_nats_broadcast_sync_publish_failed(void);
void test_H_lua_nats_broadcast_sync_empty_cache_object(void);

void setUp(void) {
    saved_config = app_config;
    memset(&cfg, 0, sizeof(cfg));
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

void test_H_lua_nats_broadcast_sync_disabled(void) {
    push_job();
    assert_error(H_lua_nats_broadcast_sync(L), "disabled");
    TEST_ASSERT_EQUAL(0, fake.written_len);
}

void test_H_lua_nats_broadcast_sync_bad_event_first(void) {
    enable_nats();
    nats_link_set(NATS_LINK_DOWN);
    lua_settop(L, 0);
    lua_pushstring(L, "bad event");
    lua_newtable(L);
    assert_error(H_lua_nats_broadcast_sync(L), "bad event");
    TEST_ASSERT_EQUAL(0, fake.written_len);
}

void test_H_lua_nats_broadcast_sync_link_down(void) {
    enable_nats();
    cfg.nats.Test.FailNextPublishOnLaunch = true;
    nats_link_set(NATS_LINK_DOWN);
    push_cache();
    assert_error(H_lua_nats_broadcast_sync(L), "link down");
    TEST_ASSERT_TRUE(cfg.nats.Test.FailNextPublishOnLaunch);
    TEST_ASSERT_EQUAL(0, fake.written_len);
    nats_link_set(NATS_LINK_UP);
    TEST_ASSERT_EQUAL(0, nats_client_flush_outbound());
    TEST_ASSERT_EQUAL(0, fake.written_len);
    TEST_ASSERT_TRUE(cfg.nats.Test.FailNextPublishOnLaunch);
}

void test_H_lua_nats_broadcast_sync_degraded(void) {
    enable_nats();
    nats_link_set(NATS_LINK_DEGRADED);
    push_job();
    assert_error(H_lua_nats_broadcast_sync(L), "link down");
    TEST_ASSERT_EQUAL(0, fake.written_len);
}

void test_H_lua_nats_broadcast_sync_cache_writes(void) {
    enable_nats();
    nats_link_set(NATS_LINK_UP);
    push_cache();
    assert_true(H_lua_nats_broadcast_sync(L));
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "PUB cluster.philement.cache.invalidate "));
}

void test_H_lua_nats_broadcast_sync_other_event(void) {
    enable_nats();
    nats_link_set(NATS_LINK_UP);
    push_job();
    assert_true(H_lua_nats_broadcast_sync(L));
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "PUB cluster.philement.jobs.done "));
    TEST_ASSERT_NULL(strstr(fake.written, "cache.invalidate "));
}

void test_H_lua_nats_broadcast_sync_publish_failed(void) {
    enable_nats();
    nats_link_set(NATS_LINK_UP);
    cfg.nats.Test.FailNextPublishOnLaunch = true;
    push_job();
    assert_error(H_lua_nats_broadcast_sync(L), "publish failed");
    TEST_ASSERT_EQUAL(0, fake.written_len);
    TEST_ASSERT_FALSE(cfg.nats.Test.FailNextPublishOnLaunch);
}

void test_H_lua_nats_broadcast_sync_empty_cache_object(void) {
    enable_nats();
    nats_link_set(NATS_LINK_DOWN);
    lua_settop(L, 0);
    lua_pushstring(L, "cache.invalidate_by_ref");
    lua_newtable(L);
    assert_error(H_lua_nats_broadcast_sync(L), "link down");

    nats_link_set(NATS_LINK_UP);
    lua_settop(L, 0);
    lua_pushstring(L, "cache.invalidate_by_ref");
    lua_newtable(L);
    assert_error(H_lua_nats_broadcast_sync(L), "publish failed");
    TEST_ASSERT_EQUAL(0, fake.written_len);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_H_lua_nats_broadcast_sync_disabled);
    RUN_TEST(test_H_lua_nats_broadcast_sync_bad_event_first);
    RUN_TEST(test_H_lua_nats_broadcast_sync_link_down);
    RUN_TEST(test_H_lua_nats_broadcast_sync_degraded);
    RUN_TEST(test_H_lua_nats_broadcast_sync_cache_writes);
    RUN_TEST(test_H_lua_nats_broadcast_sync_other_event);
    RUN_TEST(test_H_lua_nats_broadcast_sync_publish_failed);
    RUN_TEST(test_H_lua_nats_broadcast_sync_empty_cache_object);
    return UNITY_END();
}
