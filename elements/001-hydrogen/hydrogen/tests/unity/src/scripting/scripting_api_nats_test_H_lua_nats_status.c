/*
 * Unity Test File: H_lua_nats_status
 *
 * One status table. No message counters. This test does not start
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

#include <string.h>

static AppConfig cfg;
static AppConfig *saved_config;
static lua_State *L;
static time_t now_clock;

static time_t clock_now(void) {
    return now_clock;
}

static int count_now(void) {
    return 0;
}

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
    app_config = &cfg;
}

static void require_bool(const char *key, int expect) {
    lua_getfield(L, -1, key);
    TEST_ASSERT_TRUE(lua_isboolean(L, -1));
    TEST_ASSERT_EQUAL_INT(expect, lua_toboolean(L, -1));
    lua_pop(L, 1);
}

static void require_string(const char *key, const char *expect) {
    lua_getfield(L, -1, key);
    TEST_ASSERT_EQUAL_STRING(expect, lua_tostring(L, -1));
    lua_pop(L, 1);
}

static void require_int(const char *key, int expect) {
    lua_getfield(L, -1, key);
    TEST_ASSERT_TRUE(lua_isinteger(L, -1));
    TEST_ASSERT_EQUAL_INT(expect, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);
}

static void call_status(void) {
    lua_settop(L, 0);
    TEST_ASSERT_EQUAL_INT(1, H_lua_nats_status(L));
    TEST_ASSERT_EQUAL_INT(1, lua_gettop(L));
    TEST_ASSERT_TRUE(lua_istable(L, -1));
}

void test_H_lua_nats_status_disabled(void);
void test_H_lua_nats_status_flags(void);
void test_H_lua_nats_status_singleton(void);
void test_H_lua_nats_status_no_counters(void);
void test_H_lua_nats_status_null_config(void);

void setUp(void) {
    saved_config = app_config;
    memset(&cfg, 0, sizeof(cfg));
    now_clock = 5000;
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

void test_H_lua_nats_status_disabled(void) {
    call_status();
    require_bool("enabled", 0);
    require_string("link", "down");
    require_bool("presence", 0);
    require_bool("singleton", 0);
    require_int("peers", 0);
    require_int("alive", 0);
    require_string("self", "none");
}

void test_H_lua_nats_status_flags(void) {
    enable_nats();
    cfg.nats.Presence.Enabled = true;
    nats_link_set(NATS_LINK_DEGRADED);
    call_status();
    require_bool("enabled", 1);
    require_string("link", "degraded");
    require_bool("presence", 1);
    require_bool("singleton", 0);
    require_string("self", "none");
}

void test_H_lua_nats_status_singleton(void) {
    int stale;

    enable_nats();
    cfg.nats.Presence.Enabled = true;
    nats_registry_set_clock(clock_now);
    nats_registry_set_connection_count(count_now);
    TEST_ASSERT_EQUAL(0, nats_registry_publish("Alive"));
    nats_link_set(NATS_LINK_UP);
    stale = nats_registry_stale_seconds();
    now_clock = 5000 + stale;
    call_status();
    require_bool("singleton", 1);
    require_string("link", "up");
    require_string("self", "Alive");
    require_int("peers", 0);
    require_int("alive", 0);
}

void test_H_lua_nats_status_no_counters(void) {
    call_status();
    lua_getfield(L, -1, "messages");
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "published");
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
}

void test_H_lua_nats_status_null_config(void) {
    app_config = NULL;
    nats_link_set(NATS_LINK_UP);
    call_status();
    require_bool("enabled", 0);
    require_bool("presence", 0);
    require_string("link", "up");
    require_bool("singleton", 0);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_H_lua_nats_status_disabled);
    RUN_TEST(test_H_lua_nats_status_flags);
    RUN_TEST(test_H_lua_nats_status_singleton);
    RUN_TEST(test_H_lua_nats_status_no_counters);
    RUN_TEST(test_H_lua_nats_status_null_config);
    return UNITY_END();
}
