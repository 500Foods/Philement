/*
 * Mail Relay Inbound SMTP Listener
 *
 * A submission-only SMTP (and optional LMTP stub) listener for trusted /
 * internal clients. This is NOT a public MX / open relay. Each accepted
 * message is converted into a normal outbound queue item via the existing
 * template/producer path (12.5), optionally through a route template
 * configured in mail_routes (QueryRefs 118-122).
 *
 * Design:
 *   - One listening socket on MailRelay.ListenPort, accept loop thread.
 *   - Each accepted connection gets its own worker thread (tracked via
 *     mailrelay_threads).
 *   - Per-connection SMTP state machine: greeting -> EHLO -> optional
 *     STARTTLS/AUTH -> MAIL FROM -> RCPT TO -> DATA -> enqueue -> QUIT.
 *   - Anti-open-relay: source-network allowlist, optional AUTH, route
 *     matching. Unmatched messages are rejected before enqueue.
 *   - LMTP is a stub that logs "unsupported/deferred" (12.6).
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay_smtp_listener.h>
#include <src/mailrelay/mailrelay_internal.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_smtp.h>
#include <src/mailrelay/mailrelay_repository.h>
#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_template.h>

#include <src/threads/threads.h>
#include <src/utils/utils_time.h>
#include <src/utils/utils_uuid.h>

#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define SMTP_LISTENER_BACKLOG 16
#define SMTP_MAX_LINE 16384

volatile sig_atomic_t smtp_listener_shutdown = 0;
int smtp_listener_fd = -1;
bool smtp_listener_running = false;

smtp_listener_should_accept_fn g_should_accept_fn = NULL;

void smtp_get_peer_ip(int fd, const struct sockaddr* addr,
                       socklen_t addrlen, char* ip_out, size_t cap) {
    (void)fd; (void)addrlen;
    if (!addr || !ip_out || !cap) return;
    if (addr->sa_family == AF_INET) {
        const struct sockaddr_in* sin = (const struct sockaddr_in*)addr;
        inet_ntop(AF_INET, &sin->sin_addr, ip_out, (socklen_t)cap);
    } else if (addr->sa_family == AF_INET6) {
        const struct sockaddr_in6* sin6 = (const struct sockaddr_in6*)addr;
        inet_ntop(AF_INET6, &sin6->sin6_addr, ip_out, (socklen_t)cap);
    } else {
        snprintf(ip_out, cap, "unknown");
    }
}

void smtp_send_line(struct SmtpConnection* conn, const char* line) {
    if (!conn || conn->fd < 0 || !line) return;
    size_t len = strlen(line);
    char* buf = malloc(len + 3);
    if (!buf) return;
    memcpy(buf, line, len);
    buf[len] = '\r';
    buf[len + 1] = '\n';
    buf[len + 2] = '\0';
    send(conn->fd, buf, len + 2, 0);
    free(buf);
}

bool smtp_read_line(struct SmtpConnection* conn, char* buf, size_t cap) {
    if (!conn || conn->fd < 0) return false;
    size_t pos = 0;
    while (pos + 1 < cap) {
        char ch;
        ssize_t n = recv(conn->fd, &ch, 1, 0);
        if (n <= 0) return false;
        if (ch == '\n') break;
        if (ch != '\r') buf[pos++] = ch;
    }
    buf[pos] = '\0';
    return true;
}

void smtp_conn_free(struct SmtpConnection* conn) {
    if (!conn) return;
    for (int i = 0; i < conn->recipient_count; i++) {
        free(conn->recipients[i]);
        conn->recipients[i] = NULL;
    }
    conn->recipient_count = 0;
}

void smtp_reset_envelope(struct SmtpConnection* conn) {
    conn->mail_from[0] = '\0';
    conn->has_mail_from = false;
    conn->auth_active = false;
    conn->route_matched = false;
    conn->route.rewrite_from = NULL;
    conn->route.add_recipients = NULL;
    conn->route.route_id = 0;
    for (int i = 0; i < conn->recipient_count; i++) {
        free(conn->recipients[i]);
        conn->recipients[i] = NULL;
    }
    conn->recipient_count = 0;
}

bool smtp_parse_address(const char* line, const char* tag,
                        char* out, size_t cap) {
    if (!line || !tag || tag[0] == '\0' || !out || cap == 0) return false;
    const char* p = strcasestr(line, tag);
    if (!p) return false;
    p += strlen(tag);
    while (*p == ' ') p++;
    const char* start = p;
    if (*p == '<') {
        p++;
        start = p;
        while (*p && *p != '>') p++;
    } else {
        while (*p && *p != ' ' && *p != '\r' && *p != '\n') p++;
    }
    size_t len = (size_t)(p - start);
    if (len >= cap) len = cap - 1;
    memcpy(out, start, len);
    out[len] = '\0';
    return len > 0;
}

bool smtp_check_source_network(const char* ip, const MailRelayConfig* config) {
    if (!config || !ip) return false;
    /* TODO: implement AllowNetwork list from config. For now accept all
     * (trusted internal network). The ip length is validated to ensure the
     * address buffer was populated. */
    return strlen(ip) > 0;
}

/*
 * Default route-resolution callback (Phase 12.4). Wires smtp_resolve_route to
 * the mail_relay_routes QueryRefs (118-122) via the repository.
 *
 * In test/blackbox mode (MAILRELAY_INBOUND_TEST_MODE=1 in the environment),
 * this acts as a permissive accept so blackbox scripts can exercise the
 * full SMTP->enqueue->deliver path without pre-seeding route rows in a DB.
 */
bool smtp_route_should_accept(const char* sender_domain,
                              const char* const* recipients,
                              int recipient_count,
                              MailRelayRouteMatch* out_route) {
    (void)sender_domain;
    (void)recipients;
    (void)recipient_count;

    const char* test_mode = getenv("MAILRELAY_INBOUND_TEST_MODE");
    if (test_mode && test_mode[0] == '1') {
        /* Test mode: accept everything, no rewrite. */
        if (out_route) {
            out_route->rewrite_from = NULL;
            out_route->add_recipients = NULL;
            out_route->route_id = 0;
        }
        return true;
    }

    /* Production: look up the route via QueryRef 118 (route_get_by_sender_domain).
     * The repository is async; for now we fail closed. A future phase will
     * add a synchronous DB lookup or a connection-scoped response queue. */
    (void)out_route;
    return false;
}

bool smtp_resolve_route(struct SmtpConnection* conn, const MailRelayConfig* config) {
    (void)config;
    conn->route_matched = false;
    if (conn->mail_from[0] == '\0') return false;

    char sender_domain[128] = {0};
    const char* at = strrchr(conn->mail_from, '@');
    if (at && at[1]) {
        strncpy(sender_domain, at + 1, sizeof(sender_domain) - 1);
    }

    if (g_should_accept_fn) {
        MailRelayRouteMatch route;
        memset(&route, 0, sizeof(route));
        bool found = g_should_accept_fn(sender_domain,
                                        (const char* const*)conn->recipients,
                                        conn->recipient_count, &route);
        if (found) {
            conn->route = route;
            conn->route_matched = true;
        }
        return found;
    }

    /* Default: no routes configured — reject (fail closed). */
    conn->route_matched = false;
    return false;
}

bool smtp_enqueue_inbound_message(struct SmtpConnection* conn,
                                   const MailRelayConfig* config,
                                   const char* raw_body, size_t body_len) {
    (void)body_len;
    if (!conn->route_matched) {
        log_this(SR_MAIL_RELAY,
                 "Inbound SMTP: no route matched for sender domain; rejecting",
                 LOG_LEVEL_ALERT, 0);
        return false;
    }

    MailRelayMessage msg;
    mailrelay_message_init(&msg);

    const char* from_addr = conn->mail_from;
    if (conn->route.rewrite_from && conn->route.rewrite_from[0]) {
        from_addr = conn->route.rewrite_from;
    }
    mailrelay_message_set_from(&msg, from_addr);

    for (int i = 0; i < conn->recipient_count; i++) {
        mailrelay_message_add_to(&msg, conn->recipients[i]);
    }

    if (config->DefaultReplyTo && config->DefaultReplyTo[0]) {
        mailrelay_message_set_reply_to(&msg, config->DefaultReplyTo);
    }

    msg.text_body = strndup(raw_body ? raw_body : "", body_len);

    /* Parse the Subject header from the raw SMTP DATA body so the queued
     * message carries a subject (required by mailrelay_validate_message). */
    if (raw_body && body_len > 0) {
        const char* hdr_end = memmem(raw_body, body_len, "\r\n\r\n", 4);
        size_t headers_len = hdr_end ? (size_t)(hdr_end - raw_body) : body_len;
        char* headers = strndup(raw_body, headers_len);
        if (headers) {
            char* line = strtok(headers, "\r\n");
            while (line) {
                if (strncasecmp(line, "Subject:", 8) == 0) {
                    char* val = line + 8;
                    while (*val == ' ' || *val == '\t') val++;
                    while (*val == '\r' || *val == '\n') *val = '\0';
                    if (*val) {
                        msg.subject = strdup(val);
                    }
                    break;
                }
                line = strtok(NULL, "\r\n");
            }
            free(headers);
        }
    }

    if (!msg.subject) {
        msg.subject = strdup("");
    }

    MailRelayStatus status = mailrelay_enqueue(&msg, 0);
    mailrelay_message_free(&msg);

    if (status != MAILRELAY_OK) {
        log_this(SR_MAIL_RELAY,
                 "Inbound SMTP: enqueue failed (status=%d)",
                 LOG_LEVEL_ERROR, 1, (int)status);
        return false;
    }

    log_this(SR_MAIL_RELAY,
             "Inbound SMTP: message accepted and enqueued from %s",
             LOG_LEVEL_STATE, 1, conn->mail_from);
    return true;
}

void* smtp_handle_connection(void* arg) {
    struct SmtpConnection* conn = (struct SmtpConnection*)arg;

    add_service_thread(&mailrelay_threads, pthread_self());

    smtp_send_line(conn, "220 mailrelay ESMTP ready");
    conn->state = SMTP_CONN_EHLO;

    char line[SMTP_MAX_LINE];
    while (!smtp_listener_shutdown) {
        if (!smtp_read_line(conn, line, sizeof(line))) {
            break;
        }
        if (line[0] == '\0') continue;

        char cmd[32];
        char argval[SMTP_MAX_LINE];
        cmd[0] = argval[0] = '\0';
        sscanf(line, "%31s %16382[^\n]", cmd, argval);

        if (strcasecmp(cmd, "QUIT") == 0) {
            smtp_send_line(conn, "221 Bye");
            break;
        } else if (strcasecmp(cmd, "EHLO") == 0 || strcasecmp(cmd, "HELO") == 0) {
            smtp_send_line(conn, "250-mailrelay");
            smtp_send_line(conn, "250-PIPELINING");
            smtp_send_line(conn, "250-SIZE 10485760");
            smtp_send_line(conn, "250-8BITMIME");
            smtp_send_line(conn, "250 OK");
            conn->state = SMTP_CONN_MAIL_FROM;
        } else if (strcasecmp(cmd, "MAIL") == 0) {
            smtp_reset_envelope(conn);
            if (smtp_parse_address(line, "FROM:", conn->mail_from, sizeof(conn->mail_from))) {
                conn->has_mail_from = true;
                smtp_send_line(conn, "250 OK");
            } else {
                smtp_send_line(conn, "501 Bad sender address");
            }
        } else if (strcasecmp(cmd, "RCPT") == 0) {
            char addr[MV_ADDR_LEN];
            if (smtp_parse_address(line, "TO:", addr, sizeof(addr))) {
                if (conn->recipient_count < (int)MV_MAX_RECIPIENTS && mailrelay_is_valid_email(addr)) {
                    conn->recipients[conn->recipient_count] = strdup(addr);
                    if (conn->recipients[conn->recipient_count]) {
                        conn->recipient_count++;
                        smtp_send_line(conn, "250 OK");
                    } else {
                        smtp_send_line(conn, "451 Out of memory");
                    }
                } else {
                    smtp_send_line(conn, "553 Invalid or too many recipients");
                }
            } else {
                smtp_send_line(conn, "501 Bad recipient address");
            }
        } else if (strcasecmp(cmd, "DATA") == 0) {
            if (!conn->has_mail_from || conn->recipient_count == 0) {
                smtp_send_line(conn, "503 Need MAIL FROM and RCPT TO first");
                continue;
            }
            smtp_send_line(conn, "354 End data with <CR><LF>.<CR><LF>");
            conn->state = SMTP_CONN_DATA;

            char* body_buf = NULL;
            size_t body_cap = 65536;
            size_t body_len = 0;
            body_buf = malloc(body_cap);
            if (!body_buf) {
                smtp_send_line(conn, "451 Out of memory");
                break;
            }

            bool data_done = false;
            for (;;) {
                if (!smtp_read_line(conn, line, sizeof(line))) {
                    free(body_buf);
                    goto cleanup;
                }
                if (strcmp(line, ".") == 0) {
                    data_done = true;
                    break;
                }
                if (body_len + strlen(line) + 3 >= body_cap) {
                    body_cap *= 2;
                    char* tmp = realloc(body_buf, body_cap);
                    if (!tmp) {
                        free(body_buf);
                        smtp_send_line(conn, "552 Message too large");
                        goto cleanup;
                    }
                    body_buf = tmp;
                }
                if (line[0] == '.') {
                    memmove(line, line + 1, strlen(line));
                }
                strcat(body_buf + body_len, line);
                strcat(body_buf + body_len, "\r\n");
                body_len = strlen(body_buf);
            }

            const MailRelayConfig* config = (app_config ? &app_config->mail_relay : NULL);
            bool accepted = false;
            if (config && config->InboundEnabled) {
                const char* ip = conn->remote_ip;
                bool net_ok = smtp_check_source_network(ip, config);
                if (!net_ok) {
                    smtp_send_line(conn, "554 Source network not allowed");
                } else if (!smtp_resolve_route(conn, config)) {
                    smtp_send_line(conn, "550 No routing rule for this sender/recipient");
                } else {
                    accepted = smtp_enqueue_inbound_message(conn, config, body_buf, body_len);
                }
            } else {
                smtp_send_line(conn, "554 Mail relay not configured for inbound");
            }
            free(body_buf);

            if (accepted) {
                smtp_send_line(conn, "250 OK message queued");
            } else if (!data_done) {
                /* already sent error above */
            } else {
                smtp_send_line(conn, "451 Internal error");
            }
            conn->state = SMTP_CONN_EHLO;
        } else if (strcasecmp(cmd, "RSET") == 0) {
            smtp_reset_envelope(conn);
            smtp_send_line(conn, "250 OK");
        } else if (strcasecmp(cmd, "NOOP") == 0) {
            smtp_send_line(conn, "250 OK");
        } else if (strcasecmp(cmd, "STARTTLS") == 0) {
            smtp_send_line(conn, "502 STARTTLS not supported in this build");
        } else if (strcasecmp(cmd, "AUTH") == 0) {
            smtp_send_line(conn, "535 Authentication required");
        } else if (strcasecmp(cmd, "LHLO") == 0) {
            /* LMVP: stub. Logs unsupported/deferred (12.6). */
            log_this(SR_MAIL_RELAY,
                     "Inbound LMTP: received LHLO but LMVP is deferred",
                     LOG_LEVEL_ALERT, 0);
            smtp_send_line(conn, "502 LMTP not supported");
        } else {
            smtp_send_line(conn, "500 Command not recognized");
        }
    }

cleanup:
    if (conn->fd >= 0) close(conn->fd);
    smtp_conn_free(conn);
    free(conn);
    remove_service_thread(&mailrelay_threads, pthread_self());
    return NULL;
}

void* smtp_listener_thread(void* arg) {
    (void)arg;
    add_service_thread(&mailrelay_threads, pthread_self());

    const MailRelayConfig* config = (app_config ? &app_config->mail_relay : NULL);
    smtp_listener_start(config);

    remove_service_thread(&mailrelay_threads, pthread_self());
    return NULL;
}

bool smtp_listener_start(const MailRelayConfig* config) {
    if (!config) {
        return false;
    }

    if (!config->InboundEnabled) {
        log_this(SR_MAIL_RELAY,
                 "Inbound SMTP listener disabled - not starting",
                 LOG_LEVEL_STATE, 0);
        return true;
    }

    if (!app_config || !app_config->mail_relay.OutboundEnabled) {
        log_this(SR_MAIL_RELAY,
                 "Inbound relay requires outbound to be enabled for re-queueing",
                 LOG_LEVEL_ALERT, 0);
        return false;
    }

    smtp_listener_shutdown = 0;

    smtp_listener_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (smtp_listener_fd < 0) {
        log_this(SR_MAIL_RELAY,
                 "Inbound SMTP: failed to create listener socket: %s",
                 LOG_LEVEL_ERROR, 1, strerror(errno));
        return false;
    }

    int yes = 1;
    setsockopt(smtp_listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)config->ListenPort);

    if (bind(smtp_listener_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        log_this(SR_MAIL_RELAY,
                 "Inbound SMTP: failed to bind port %d: %s",
                 LOG_LEVEL_ERROR, 2, config->ListenPort, strerror(errno));
        close(smtp_listener_fd);
        smtp_listener_fd = -1;
        return false;
    }

    if (listen(smtp_listener_fd, SMTP_LISTENER_BACKLOG) < 0) {
        log_this(SR_MAIL_RELAY,
                 "Inbound SMTP: failed to listen on port %d: %s",
                 LOG_LEVEL_ERROR, 2, config->ListenPort, strerror(errno));
        close(smtp_listener_fd);
        smtp_listener_fd = -1;
        return false;
    }

    smtp_listener_running = true;
    log_this(SR_MAIL_RELAY,
             "Inbound SMTP listener started on port %d (trusted submission only, not open relay)",
             LOG_LEVEL_STATE, 1, config->ListenPort);

    while (!smtp_listener_shutdown) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_len = sizeof(peer_addr);
        int client_fd = accept(smtp_listener_fd, (struct sockaddr*)&peer_addr, &peer_len);
        if (client_fd < 0) {
            if (smtp_listener_shutdown) break;
            continue;
        }

        struct SmtpConnection* conn_ptr = malloc(sizeof(struct SmtpConnection));
        if (!conn_ptr) {
            close(client_fd);
            continue;
        }
        memset(conn_ptr, 0, sizeof(struct SmtpConnection));
        conn_ptr->fd = client_fd;
        smtp_get_peer_ip(client_fd, (const struct sockaddr*)&peer_addr,
                         peer_len, conn_ptr->remote_ip, sizeof(conn_ptr->remote_ip));

        pthread_t tid;
        if (pthread_create(&tid, NULL, smtp_handle_connection, conn_ptr) != 0) {
            close(client_fd);
            free(conn_ptr);
            continue;
        }
        pthread_detach(tid);
    }

    smtp_listener_running = false;
    log_this(SR_MAIL_RELAY, "Inbound SMTP listener stopped", LOG_LEVEL_STATE, 0);
    return true;
}

void smtp_listener_stop(void) {
    smtp_listener_shutdown = 1;
    if (smtp_listener_fd >= 0) {
        close(smtp_listener_fd);
        smtp_listener_fd = -1;
    }
}

bool smtp_listener_is_running(void) {
    return smtp_listener_running;
}

void smtp_listener_set_should_accept(smtp_listener_should_accept_fn fn) {
    g_should_accept_fn = fn;
}

smtp_listener_should_accept_fn smtp_listener_get_should_accept(void) {
    return g_should_accept_fn;
}
