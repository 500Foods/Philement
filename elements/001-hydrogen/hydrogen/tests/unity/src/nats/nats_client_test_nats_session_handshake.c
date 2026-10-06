/*
 * Unity Test File: nats_session_handshake
 *
 * The socket calls go through the IO table. This file never dials.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_nats.h>
#include <src/nats/nats.h>

#include <string.h>

typedef struct FakeNats {
    char written[8192];
    size_t written_len;
    const char *script;
    size_t script_len;
    size_t script_off;
    char host[256];
    int port;
    int connect_count;
} FakeNats;

static FakeNats fake;
static AppConfig cfg;
static AppConfig *saved_config;

static int fake_connect(void *ctx, const char *host, int port, int timeout_seconds) {
    FakeNats *io = ctx;

    (void)timeout_seconds;
    if (!io || !host) {
        return -1;
    }
    io->connect_count++;
    io->port = port;
    snprintf(io->host, sizeof(io->host), "%s", host);
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

static void prime_basic(void) {
    nats_config_apply_defaults(&cfg.nats);
    cfg.nats.Servers[0] = strdup("nats://127.0.0.1:4222");
    cfg.nats.ServerCount = 1;
    cfg.nats.Subscriptions[0].Subject = strdup("cache.invalidate");
    cfg.nats.Subscriptions[0].Type = strdup("cluster-wide");
    cfg.nats.Subscriptions[1].Subject = strdup("jobs.refresh-all");
    cfg.nats.Subscriptions[1].Type = strdup("queue-group");
    cfg.nats.Subscriptions[1].QueueGroup = strdup("philement-refresh");
    cfg.nats.SubscriptionCount = 2;
    app_config = &cfg;
}

void test_nats_session_handshake_connect_and_subs(void);
void test_nats_session_handshake_refuses_tls(void);
void test_nats_session_handshake_refuses_auth_without_user(void);
void test_nats_session_handshake_sends_user_and_pass(void);

void setUp(void) {
    saved_config = app_config;
    memset(&cfg, 0, sizeof(cfg));
    memset(&fake, 0, sizeof(fake));
    nats_client_reset();
    nats_io_install(NULL);
}

void tearDown(void) {
    nats_io_install(NULL);
    nats_client_reset();
    cleanup_nats_config(&cfg.nats);
    app_config = saved_config;
}

void test_nats_session_handshake_connect_and_subs(void) {
    const char *script = "PING\r\nINFO {}\r\n";
    char *pong;
    char *connect;

    prime_basic();
    install_fake(script);

    TEST_ASSERT_EQUAL(0, nats_session_handshake());
    TEST_ASSERT_EQUAL_STRING("127.0.0.1", fake.host);
    TEST_ASSERT_EQUAL(4222, fake.port);
    TEST_ASSERT_EQUAL_STRING("up", nats_link_state_name());

    pong = strstr(fake.written, "PONG\r\n");
    connect = strstr(fake.written, "CONNECT ");
    TEST_ASSERT_NOT_NULL(pong);
    TEST_ASSERT_NOT_NULL(connect);
    TEST_ASSERT_TRUE(pong < connect);
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "\"no_echo\":true"));
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "\"verbose\":false"));
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "\"pedantic\":true"));
    TEST_ASSERT_NULL(strstr(fake.written, "\"user\":"));
    TEST_ASSERT_NULL(strstr(fake.written, "\"pass\":"));
    TEST_ASSERT_NOT_NULL(strstr(fake.written,
                                "SUB cluster.philement.cache.invalidate 1\r\n"));
    TEST_ASSERT_NOT_NULL(strstr(fake.written,
                                "SUB cluster.philement.jobs.refresh-all philement-refresh 2\r\n"));

    TEST_ASSERT_EQUAL(0, nats_session_send_unsubs());
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "UNSUB 1\r\n"));
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "UNSUB 2\r\n"));

    fake.script_off = 0;
    fake.written_len = 0;
    fake.written[0] = '\0';
    TEST_ASSERT_EQUAL(0, nats_session_handshake());
    TEST_ASSERT_EQUAL(2, fake.connect_count);
    TEST_ASSERT_NOT_NULL(strstr(fake.written,
                                "SUB cluster.philement.cache.invalidate 1\r\n"));
    TEST_ASSERT_NOT_NULL(strstr(fake.written,
                                "SUB cluster.philement.jobs.refresh-all philement-refresh 2\r\n"));
}

void test_nats_session_handshake_refuses_tls(void) {
    prime_basic();
    install_fake("INFO {\"tls_required\":true}\r\n");

    TEST_ASSERT_EQUAL(-1, nats_session_handshake());
    TEST_ASSERT_NULL(strstr(fake.written, "CONNECT "));
    TEST_ASSERT_EQUAL_STRING("degraded", nats_link_state_name());
}

void test_nats_session_handshake_refuses_auth_without_user(void) {
    prime_basic();
    install_fake("INFO {\"auth_required\":true}\r\n");

    TEST_ASSERT_EQUAL(-1, nats_session_handshake());
    TEST_ASSERT_NULL(strstr(fake.written, "CONNECT "));
}

void test_nats_session_handshake_sends_user_and_pass(void) {
    prime_basic();
    cfg.nats.Username = strdup("nats-user");
    cfg.nats.Password = strdup("nats-secret");
    install_fake("INFO {\"auth_required\":true}\r\n");

    TEST_ASSERT_EQUAL(0, nats_session_handshake());
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "\"user\":\"nats-user\""));
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "\"pass\":\"nats-secret\""));
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "\"no_echo\":true"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nats_session_handshake_connect_and_subs);
    RUN_TEST(test_nats_session_handshake_refuses_tls);
    RUN_TEST(test_nats_session_handshake_refuses_auth_without_user);
    RUN_TEST(test_nats_session_handshake_sends_user_and_pass);
    return UNITY_END();
}
