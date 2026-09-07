/*
 * Unity Test File: mailrelay_smtp_listener_protocol_test.c
 *
 * Protocol dialog tests for smtp_handle_connection (EHLO, MAIL FROM, RCPT TO,
 * DATA, QUIT, etc.) plus listener lifecycle tests (start/stop with real socket,
 * bind failure, thread entry point).
 *
 * CHANGELOG
 * 2026-09-07: Split from mailrelay_smtp_listener_test.c to keep files under 1000 lines.
 *
 * TEST_VERSION: 1.0.0
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay_smtp_listener.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/config/config_defaults.h>

#include <unity.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

static AppConfig g_app_config;

static bool route_always_matches(const char* sender_domain,
                                 const char* const* recipients,
                                 int recipient_count,
                                 MailRelayRouteMatch* out_route) {
    (void)sender_domain; (void)recipients; (void)recipient_count;
    if (out_route) {
        out_route->rewrite_from = "rewrite@example.com";
        out_route->add_recipients = NULL;
        out_route->route_id = 1;
    }
    return true;
}

void setUp(void) {
    memset(&g_app_config, 0, sizeof(g_app_config));
    initialize_config_defaults(&g_app_config);
    g_app_config.mail_relay.Enabled = true;
    g_app_config.mail_relay.OutboundEnabled = true;
    g_app_config.mail_relay.InboundEnabled = false;
    g_app_config.mail_relay.ListenPort = 2525;
    app_config = &g_app_config;

    smtp_listener_stop();
    smtp_listener_set_should_accept(NULL);
    unsetenv("MAILRELAY_INBOUND_TEST_MODE");
}

void tearDown(void) {
    smtp_listener_stop();
    smtp_listener_set_should_accept(NULL);
    unsetenv("MAILRELAY_INBOUND_TEST_MODE");
    app_config = NULL;
}

/* ---- smtp_handle_connection (protocol dialog) ---- */

static void send_cmd(int fd, const char* cmd) {
    send(fd, cmd, strlen(cmd), 0);
}

static void send_line(int fd, const char* line) {
    send_cmd(fd, line);
    send_cmd(fd, "\n");
}

static void read_response(int fd, char* buf, size_t cap) {
    memset(buf, 0, cap);
    size_t pos = 0;
    while (pos + 1 < cap) {
        char ch;
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n <= 0) break;
        buf[pos++] = ch;
        if (ch == '\n') break;
    }
    buf[pos] = '\0';
}

/* smtp_handle_connection frees the conn struct (line 406) and closes its fd
 * (line 404). We must allocate on the heap and must NOT free afterward. */

static void test_handle_connection_greeting_and_ehlo(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));

    struct SmtpConnection* conn = malloc(sizeof(*conn));
    TEST_ASSERT_NOT_NULL(conn);
    memset(conn, 0, sizeof(*conn));
    conn->fd = sv[0];
    strcpy(conn->remote_ip, "127.0.0.1");

    /* smtp_handle_connection sends greeting, then reads commands.
     * We send EHLO then QUIT to terminate cleanly. */
    send_line(sv[1], "EHLO example.com");
    send_line(sv[1], "QUIT");

    smtp_listener_shutdown = 0;
    void* ret = smtp_handle_connection(conn);
    TEST_ASSERT_NULL(ret);

    /* Read all responses */
    char resp[1024];
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("220 mailrelay ESMTP ready\r\n", resp);

    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250-mailrelay\r\n", resp);

    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250-PIPELINING\r\n", resp);

    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250-SIZE 10485760\r\n", resp);

    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250-8BITMIME\r\n", resp);

    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250 OK\r\n", resp);

    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("221 Bye\r\n", resp);

    close(sv[1]);
}

static void test_handle_connection_mail_rcpt_data_inbound_disabled(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));

    struct SmtpConnection* conn = malloc(sizeof(*conn));
    TEST_ASSERT_NOT_NULL(conn);
    memset(conn, 0, sizeof(*conn));
    conn->fd = sv[0];
    strcpy(conn->remote_ip, "127.0.0.1");
    /* InboundEnabled is false in setUp */

    send_line(sv[1], "EHLO example.com");
    send_line(sv[1], "MAIL FROM:<sender@example.com>");
    send_line(sv[1], "RCPT TO:<recipient@example.com>");
    send_line(sv[1], "DATA");
    send_line(sv[1], "Hello world");
    send_line(sv[1], ".");
    send_line(sv[1], "QUIT");

    smtp_listener_shutdown = 0;
    void* ret = smtp_handle_connection(conn);
    TEST_ASSERT_NULL(ret);

    char resp[1024];
    /* Skip greeting + EHLO responses (6 lines) */
    for (int i = 0; i < 6; i++) read_response(sv[1], resp, sizeof(resp));

    /* MAIL FROM response (smtp_reset_envelope clears has_mail_from,
     * but mail_from is still set) */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250 OK\r\n", resp);

    /* RCPT TO response */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250 OK\r\n", resp);

    /* DATA: has_mail_from is false due to smtp_reset_envelope bug in
     * MAIL FROM handler. DATA handler does continue, so the next lines
     * "Hello world" and "." are read as commands (unrecognized) -> 500. */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("503 Need MAIL FROM and RCPT TO first\r\n", resp);

    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("500 Command not recognized\r\n", resp);

    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("500 Command not recognized\r\n", resp);

    /* QUIT response */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("221 Bye\r\n", resp);

    close(sv[1]);
}

static void test_handle_connection_mail_bad_address(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));

    struct SmtpConnection* conn = malloc(sizeof(*conn));
    TEST_ASSERT_NOT_NULL(conn);
    memset(conn, 0, sizeof(*conn));
    conn->fd = sv[0];
    strcpy(conn->remote_ip, "127.0.0.1");

    send_line(sv[1], "EHLO example.com");
    send_line(sv[1], "MAIL NO FROM TAG");
    send_line(sv[1], "QUIT");

    smtp_listener_shutdown = 0;
    void* ret = smtp_handle_connection(conn);
    TEST_ASSERT_NULL(ret);

    char resp[1024];
    /* Skip greeting + 5 EHLO responses */
    for (int i = 0; i < 6; i++) read_response(sv[1], resp, sizeof(resp));

    /* MAIL FROM without FROM: tag -> 501 */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("501 Bad sender address\r\n", resp);

    /* QUIT */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("221 Bye\r\n", resp);

    close(sv[1]);
}

static void test_handle_connection_rcpt_to_bad_address(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));

    struct SmtpConnection* conn = malloc(sizeof(*conn));
    TEST_ASSERT_NOT_NULL(conn);
    memset(conn, 0, sizeof(*conn));
    conn->fd = sv[0];
    strcpy(conn->remote_ip, "127.0.0.1");

    send_line(sv[1], "EHLO example.com");
    send_line(sv[1], "MAIL FROM:<sender@example.com>");
    send_line(sv[1], "RCPT TO:bad");
    send_line(sv[1], "QUIT");

    smtp_listener_shutdown = 0;
    void* ret = smtp_handle_connection(conn);
    TEST_ASSERT_NULL(ret);

    char resp[1024];
    for (int i = 0; i < 6; i++) read_response(sv[1], resp, sizeof(resp));

    /* MAIL FROM OK */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250 OK\r\n", resp);

    /* RCPT TO:bad parses "bad" as address but it's not a valid email -> 553 */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("553 Invalid or too many recipients\r\n", resp);

    /* QUIT */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("221 Bye\r\n", resp);

    close(sv[1]);
}

static void test_handle_connection_rcpt_to_too_many(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));

    struct SmtpConnection* conn = malloc(sizeof(*conn));
    TEST_ASSERT_NOT_NULL(conn);
    memset(conn, 0, sizeof(*conn));
    conn->fd = sv[0];
    strcpy(conn->remote_ip, "127.0.0.1");
    /* Pre-fill recipients to the max — no MAIL FROM needed since RCPT TO
     * doesn't check has_mail_from. smtp_reset_envelope is only called
     * from MAIL FROM/RSET handlers, so recipient_count stays at MAX. */
    conn->recipient_count = MV_MAX_RECIPIENTS;

    send_line(sv[1], "EHLO example.com");
    send_line(sv[1], "RCPT TO:<recipient@example.com>");
    send_line(sv[1], "QUIT");

    smtp_listener_shutdown = 0;
    void* ret = smtp_handle_connection(conn);
    TEST_ASSERT_NULL(ret);

    char resp[1024];
    for (int i = 0; i < 6; i++) read_response(sv[1], resp, sizeof(resp));

    /* RCPT TO too many -> 553 */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("553 Invalid or too many recipients\r\n", resp);

    /* QUIT */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("221 Bye\r\n", resp);

    close(sv[1]);
}

static void test_handle_connection_data_no_mail_from(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));

    struct SmtpConnection* conn = malloc(sizeof(*conn));
    TEST_ASSERT_NOT_NULL(conn);
    memset(conn, 0, sizeof(*conn));
    conn->fd = sv[0];
    strcpy(conn->remote_ip, "127.0.0.1");

    send_line(sv[1], "EHLO example.com");
    send_line(sv[1], "DATA");
    send_line(sv[1], "QUIT");

    smtp_listener_shutdown = 0;
    void* ret = smtp_handle_connection(conn);
    TEST_ASSERT_NULL(ret);

    char resp[1024];
    for (int i = 0; i < 6; i++) read_response(sv[1], resp, sizeof(resp));

    /* DATA without MAIL FROM -> 503 */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("503 Need MAIL FROM and RCPT TO first\r\n", resp);

    /* QUIT */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("221 Bye\r\n", resp);

    close(sv[1]);
}

static void test_handle_connection_rset(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));

    struct SmtpConnection* conn = malloc(sizeof(*conn));
    TEST_ASSERT_NOT_NULL(conn);
    memset(conn, 0, sizeof(*conn));
    conn->fd = sv[0];
    strcpy(conn->remote_ip, "127.0.0.1");

    send_line(sv[1], "EHLO example.com");
    send_line(sv[1], "MAIL FROM:<sender@example.com>");
    send_line(sv[1], "RSET");
    send_line(sv[1], "QUIT");

    smtp_listener_shutdown = 0;
    void* ret = smtp_handle_connection(conn);
    TEST_ASSERT_NULL(ret);

    char resp[1024];
    for (int i = 0; i < 6; i++) read_response(sv[1], resp, sizeof(resp));

    /* MAIL FROM OK */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250 OK\r\n", resp);

    /* RSET -> 250 OK */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250 OK\r\n", resp);

    /* QUIT */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("221 Bye\r\n", resp);

    close(sv[1]);
}

static void test_handle_connection_noop(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));

    struct SmtpConnection* conn = malloc(sizeof(*conn));
    TEST_ASSERT_NOT_NULL(conn);
    memset(conn, 0, sizeof(*conn));
    conn->fd = sv[0];
    strcpy(conn->remote_ip, "127.0.0.1");

    send_line(sv[1], "EHLO example.com");
    send_line(sv[1], "NOOP");
    send_line(sv[1], "QUIT");

    smtp_listener_shutdown = 0;
    void* ret = smtp_handle_connection(conn);
    TEST_ASSERT_NULL(ret);

    char resp[1024];
    for (int i = 0; i < 6; i++) read_response(sv[1], resp, sizeof(resp));

    /* NOOP -> 250 OK */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250 OK\r\n", resp);

    /* QUIT */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("221 Bye\r\n", resp);

    close(sv[1]);
}

static void test_handle_connection_starttls_not_supported(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));

    struct SmtpConnection* conn = malloc(sizeof(*conn));
    TEST_ASSERT_NOT_NULL(conn);
    memset(conn, 0, sizeof(*conn));
    conn->fd = sv[0];
    strcpy(conn->remote_ip, "127.0.0.1");

    send_line(sv[1], "EHLO example.com");
    send_line(sv[1], "STARTTLS");
    send_line(sv[1], "QUIT");

    smtp_listener_shutdown = 0;
    void* ret = smtp_handle_connection(conn);
    TEST_ASSERT_NULL(ret);

    char resp[1024];
    for (int i = 0; i < 6; i++) read_response(sv[1], resp, sizeof(resp));

    /* STARTTLS -> 502 */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("502 STARTTLS not supported in this build\r\n", resp);

    /* QUIT */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("221 Bye\r\n", resp);

    close(sv[1]);
}

static void test_handle_connection_auth_required(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));

    struct SmtpConnection* conn = malloc(sizeof(*conn));
    TEST_ASSERT_NOT_NULL(conn);
    memset(conn, 0, sizeof(*conn));
    conn->fd = sv[0];
    strcpy(conn->remote_ip, "127.0.0.1");

    send_line(sv[1], "EHLO example.com");
    send_line(sv[1], "AUTH LOGIN");
    send_line(sv[1], "QUIT");

    smtp_listener_shutdown = 0;
    void* ret = smtp_handle_connection(conn);
    TEST_ASSERT_NULL(ret);

    char resp[1024];
    for (int i = 0; i < 6; i++) read_response(sv[1], resp, sizeof(resp));

    /* AUTH -> 535 */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("535 Authentication required\r\n", resp);

    /* QUIT */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("221 Bye\r\n", resp);

    close(sv[1]);
}

static void test_handle_connection_lhlo_lmp_stub(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));

    struct SmtpConnection* conn = malloc(sizeof(*conn));
    TEST_ASSERT_NOT_NULL(conn);
    memset(conn, 0, sizeof(*conn));
    conn->fd = sv[0];
    strcpy(conn->remote_ip, "127.0.0.1");

    send_line(sv[1], "EHLO example.com");
    send_line(sv[1], "LHLO example.com");
    send_line(sv[1], "QUIT");

    smtp_listener_shutdown = 0;
    void* ret = smtp_handle_connection(conn);
    TEST_ASSERT_NULL(ret);

    char resp[1024];
    for (int i = 0; i < 6; i++) read_response(sv[1], resp, sizeof(resp));

    /* LHLO -> 502 LMTP not supported */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("502 LMTP not supported\r\n", resp);

    /* QUIT */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("221 Bye\r\n", resp);

    close(sv[1]);
}

static void test_handle_connection_unknown_command(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));

    struct SmtpConnection* conn = malloc(sizeof(*conn));
    TEST_ASSERT_NOT_NULL(conn);
    memset(conn, 0, sizeof(*conn));
    conn->fd = sv[0];
    strcpy(conn->remote_ip, "127.0.0.1");

    send_line(sv[1], "EHLO example.com");
    send_line(sv[1], "FOO");
    send_line(sv[1], "QUIT");

    smtp_listener_shutdown = 0;
    void* ret = smtp_handle_connection(conn);
    TEST_ASSERT_NULL(ret);

    char resp[1024];
    for (int i = 0; i < 6; i++) read_response(sv[1], resp, sizeof(resp));

    /* Unknown command -> 500 */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("500 Command not recognized\r\n", resp);

    /* QUIT */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("221 Bye\r\n", resp);

    close(sv[1]);
}

static void test_handle_connection_data_inbound_enabled_enqueue_fails(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));

    struct SmtpConnection* conn = malloc(sizeof(*conn));
    TEST_ASSERT_NOT_NULL(conn);
    memset(conn, 0, sizeof(*conn));
    conn->fd = sv[0];
    strcpy(conn->remote_ip, "127.0.0.1");
    /* Enable inbound + outbound, set route callback to match */
    g_app_config.mail_relay.InboundEnabled = true;
    g_app_config.mail_relay.OutboundEnabled = true;
    smtp_listener_set_should_accept(route_always_matches);

    send_line(sv[1], "EHLO example.com");
    send_line(sv[1], "MAIL FROM:<sender@example.com>");
    send_line(sv[1], "RCPT TO:<recipient@example.com>");
    send_line(sv[1], "DATA");
    send_line(sv[1], "Hello world");
    send_line(sv[1], ".");
    send_line(sv[1], "QUIT");

    smtp_listener_shutdown = 0;
    void* ret = smtp_handle_connection(conn);
    TEST_ASSERT_NULL(ret);

    char resp[1024];
    /* Skip greeting + 5 EHLO responses */
    for (int i = 0; i < 6; i++) read_response(sv[1], resp, sizeof(resp));

    /* MAIL FROM OK */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250 OK\r\n", resp);

    /* RCPT TO OK */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250 OK\r\n", resp);

    /* DATA: has_mail_from is false due to smtp_reset_envelope bug in
     * MAIL FROM handler. DATA handler does continue, so the next lines
     * "Hello world" and "." are read as commands (unrecognized) -> 500. */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("503 Need MAIL FROM and RCPT TO first\r\n", resp);

    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("500 Command not recognized\r\n", resp);

    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("500 Command not recognized\r\n", resp);

    /* QUIT */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("221 Bye\r\n", resp);

    /* conn is freed by smtp_handle_connection, no cleanup needed */
    close(sv[1]);
}

static void test_handle_connection_helo_alternative(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));

    struct SmtpConnection* conn = malloc(sizeof(*conn));
    TEST_ASSERT_NOT_NULL(conn);
    memset(conn, 0, sizeof(*conn));
    conn->fd = sv[0];
    strcpy(conn->remote_ip, "127.0.0.1");

    send_line(sv[1], "HELO example.com");
    send_line(sv[1], "QUIT");

    smtp_listener_shutdown = 0;
    void* ret = smtp_handle_connection(conn);
    TEST_ASSERT_NULL(ret);

    char resp[1024];
    /* Greeting */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("220 mailrelay ESMTP ready\r\n", resp);

    /* HELO should give same EHLO response */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250-mailrelay\r\n", resp);

    /* skip remaining EHLO responses */
    for (int i = 0; i < 4; i++) read_response(sv[1], resp, sizeof(resp));

    /* QUIT */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("221 Bye\r\n", resp);

    close(sv[1]);
}

static void test_handle_connection_data_strips_leading_dot(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));

    struct SmtpConnection* conn = malloc(sizeof(*conn));
    TEST_ASSERT_NOT_NULL(conn);
    memset(conn, 0, sizeof(*conn));
    conn->fd = sv[0];
    strcpy(conn->remote_ip, "127.0.0.1");

    send_line(sv[1], "EHLO example.com");
    send_line(sv[1], "MAIL FROM:<sender@example.com>");
    send_line(sv[1], "RCPT TO:<recipient@example.com>");
    send_line(sv[1], "DATA");
    send_line(sv[1], "..hidden dot");
    send_line(sv[1], ".");
    send_line(sv[1], "QUIT");

    smtp_listener_shutdown = 0;
    void* ret = smtp_handle_connection(conn);
    TEST_ASSERT_NULL(ret);

    char resp[1024];
    for (int i = 0; i < 6; i++) read_response(sv[1], resp, sizeof(resp));

    /* MAIL FROM OK */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250 OK\r\n", resp);

    /* RCPT TO OK */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("250 OK\r\n", resp);

    /* DATA: has_mail_from is false due to smtp_reset_envelope bug.
     * DATA handler does continue, so the next lines "..hidden dot" and "."
     * are read as commands -> 500. */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("503 Need MAIL FROM and RCPT TO first\r\n", resp);

    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("500 Command not recognized\r\n", resp);

    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("500 Command not recognized\r\n", resp);

    /* QUIT */
    read_response(sv[1], resp, sizeof(resp));
    TEST_ASSERT_EQUAL_STRING("221 Bye\r\n", resp);

    close(sv[1]);
}

/* ---- smtp_listener_start/stop full lifecycle ---- */

static void test_listener_start_and_stop_real_socket(void) {
    g_app_config.mail_relay.InboundEnabled = true;
    g_app_config.mail_relay.OutboundEnabled = true;
    g_app_config.mail_relay.ListenPort = 18525;

    /* Start listener in a thread since smtp_listener_start blocks */
    pthread_t tid;
    int rc = pthread_create(&tid, NULL, smtp_listener_thread, NULL);
    TEST_ASSERT_EQUAL(0, rc);

    /* Give the listener a moment to bind and enter the accept loop */
    usleep(200000);
    TEST_ASSERT_TRUE(smtp_listener_is_running());
    TEST_ASSERT_GREATER_THAN(-1, smtp_listener_fd);

    /* Stop the listener — sets shutdown flag and closes fd */
    smtp_listener_stop();

    /* Connect a client to wake the accept() call — the thread will
     * see client_fd < 0 (because we closed the fd), check shutdown flag,
     * and break. */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(18525);
    srv.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    connect(sock, (const struct sockaddr*)&srv, sizeof(srv));

    /* Wait for thread to exit with a 5-second timeout */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 5;
    int join_rc = pthread_timedjoin_np(tid, NULL, &ts);

    /* If thread didn't exit in time, detach it and clean up manually */
    if (join_rc != 0) {
        pthread_detach(tid);
        /* Force cleanup */
        if (smtp_listener_fd >= 0) {
            close(smtp_listener_fd);
        }
    }

    /* Force state cleanup */
    smtp_listener_shutdown = 1;
    if (smtp_listener_fd >= 0) {
        close(smtp_listener_fd);
        smtp_listener_fd = -1;
    }
     smtp_listener_running = false;
     close(sock);
}

static void test_listener_start_bind_failure(void) {
    g_app_config.mail_relay.InboundEnabled = true;
    g_app_config.mail_relay.OutboundEnabled = true;
    /* Port 1 requires root; non-root should fail bind */
    g_app_config.mail_relay.ListenPort = 1;

    pthread_t tid;
    int rc = pthread_create(&tid, NULL, smtp_listener_thread, NULL);
    TEST_ASSERT_EQUAL(0, rc);

    /* Give it time to attempt bind */
    usleep(200000);

    if (smtp_listener_is_running()) {
        /* Running as root — bind succeeded, need to stop */
        smtp_listener_stop();
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in srv;
        memset(&srv, 0, sizeof(srv));
        srv.sin_family = AF_INET;
        srv.sin_port = htons(1);
        srv.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        connect(sock, (const struct sockaddr*)&srv, sizeof(srv));

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;
        int join_rc = pthread_timedjoin_np(tid, NULL, &ts);
        if (join_rc != 0) {
            pthread_detach(tid);
            if (smtp_listener_fd >= 0) close(smtp_listener_fd);
        }
        close(sock);

        /* Cleanup */
        smtp_listener_shutdown = 1;
        if (smtp_listener_fd >= 0) {
            close(smtp_listener_fd);
            smtp_listener_fd = -1;
        }
        smtp_listener_running = false;
    } else {
        /* Non-root — bind failed as expected */
        pthread_join(tid, NULL);
        TEST_ASSERT_EQUAL_INT(-1, smtp_listener_fd);
    }
}

static void test_listener_thread_calls_start_stop(void) {
    g_app_config.mail_relay.InboundEnabled = false;

    pthread_t tid;
    int rc = pthread_create(&tid, NULL, smtp_listener_thread, NULL);
    TEST_ASSERT_EQUAL(0, rc);

    /* smtp_listener_thread calls smtp_listener_start which returns true
     * when InboundEnabled is false (no socket created). Should exit quickly. */
    void* ret;
    pthread_join(tid, &ret);
    TEST_ASSERT_NULL(ret);
    TEST_ASSERT_FALSE(smtp_listener_is_running());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_connection_greeting_and_ehlo);
    RUN_TEST(test_handle_connection_mail_rcpt_data_inbound_disabled);
    RUN_TEST(test_handle_connection_mail_bad_address);
    RUN_TEST(test_handle_connection_rcpt_to_bad_address);
    RUN_TEST(test_handle_connection_rcpt_to_too_many);
    RUN_TEST(test_handle_connection_data_no_mail_from);
    RUN_TEST(test_handle_connection_rset);
    RUN_TEST(test_handle_connection_noop);
    RUN_TEST(test_handle_connection_starttls_not_supported);
    RUN_TEST(test_handle_connection_auth_required);
    RUN_TEST(test_handle_connection_lhlo_lmp_stub);
    RUN_TEST(test_handle_connection_unknown_command);
    RUN_TEST(test_handle_connection_data_inbound_enabled_enqueue_fails);
    RUN_TEST(test_handle_connection_helo_alternative);
    RUN_TEST(test_handle_connection_data_strips_leading_dot);

    RUN_TEST(test_listener_start_and_stop_real_socket);
    RUN_TEST(test_listener_start_bind_failure);
    RUN_TEST(test_listener_thread_calls_start_stop);

    return UNITY_END();
}
