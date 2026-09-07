/*
 * Unit tests for mailrelay_smtp_listener: the inbound SMTP listener
 * (Phase 12). Tests cover address parsing, source-network checks,
 * route resolution, listener start/stop state management, and the
 * should_accept callback seam.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay_smtp_listener.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/config/config_defaults.h>

#include <unity.h>
#include <string.h>
#include <stdlib.h>
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
    g_app_config.mail_relay.InboundEnabled = true;
    g_app_config.mail_relay.ListenPort = 2525;
    app_config = &g_app_config;

    smtp_listener_stop();
    smtp_listener_set_should_accept(NULL);
}

void tearDown(void) {
    smtp_listener_stop();
    smtp_listener_set_should_accept(NULL);
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

/* ---- smtp_check_source_network ---- */

static void test_check_source_network_rejects_null_config(void) {
    TEST_ASSERT_FALSE(smtp_check_source_network("1.2.3.4", NULL));
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
    conn.has_mail_from = true;
    conn.auth_active = true;
    conn.recipient_count = 0;

    smtp_reset_envelope(&conn);
    TEST_ASSERT_EQUAL_STRING("", conn.mail_from);
    TEST_ASSERT_FALSE(conn.has_mail_from);
    TEST_ASSERT_FALSE(conn.auth_active);
    TEST_ASSERT_FALSE(conn.route_matched);
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
    MailRelayRouteMatch route = {0};
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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_address_extracts_angle_bracket);
    RUN_TEST(test_parse_address_extracts_bare_after_keyword);
    RUN_TEST(test_parse_address_missing_tag_returns_false);
    RUN_TEST(test_parse_address_empty_tag_returns_false);
    RUN_TEST(test_get_peer_ip_ipv4);
    RUN_TEST(test_get_peer_ip_ipv6);
    RUN_TEST(test_get_peer_ip_null_args_returns_safely);
    RUN_TEST(test_check_source_network_rejects_null_config);
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
    return UNITY_END();
}
