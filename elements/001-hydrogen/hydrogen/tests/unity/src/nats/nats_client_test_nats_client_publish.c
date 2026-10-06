/*
 * Unity Test File: nats_client_publish
 *
 * Publish writes only while the link is up. A queued message stays
 * queued across the handshake.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_nats.h>
#include <src/nats/nats.h>
#include <src/nats/nats_internal.h>

#include <string.h>

typedef struct FakeNats {
    char written[8192];
    size_t written_len;
    const char *script;
    size_t script_len;
    size_t script_off;
} FakeNats;

static FakeNats fake;
static AppConfig cfg;
static AppConfig *saved_config;

static int fake_connect(void *ctx, const char *host, int port, int timeout_seconds) {
    (void)ctx;
    (void)host;
    (void)port;
    (void)timeout_seconds;
    return 0;
}

static int fake_read(void *ctx, void *buf, size_t len) {
    FakeNats *io = ctx;
    size_t left;
    size_t n;

    if (!io || !buf || len == 0) {
        return -1;
    }
    if (!io->script || io->script_off >= io->script_len) {
        return 0;
    }
    left = io->script_len - io->script_off;
    n = left < len ? left : len;
    memcpy(buf, io->script + io->script_off, n);
    io->script_off += n;
    return (int)n;
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

static void install_fake(const char *script) {
    NatsIo io;

    memset(&fake, 0, sizeof(fake));
    fake.script = script;
    fake.script_len = script ? strlen(script) : 0;
    io.connect_fn = fake_connect;
    io.read_fn = fake_read;
    io.write_fn = fake_write;
    io.close_fn = fake_close;
    io.ctx = &fake;
    nats_io_install(&io);
}

static void prime_server(void) {
    nats_config_apply_defaults(&cfg.nats);
    cfg.nats.Servers[0] = strdup("nats://127.0.0.1:4222");
    cfg.nats.ServerCount = 1;
    app_config = &cfg;
}

void test_nats_client_publish_after_handshake(void);
void test_nats_client_publish_fail_next_once(void);
void test_nats_client_publish_respects_max_payload(void);
void test_nats_client_publish_queues_until_up(void);

void setUp(void) {
    saved_config = app_config;
    memset(&cfg, 0, sizeof(cfg));
    memset(&fake, 0, sizeof(fake));
    nats_link_set(NATS_LINK_DOWN);
    nats_client_reset();
    nats_io_install(NULL);
}

void tearDown(void) {
    nats_link_set(NATS_LINK_DOWN);
    nats_io_install(NULL);
    nats_client_reset();
    cleanup_nats_config(&cfg.nats);
    app_config = saved_config;
}

void test_nats_client_publish_after_handshake(void) {
    prime_server();
    install_fake("INFO {}\r\n");
    TEST_ASSERT_EQUAL(0, nats_session_handshake());

    TEST_ASSERT_EQUAL(0, nats_client_publish("cluster.philement.cache.invalidate", "abc", 3));
    TEST_ASSERT_EQUAL(0, nats_client_flush_outbound());
    TEST_ASSERT_NOT_NULL(strstr(fake.written,
                                "PUB cluster.philement.cache.invalidate 3\r\nabc\r\n"));
}

void test_nats_client_publish_fail_next_once(void) {
    size_t before;

    prime_server();
    install_fake("INFO {}\r\n");
    TEST_ASSERT_EQUAL(0, nats_session_handshake());
    before = fake.written_len;

    cfg.nats.Test.FailNextPublishOnLaunch = true;
    TEST_ASSERT_EQUAL(-1, nats_client_publish("cluster.philement.cache.invalidate", "abc", 3));
    TEST_ASSERT_EQUAL(before, fake.written_len);
    TEST_ASSERT_FALSE(cfg.nats.Test.FailNextPublishOnLaunch);

    TEST_ASSERT_EQUAL(0, nats_client_publish("cluster.philement.cache.invalidate", "abc", 3));
    TEST_ASSERT_NOT_NULL(strstr(fake.written,
                                "PUB cluster.philement.cache.invalidate 3\r\nabc\r\n"));
}

void test_nats_client_publish_respects_max_payload(void) {
    prime_server();
    install_fake("INFO {\"max_payload\":4}\r\n");
    TEST_ASSERT_EQUAL(0, nats_session_handshake());

    TEST_ASSERT_EQUAL(-1, nats_client_publish("cluster.philement.cache.invalidate", "abcde", 5));
    TEST_ASSERT_NULL(strstr(fake.written, "PUB "));
    TEST_ASSERT_EQUAL(0, nats_client_publish("cluster.philement.cache.invalidate", "abcd", 4));
    TEST_ASSERT_NOT_NULL(strstr(fake.written,
                                "PUB cluster.philement.cache.invalidate 4\r\nabcd\r\n"));
}

void test_nats_client_publish_queues_until_up(void) {
    prime_server();
    install_fake("INFO {}\r\n");
    nats_link_set(NATS_LINK_DOWN);

    TEST_ASSERT_EQUAL(0, nats_client_publish("cluster.philement.cache.invalidate", "abc", 3));
    TEST_ASSERT_EQUAL(0, fake.written_len);

    TEST_ASSERT_EQUAL(0, nats_session_handshake());
    TEST_ASSERT_NULL(strstr(fake.written, "PUB "));
    TEST_ASSERT_EQUAL(0, nats_client_flush_outbound());
    TEST_ASSERT_NOT_NULL(strstr(fake.written,
                                "PUB cluster.philement.cache.invalidate 3\r\nabc\r\n"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nats_client_publish_after_handshake);
    RUN_TEST(test_nats_client_publish_fail_next_once);
    RUN_TEST(test_nats_client_publish_respects_max_payload);
    RUN_TEST(test_nats_client_publish_queues_until_up);
    return UNITY_END();
}
