/*
 * Mail Relay Inbound SMTP Listener
 *
 * A submission-only SMTP listener for trusted / internal clients. NOT a public
 * MX / open relay. Each accepted message is routed through the existing outbound
 * queue via template + mailrelay_enqueue (see MAIL_GUIDE.md "Inbound SMTP
 * submission and route rewrites").
 *
 * The listener runs in a dedicated thread spawned by mailrelay_init() when
 * MailRelay.InboundEnabled is true. Per-connection worker threads are tracked
 * via the global mailrelay_threads ServiceThreads.
 */

#ifndef MAILRELAY_SMTP_LISTENER_H
#define MAILRELAY_SMTP_LISTENER_H

#include <stdbool.h>
#include <stddef.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <src/config/config_mail_relay.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/threads/threads.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Inbound route match result. When a route matches, these fields drive
 * how the inbound message is rewritten and enqueued.
 *
 * rewrite_from:   Override envelope From address (NULL = use inbound From).
 * add_recipients: JSON array string of extra recipients (e.g. bcc) to add.
 */
typedef struct MailRelayRouteMatch {
    const char* rewrite_from;
    const char* add_recipients;
    long long route_id;
} MailRelayRouteMatch;

/*
 * Callback for route resolution (Phase 12.4). Returns true if a route matched
 * the given sender_domain + recipients. Called from the SMTP connection handler.
 * Unit tests inject a mock so no live database is needed.
 */
typedef bool (*smtp_listener_should_accept_fn)(const char* sender_domain,
                                               const char* const* recipients,
                                               int recipient_count,
                                               MailRelayRouteMatch* out_route);

/* Thread entry point for the listener (called from mailrelay_init). */
void* smtp_listener_thread(void* arg);

/* Start/stop the listener loop. */
bool smtp_listener_start(const MailRelayConfig* config);
void smtp_listener_stop(void);
bool smtp_listener_is_running(void);

/* Test seams. */
void smtp_listener_set_should_accept(smtp_listener_should_accept_fn fn);
smtp_listener_should_accept_fn smtp_listener_get_should_accept(void);

/*
 * Default route-resolution callback used by mailrelay_init(). May be replaced
 * by tests via smtp_listener_set_should_accept().
 */
bool smtp_route_should_accept(const char* sender_domain,
                              const char* const* recipients,
                              int recipient_count,
                              MailRelayRouteMatch* out_route);

/*
 * Connection-level helpers (exposed non-static for Unity tests).
 */
typedef enum {
    SMTP_CONN_EHLO = 0,
    SMTP_CONN_MAIL_FROM,
    SMTP_CONN_DATA,
} SmtpConnState;

struct SmtpConnection {
    int fd;
    char peer[64];
    char remote_ip[INET6_ADDRSTRLEN];
    SmtpConnState state;
    char mail_from[MV_ADDR_LEN];
    char* recipients[MV_MAX_RECIPIENTS];
    int recipient_count;
    bool has_mail_from;
    bool auth_active;
    MailRelayRouteMatch route;
    bool route_matched;
};

typedef struct SmtpConnection SmtpConnection;

void smtp_get_peer_ip(int fd, const struct sockaddr* addr,
                      socklen_t addrlen, char* ip_out, size_t cap);
void smtp_send_line(struct SmtpConnection* conn, const char* line);
bool smtp_read_line(struct SmtpConnection* conn, char* buf, size_t cap);
void smtp_conn_free(struct SmtpConnection* conn);
void smtp_reset_envelope(struct SmtpConnection* conn);
bool smtp_parse_address(const char* line, const char* tag,
                        char* out, size_t cap);
bool smtp_check_source_network(const char* ip, const MailRelayConfig* config);
bool smtp_resolve_route(struct SmtpConnection* conn, const MailRelayConfig* config);
bool smtp_enqueue_inbound_message(struct SmtpConnection* conn,
                                  const MailRelayConfig* config,
                                  const char* raw_body, size_t body_len);
void* smtp_handle_connection(void* arg);

/* Internal state (exposed for tests/landing shutdown). */
extern volatile sig_atomic_t smtp_listener_shutdown;
extern int smtp_listener_fd;
extern bool smtp_listener_running;

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_SMTP_LISTENER_H */
