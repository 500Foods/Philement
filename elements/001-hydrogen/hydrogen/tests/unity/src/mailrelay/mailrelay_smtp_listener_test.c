/*
 * Unit tests for mailrelay_smtp_listener: the inbound SMTP listener
 * (Phase 12). Tests cover address parsing, source-network checks,
 * route resolution, listener start/stop state management, the
 * should_accept callback seam, SMTP line I/O, connection lifecycle,
 * and inbound message enqueue paths.
 *
 * Protocol dialog tests (smtp_handle_connection) are in
 * mailrelay_smtp_listener_protocol_test.c.
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

static bool route_never_matches(const char* sender_domain,
                                const char* const* recipients,
                                int recipient_count,
                                MailRelayRouteMatch* out_route) {
    (void)sender_domain; (void)recipients; (void)recipient_count;
    (void)out_route;
    return false;
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

/* ---- smtp_parse_address ---- */

static void test_parse_address_extracts_angle_bracket(void) {
    char out[256];
    TEST_ASSERT_TRUE(smtp_parse_address("MAIL FROM:<alice@example.com>",
                                         "FROM:", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("alice@example.com", out);
}

static void test_parse_address_extracts_bare_after_keyword(void) {
    char out[256];
    TEST_ASSERT_TRUE(smtp_parse_address("RCPT TO:bob@example.com",
                                         "TO:", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("bob@example.com", out);
}

static void test_parse_address_missing_tag_returns_false(void) {
    char out[256];
    TEST_ASSERT_FALSE(smtp_parse_address("MAIL FROM:<alice@example.com>",
                                          "TO:", out, sizeof(out)));
}

static void test_parse_address_empty_tag_returns_false(void) {
    char out[256];
    TEST_ASSERT_FALSE(smtp_parse_address("MAIL FROM:<alice@example.com>",
                                          "", out, sizeof(out)));
}

static void test_parse_address_null_args(void) {
    char out[256];
    TEST_ASSERT_FALSE(smtp_parse_address(NULL, "FROM:", out, sizeof(out)));
    TEST_ASSERT_FALSE(smtp_parse_address("test", NULL, out, sizeof(out)));
    TEST_ASSERT_FALSE(smtp_parse_address("test", "FROM:", NULL, 0));
}

static void test_parse_address_truncates_long_address(void) {
    char small[8];
    TEST_ASSERT_TRUE(smtp_parse_address("MAIL FROM:abcdefghijklmnop@b.c",
                                         "FROM:", small, sizeof(small)));
    TEST_ASSERT_EQUAL(7, strlen(small));
}

/* ---- smtp_get_peer_ip ---- */

static void test_get_peer_ip_ipv4(void) {
    char ip[INET6_ADDRSTRLEN];
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.1.100", &addr.sin_addr);

    smtp_get_peer_ip(-1, (const struct sockaddr*)&addr,
                     sizeof(addr), ip, sizeof(ip));
    TEST_ASSERT_EQUAL_STRING("192.168.1.100", ip);
}

static void test_get_peer_ip_ipv6(void) {
    char ip[INET6_ADDRSTRLEN];
    struct sockaddr_in6 addr6;
    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &addr6.sin6_addr);

    smtp_get_peer_ip(-1, (const struct sockaddr*)&addr6,
                     sizeof(addr6), ip, sizeof(ip));
    TEST_ASSERT_EQUAL_STRING("::1", ip);
}

static void test_get_peer_ip_null_args_returns_safely(void) {
    smtp_get_peer_ip(-1, NULL, 0, NULL, 0);
}

static void test_get_peer_ip_unknown_family(void) {
    char ip[INET6_ADDRSTRLEN];
    struct sockaddr sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_family = AF_UNIX;
    smtp_get_peer_ip(-1, &sa, sizeof(sa), ip, sizeof(ip));
    TEST_ASSERT_EQUAL_STRING("unknown", ip);
}

/* ---- smtp_check_source_network ---- */

static void test_check_source_network_rejects_null_config(void) {
    TEST_ASSERT_FALSE(smtp_check_source_network("1.2.3.4", NULL));
}

static void test_check_source_network_null_ip(void) {
    TEST_ASSERT_FALSE(smtp_check_source_network(NULL, &g_app_config.mail_relay));
}

static void test_check_source_network_empty_ip(void) {
    TEST_ASSERT_FALSE(smtp_check_source_network("", &g_app_config.mail_relay));
}

static void test_check_source_network_accepts_with_config(void) {
    TEST_ASSERT_TRUE(smtp_check_source_network("10.0.0.1",
                                                &g_app_config.mail_relay));
}

/* ---- smtp_reset_envelope ---- */

static void test_reset_envelope_clears_fields(void) {
    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.mail_from[0] = 'x';
    conn.mail_from[1] = 'y';
    conn.mail_from[2] = 'z';
    conn.mail_from[3] = '\0';
    conn.has_mail_from = true;
    conn.auth_active = true;
    conn.route.rewrite_from = "old";
    conn.route.add_recipients = "extra";
    conn.route.route_id = 99;
    conn.recipient_count = 2;
    conn.recipients[0] = strdup("r1@example.com");
    conn.recipients[1] = strdup("r2@example.com");

    smtp_reset_envelope(&conn);
    TEST_ASSERT_EQUAL_STRING("", conn.mail_from);
    TEST_ASSERT_FALSE(conn.has_mail_from);
    TEST_ASSERT_FALSE(conn.auth_active);
    TEST_ASSERT_FALSE(conn.route_matched);
    TEST_ASSERT_NULL(conn.route.rewrite_from);
    TEST_ASSERT_NULL(conn.route.add_recipients);
    TEST_ASSERT_EQUAL_INT(0, conn.route.route_id);
    TEST_ASSERT_EQUAL_INT(0, conn.recipient_count);
    TEST_ASSERT_NULL(conn.recipients[0]);
    TEST_ASSERT_NULL(conn.recipients[1]);
}

/* ---- smtp_resolve_route ---- */

static void test_resolve_route_no_callback_fail_closed(void) {
    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    strcpy(conn.mail_from, "sender@example.com");

    smtp_listener_set_should_accept(NULL);
    TEST_ASSERT_FALSE(smtp_resolve_route(&conn, &g_app_config.mail_relay));
    TEST_ASSERT_FALSE(conn.route_matched);
}

static void test_resolve_route_null_sender_returns_false(void) {
    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.mail_from[0] = '\0';

    smtp_listener_set_should_accept(route_always_matches);
    TEST_ASSERT_FALSE(smtp_resolve_route(&conn, &g_app_config.mail_relay));
}

static void test_resolve_route_callback_match_sets_route(void) {
    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    strcpy(conn.mail_from, "alice@example.com");
    conn.recipients[conn.recipient_count] = strdup("bob@dest.com");
    conn.recipient_count = 1;

    smtp_listener_set_should_accept(route_always_matches);
    TEST_ASSERT_TRUE(smtp_resolve_route(&conn, &g_app_config.mail_relay));
    TEST_ASSERT_TRUE(conn.route_matched);
    TEST_ASSERT_EQUAL_STRING("rewrite@example.com", conn.route.rewrite_from);
    TEST_ASSERT_EQUAL_INT(1, conn.route.route_id);

    for (int i = 0; i < conn.recipient_count; i++) free(conn.recipients[i]);
}

static void test_resolve_route_callback_no_match(void) {
    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    strcpy(conn.mail_from, "alice@example.com");

    smtp_listener_set_should_accept(route_never_matches);
    TEST_ASSERT_FALSE(smtp_resolve_route(&conn, &g_app_config.mail_relay));
    TEST_ASSERT_FALSE(conn.route_matched);
}

/* ---- smtp_listener start/stop ---- */

static void test_listener_stop_sets_shutdown_flag(void) {
    smtp_listener_stop();
    TEST_ASSERT_EQUAL_INT(1, smtp_listener_shutdown);
    TEST_ASSERT_FALSE(smtp_listener_is_running());
}

static void test_listener_start_without_inbound_disabled(void) {
    g_app_config.mail_relay.InboundEnabled = false;

    bool result = smtp_listener_start(&g_app_config.mail_relay);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(smtp_listener_is_running());
}

static void test_listener_start_requires_outbound(void) {
    g_app_config.mail_relay.InboundEnabled = true;
    g_app_config.mail_relay.OutboundEnabled = false;

    bool result = smtp_listener_start(&g_app_config.mail_relay);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(smtp_listener_is_running());
}

static void test_listener_start_null_config_returns_false(void) {
    bool result = smtp_listener_start(NULL);
    TEST_ASSERT_FALSE(result);
}

static void test_should_accept_callback_getter_setter(void) {
    smtp_listener_set_should_accept(route_always_matches);
    TEST_ASSERT_EQUAL_PTR(route_always_matches, smtp_listener_get_should_accept());

    smtp_listener_set_should_accept(NULL);
    TEST_ASSERT_NULL(smtp_listener_get_should_accept());
}

/* ---- smtp_route_should_accept ---- */

static void test_route_should_accept_test_mode_permissive(void) {
    MailRelayRouteMatch route;
    memset(&route, 0, sizeof(route));
    /* Without env var, fail closed */
    unsetenv("MAILRELAY_INBOUND_TEST_MODE");
    TEST_ASSERT_FALSE(smtp_route_should_accept("example.com", NULL, 0, &route));

    /* With test mode env, accept everything */
    setenv("MAILRELAY_INBOUND_TEST_MODE", "1", 1);
    TEST_ASSERT_TRUE(smtp_route_should_accept("example.com", NULL, 0, &route));
    TEST_ASSERT_NULL(route.rewrite_from);
    unsetenv("MAILRELAY_INBOUND_TEST_MODE");
}

/* ---- smtp_conn_free ---- */

static void test_conn_free_handles_null(void) {
    smtp_conn_free(NULL);
}

static void test_conn_free_frees_recipients(void) {
    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.recipients[0] = strdup("a@b.com");
    conn.recipients[1] = strdup("c@d.com");
    conn.recipient_count = 2;

    smtp_conn_free(&conn);
    TEST_ASSERT_EQUAL_INT(0, conn.recipient_count);
}

/* ---- smtp_send_line ---- */

static void test_send_line_null_args_safely(void) {
    smtp_send_line(NULL, "test");
    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    smtp_send_line(&conn, NULL);
    smtp_send_line(NULL, NULL);
}

static void test_send_line_normal_roundtrip(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));
    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.fd = sv[0];

    smtp_send_line(&conn, "220 mailrelay ESMTP ready");

    char buf[256];
    ssize_t n = recv(sv[1], buf, sizeof(buf) - 1, 0);
    TEST_ASSERT_GREATER_THAN(0, n);
    buf[n] = '\0';
    TEST_ASSERT_EQUAL_STRING("220 mailrelay ESMTP ready\r\n", buf);

    if (conn.fd >= 0) close(conn.fd);
    close(sv[1]);
}

static void test_send_line_invalid_fd_safely(void) {
    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.fd = -1;
    smtp_send_line(&conn, "test");
}

/* ---- smtp_read_line ---- */

static void test_read_line_normal(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));
    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.fd = sv[0];

    const char* input = "EHLO example.com\n";
    send(sv[1], input, strlen(input), 0);

    char buf[256];
    TEST_ASSERT_TRUE(smtp_read_line(&conn, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("EHLO example.com", buf);

    close(conn.fd);
    close(sv[1]);
}

static void test_read_line_null_args(void) {
    char buf[256];
    TEST_ASSERT_FALSE(smtp_read_line(NULL, buf, sizeof(buf)));
    TEST_ASSERT_FALSE(smtp_read_line(NULL, NULL, 0));
}

static void test_read_line_invalid_fd(void) {
    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.fd = -1;
    char buf[256];
    TEST_ASSERT_FALSE(smtp_read_line(&conn, buf, sizeof(buf)));
}

static void test_read_line_strips_trailing_cr(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));
    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.fd = sv[0];

    const char* input2 = "MAIL FROM:<a@b.c>\r\n";
    send(sv[1], input2, strlen(input2), 0);

    char buf[256];
    TEST_ASSERT_TRUE(smtp_read_line(&conn, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("MAIL FROM:<a@b.c>", buf);

    close(conn.fd);
    close(sv[1]);
}

static void test_read_line_recv_fail_returns_false(void) {
    int sv[2];
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));
    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.fd = sv[0];

    close(sv[1]);
    char buf[256];
    TEST_ASSERT_FALSE(smtp_read_line(&conn, buf, sizeof(buf)));

    close(conn.fd);
}

/* ---- smtp_enqueue_inbound_message ---- */

static void test_enqueue_inbound_message_no_route_rejects(void) {
    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    strcpy(conn.mail_from, "user@example.com");
    conn.recipient_count = 1;
    conn.recipients[0] = strdup("recipient@example.com");

    bool result = smtp_enqueue_inbound_message(&conn, &g_app_config.mail_relay, "body", 4);
    TEST_ASSERT_FALSE(result);

    free(conn.recipients[0]);
}

static void test_enqueue_inbound_message_enqueue_failure_no_runtime(void) {
    setenv("MAILRELAY_INBOUND_TEST_MODE", "1", 1);
    smtp_listener_set_should_accept(smtp_route_should_accept);

    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    strcpy(conn.mail_from, "user@example.com");
    conn.recipient_count = 1;
    conn.recipients[0] = strdup("recipient@example.com");

    smtp_resolve_route(&conn, &g_app_config.mail_relay);
    TEST_ASSERT_TRUE(conn.route_matched);

    /* mailrelay_runtime is NULL here (no mailrelay_init called), so
     * mailrelay_enqueue returns MAILRELAY_SHUTDOWN. The function should
     * return false. */
    bool result = smtp_enqueue_inbound_message(&conn, &g_app_config.mail_relay, "test body", 9);
    TEST_ASSERT_FALSE(result);

    free(conn.recipients[0]);
}

static void test_enqueue_inbound_message_with_reply_to(void) {
    g_app_config.mail_relay.DefaultReplyTo = strdup("reply@example.com");
    setenv("MAILRELAY_INBOUND_TEST_MODE", "1", 1);
    smtp_listener_set_should_accept(smtp_route_should_accept);

    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    strcpy(conn.mail_from, "user@example.com");
    conn.recipient_count = 1;
    conn.recipients[0] = strdup("recipient@example.com");

    smtp_resolve_route(&conn, &g_app_config.mail_relay);
    TEST_ASSERT_TRUE(conn.route_matched);

    /* mailrelay_runtime is NULL, so enqueue fails, but we exercise the
     * DefaultReplyTo branch in smtp_enqueue_inbound_message. */
    bool result = smtp_enqueue_inbound_message(&conn, &g_app_config.mail_relay, "test body", 9);
    TEST_ASSERT_FALSE(result);

    free(conn.recipients[0]);
    unsetenv("MAILRELAY_INBOUND_TEST_MODE");
}

static void test_enqueue_inbound_message_with_rewrite_from(void) {
    smtp_listener_set_should_accept(route_always_matches);
    /* route_always_matches sets rewrite_from = "rewrite@example.com" */

    struct SmtpConnection conn;
    memset(&conn, 0, sizeof(conn));
    strcpy(conn.mail_from, "user@example.com");
    conn.recipient_count = 1;
    conn.recipients[0] = strdup("recipient@example.com");

    smtp_resolve_route(&conn, &g_app_config.mail_relay);
    TEST_ASSERT_TRUE(conn.route_matched);
    TEST_ASSERT_EQUAL_STRING("rewrite@example.com", conn.route.rewrite_from);

    /* mailrelay_runtime is NULL, so enqueue fails, but we exercise the
     * rewrite_from branch in smtp_enqueue_inbound_message. */
    bool result = smtp_enqueue_inbound_message(&conn, &g_app_config.mail_relay, "test body", 9);
    TEST_ASSERT_FALSE(result);

    free(conn.recipients[0]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_address_extracts_angle_bracket);
    RUN_TEST(test_parse_address_extracts_bare_after_keyword);
    RUN_TEST(test_parse_address_missing_tag_returns_false);
    RUN_TEST(test_parse_address_empty_tag_returns_false);
    RUN_TEST(test_parse_address_null_args);
    RUN_TEST(test_parse_address_truncates_long_address);

    RUN_TEST(test_get_peer_ip_ipv4);
    RUN_TEST(test_get_peer_ip_ipv6);
    RUN_TEST(test_get_peer_ip_null_args_returns_safely);
    RUN_TEST(test_get_peer_ip_unknown_family);

    RUN_TEST(test_check_source_network_rejects_null_config);
    RUN_TEST(test_check_source_network_null_ip);
    RUN_TEST(test_check_source_network_empty_ip);
    RUN_TEST(test_check_source_network_accepts_with_config);

    RUN_TEST(test_reset_envelope_clears_fields);

    RUN_TEST(test_resolve_route_no_callback_fail_closed);
    RUN_TEST(test_resolve_route_null_sender_returns_false);
    RUN_TEST(test_resolve_route_callback_match_sets_route);
    RUN_TEST(test_resolve_route_callback_no_match);

    RUN_TEST(test_listener_stop_sets_shutdown_flag);
    RUN_TEST(test_listener_start_without_inbound_disabled);
    RUN_TEST(test_listener_start_requires_outbound);
    RUN_TEST(test_listener_start_null_config_returns_false);

    RUN_TEST(test_should_accept_callback_getter_setter);
    RUN_TEST(test_route_should_accept_test_mode_permissive);

    RUN_TEST(test_conn_free_handles_null);
    RUN_TEST(test_conn_free_frees_recipients);

    RUN_TEST(test_send_line_null_args_safely);
    RUN_TEST(test_send_line_normal_roundtrip);
    RUN_TEST(test_send_line_invalid_fd_safely);

    RUN_TEST(test_read_line_normal);
    RUN_TEST(test_read_line_null_args);
    RUN_TEST(test_read_line_invalid_fd);
    RUN_TEST(test_read_line_strips_trailing_cr);
    RUN_TEST(test_read_line_recv_fail_returns_false);

    RUN_TEST(test_enqueue_inbound_message_no_route_rejects);
    RUN_TEST(test_enqueue_inbound_message_enqueue_failure_no_runtime);
    RUN_TEST(test_enqueue_inbound_message_with_reply_to);
    RUN_TEST(test_enqueue_inbound_message_with_rewrite_from);

    return UNITY_END();
}

