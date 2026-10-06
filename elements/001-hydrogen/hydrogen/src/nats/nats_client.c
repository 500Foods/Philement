/*
 * NATS plaintext session.
 *
 * One connection. The outbound queue survives a failed handshake.
 * Parser state and the SUB table are rebuilt on every successful CONNECT.
 * The handler must not take the client mutex. Presence subscribe,
 * announce, tick, and link-down live in nats_registry.c.
 */

#include <src/hydrogen.h>

#include <src/config/config_nats.h>
#include <src/nats/nats_internal.h>
#include <src/nats/nats_stats.h>
#include <src/nats/nats_subject.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct NatsOutbound {
    char *subject;
    unsigned char *body;
    size_t len;
} NatsOutbound;

typedef struct NatsHeldSub {
    char *subject;
    char *queue;
    int sid;
} NatsHeldSub;

static pthread_mutex_t nats_client_mu = PTHREAD_MUTEX_INITIALIZER;
static unsigned char *nats_parse_buf;
static size_t nats_parse_len;
static size_t nats_parse_cap;
static bool nats_in_payload;
static size_t nats_payload_need;
static char nats_msg_subject[NATS_SUBJECT_CAP];
static char nats_msg_sid[NATS_SID_CAP];
static char nats_msg_reply[NATS_REPLY_CAP];
static bool nats_info_saw;
static bool nats_auth_required;
static bool nats_tls_required;
static size_t nats_max_payload = NATS_DEFAULT_MAX_PAYLOAD;
static char nats_ctrl[NATS_CTRL_CAP];
static size_t nats_ctrl_len;
static NatsMsgHandler nats_handler = nats_on_msg;
static NatsOutbound nats_out[NATS_OUTBOUND_SLOTS];
static size_t nats_out_head;
static size_t nats_out_count;
static bool nats_flush_busy;
static NatsHeldSub nats_held[NATS_MAX_SUBSCRIPTIONS];
static size_t nats_held_count;
static size_t nats_server_cursor;

void nats_msg_set_handler(NatsMsgHandler handler) {
    nats_handler = handler ? handler : nats_on_msg;
}

int nats_parse_append(const void *data, size_t len) {
    const unsigned char *bytes = data;
    size_t need;
    size_t cap;

    if (len == 0) {
        return 0;
    }
    if (!bytes) {
        return -1;
    }
    need = nats_parse_len + len + 1;
    if (need > NATS_PAYLOAD_CEILING + 65536) {
        return -1;
    }
    cap = nats_parse_cap;
    if (need > cap) {
        unsigned char *grown;

        if (cap < 1024) {
            cap = 1024;
        }
        while (cap < need) {
            if (cap > (NATS_PAYLOAD_CEILING + 65536) / 2) {
                cap = NATS_PAYLOAD_CEILING + 65536;
                break;
            }
            cap *= 2;
        }
        if (cap < need) {
            return -1;
        }
        grown = realloc(nats_parse_buf, cap);
        if (!grown) {
            return -1;
        }
        nats_parse_buf = grown;
        nats_parse_cap = cap;
    }
    memcpy(nats_parse_buf + nats_parse_len, bytes, len);
    nats_parse_len += len;
    nats_parse_buf[nats_parse_len] = '\0';
    return 0;
}

void nats_parse_consume(size_t count) {
    if (!nats_parse_buf || count == 0) {
        return;
    }
    if (count > nats_parse_len) {
        count = nats_parse_len;
    }
    memmove(nats_parse_buf, nats_parse_buf + count, nats_parse_len - count);
    nats_parse_len -= count;
    nats_parse_buf[nats_parse_len] = '\0';
}

void nats_parser_reset(void) {
    nats_parse_len = 0;
    nats_in_payload = false;
    nats_payload_need = 0;
    nats_info_saw = false;
    nats_auth_required = false;
    nats_tls_required = false;
    nats_ctrl_len = 0;
    nats_msg_subject[0] = '\0';
    nats_msg_sid[0] = '\0';
    nats_msg_reply[0] = '\0';
    if (nats_parse_buf && nats_parse_cap > 0) {
        nats_parse_buf[0] = '\0';
    }
    if (pthread_mutex_lock(&nats_client_mu) == 0) {
        nats_max_payload = NATS_DEFAULT_MAX_PAYLOAD;
        pthread_mutex_unlock(&nats_client_mu);
    }
}

int nats_ctrl_append(const char *text, size_t len) {
    if (!text) {
        return -1;
    }
    if (nats_ctrl_len + len > NATS_CTRL_CAP) {
        return -1;
    }
    memcpy(nats_ctrl + nats_ctrl_len, text, len);
    nats_ctrl_len += len;
    return 0;
}

int nats_parser_take_ctrl(void *dst, size_t cap) {
    char *out = dst;
    size_t len;

    if (!out) {
        return -1;
    }
    len = nats_ctrl_len;
    if (len == 0) {
        if (cap > 0) {
            out[0] = '\0';
        }
        return 0;
    }
    if (cap < len) {
        return -1;
    }
    memcpy(out, nats_ctrl, len);
    if (cap > len) {
        out[len] = '\0';
    }
    nats_ctrl_len = 0;
    return (int)len;
}

int nats_session_flush_ctrl(void) {
    char buf[NATS_CTRL_CAP + 1];
    int n = nats_parser_take_ctrl(buf, sizeof(buf));

    if (n < 0) {
        return -1;
    }
    if (n == 0) {
        return 0;
    }
    return nats_io_write(buf, (size_t)n);
}

int nats_parse_info(const char *json_text) {
    json_t *root;
    json_t *field;

    if (!json_text) {
        return -1;
    }
    root = json_loads(json_text, 0, NULL);
    if (!json_is_object(root)) {
        json_decref(root);
        return -1;
    }
    field = json_object_get(root, "tls_required");
    nats_tls_required = json_is_true(field);
    field = json_object_get(root, "auth_required");
    nats_auth_required = json_is_true(field);
    field = json_object_get(root, "max_payload");
    if (json_is_integer(field)) {
        json_int_t value = json_integer_value(field);

        if (value < 0 || value > (json_int_t)NATS_PAYLOAD_CEILING) {
            json_decref(root);
            return -1;
        }
        if (pthread_mutex_lock(&nats_client_mu) != 0) {
            json_decref(root);
            return -1;
        }
        nats_max_payload = (size_t)value;
        pthread_mutex_unlock(&nats_client_mu);
    }
    json_decref(root);
    nats_info_saw = true;
    return 0;
}

int nats_parse_msg_header(const char *rest) {
    char tok[4][NATS_SUBJECT_CAP];
    const char *cursor;
    const char *subject;
    const char *sid;
    const char *reply;
    const char *size_text;
    char *end = NULL;
    unsigned long size;
    size_t limit;
    int count = 0;

    if (!rest) {
        return -1;
    }
    memset(tok, 0, sizeof(tok));
    cursor = rest;
    while (count < 4) {
        const char *next = nats_next_token(cursor, tok[count], NATS_SUBJECT_CAP);

        if (!next) {
            break;
        }
        count++;
        cursor = next;
    }
    while (*cursor == ' ') {
        cursor++;
    }
    if (*cursor != '\0' || (count != 3 && count != 4)) {
        return -1;
    }
    subject = tok[0];
    sid = tok[1];
    reply = "";
    size_text = tok[2];
    if (count == 4) {
        reply = tok[2];
        size_text = tok[3];
    }
    errno = 0;
    size = strtoul(size_text, &end, 10);
    if (end == size_text || !end || *end != '\0' || errno == ERANGE) {
        return -1;
    }
    if (size > NATS_PAYLOAD_CEILING) {
        return -1;
    }
    if (pthread_mutex_lock(&nats_client_mu) != 0) {
        return -1;
    }
    limit = nats_max_payload;
    pthread_mutex_unlock(&nats_client_mu);
    if ((size_t)size > limit) {
        return -1;
    }
    {
        size_t subject_len = strlen(subject);
        size_t sid_len = strlen(sid);
        size_t reply_len = strlen(reply);

        if (subject_len >= sizeof(nats_msg_subject) ||
            sid_len >= sizeof(nats_msg_sid) ||
            reply_len >= sizeof(nats_msg_reply)) {
            return -1;
        }
        memcpy(nats_msg_subject, subject, subject_len + 1);
        memcpy(nats_msg_sid, sid, sid_len + 1);
        memcpy(nats_msg_reply, reply, reply_len + 1);
    }
    nats_payload_need = (size_t)size + 2;
    nats_in_payload = true;
    return 0;
}

int nats_parse_line(const char *line) {
    char op[16];
    const char *rest;

    if (!line || line[0] == '\0') {
        return 0;
    }
    rest = nats_next_token(line, op, sizeof(op));
    if (!rest) {
        return -1;
    }
    if (strcmp(op, "INFO") == 0) {
        return nats_parse_info(rest);
    }
    if (strcmp(op, "PING") == 0) {
        return nats_ctrl_append("PONG\r\n", 6);
    }
    if (strcmp(op, "PONG") == 0 || strcmp(op, "+OK") == 0) {
        return 0;
    }
    if (strcmp(op, "-ERR") == 0) {
        return -1;
    }
    if (strcmp(op, "MSG") == 0) {
        return nats_parse_msg_header(rest);
    }
    return -1;
}

int nats_parser_drain(void) {
    while (nats_parse_len > 0) {
        if (nats_in_payload) {
            size_t body;
            const char *reply;

            if (nats_parse_len < nats_payload_need) {
                return 0;
            }
            if (nats_payload_need < 2 ||
                nats_parse_buf[nats_payload_need - 2] != '\r' ||
                nats_parse_buf[nats_payload_need - 1] != '\n') {
                return -1;
            }
            body = nats_payload_need - 2;
            reply = nats_msg_reply[0] != '\0' ? nats_msg_reply : NULL;
            if (nats_handler) {
                nats_handler(nats_msg_subject, nats_msg_sid, reply,
                             body > 0 ? nats_parse_buf : NULL, body);
            }
            nats_in_payload = false;
            nats_parse_consume(nats_payload_need);
            continue;
        }

        {
            size_t limit = nats_parse_len < NATS_LINE_CAP ? nats_parse_len : NATS_LINE_CAP;
            size_t i;
            bool found = false;

            for (i = 0; i + 1 < limit; i++) {
                if (nats_parse_buf[i] == '\r' && nats_parse_buf[i + 1] == '\n') {
                    found = true;
                    break;
                }
            }
            if (!found) {
                if (nats_parse_len >= NATS_LINE_CAP) {
                    return -1;
                }
                return 0;
            }
            nats_parse_buf[i] = '\0';
            if (nats_parse_line((const char *)nats_parse_buf) != 0) {
                return -1;
            }
            nats_parse_consume(i + 2);
        }
    }
    return 0;
}

int nats_parser_feed(const void *data, size_t len) {
    if (nats_parse_append(data, len) != 0) {
        return -1;
    }
    return nats_parser_drain();
}

void nats_session_clear_held(void) {
    size_t i;

    for (i = 0; i < nats_held_count; i++) {
        free(nats_held[i].subject);
        free(nats_held[i].queue);
        nats_held[i].subject = NULL;
        nats_held[i].queue = NULL;
        nats_held[i].sid = 0;
    }
    nats_held_count = 0;
}

int nats_session_send_subs(void) {
    const NATSConfig *cfg;
    size_t i;

    nats_session_clear_held();
    if (!app_config) {
        return -1;
    }
    cfg = &app_config->nats;
    for (i = 0; i < cfg->SubscriptionCount && i < NATS_MAX_SUBSCRIPTIONS; i++) {
        char *subject;
        const char *queue = NULL;
        char line[NATS_CTRL_CAP];
        int written;
        int sid = (int)i + 1;

        subject = nats_subject_build(cfg->ClusterId, cfg->Subscriptions[i].Subject);
        if (!subject) {
            return -1;
        }
        if (cfg->Subscriptions[i].Type &&
            strcmp(cfg->Subscriptions[i].Type, "queue-group") == 0) {
            queue = cfg->Subscriptions[i].QueueGroup;
        }
        written = nats_frame_sub(line, sizeof(line), subject, queue, sid);
        if (written < 0) {
            free(subject);
            return -1;
        }
        nats_held[nats_held_count].subject = subject;
        nats_held[nats_held_count].queue = NULL;
        nats_held[nats_held_count].sid = sid;
        if (queue && queue[0] != '\0') {
            nats_held[nats_held_count].queue = strdup(queue);
            if (!nats_held[nats_held_count].queue) {
                nats_held_count++;
                return -1;
            }
        }
        nats_held_count++;
        if (nats_io_write(line, (size_t)written) != 0) {
            return -1;
        }
    }
    return 0;
}

int nats_session_send_unsubs(void) {
    size_t i;

    for (i = 0; i < nats_held_count; i++) {
        char line[64];
        int written = nats_frame_unsub(line, sizeof(line), nats_held[i].sid);

        if (written < 0 || nats_io_write(line, (size_t)written) != 0) {
            return -1;
        }
    }
    return 0;
}

int nats_session_handshake_try(void) {
    char host[NATS_HOST_CAP];
    unsigned char chunk[512];
    const NATSConfig *cfg;
    size_t index;
    int port = 0;
    int timeout;
    int waits = 0;
    int wait_limit;

    if (!app_config) {
        return -1;
    }
    cfg = &app_config->nats;
    if (cfg->ServerCount == 0 || cfg->ServerCount > NATS_MAX_SERVERS) {
        return -1;
    }
    nats_parser_reset();
    index = nats_server_cursor % cfg->ServerCount;
    if (!cfg->Servers[index] ||
        !nats_server_endpoint(cfg->Servers[index], host, sizeof(host), &port)) {
        return -1;
    }
    timeout = cfg->ConnectionTimeoutSeconds;
    if (timeout < 1) {
        timeout = 10;
    }
    if (timeout > 120) {
        timeout = 120;
    }
    if (nats_io_connect(host, port, timeout) != 0) {
        return -1;
    }
    /* Each read waits about 200ms. Stop after the connect budget. */
    wait_limit = timeout * 5;
    while (!nats_info_saw) {
        int n;

        if (nats_system_shutdown) {
            return -1;
        }
        n = nats_io_read(chunk, sizeof(chunk));
        if (n == -2) {
            waits++;
            if (waits > wait_limit) {
                return -1;
            }
            continue;
        }
        if (n <= 0) {
            return -1;
        }
        if (nats_parser_feed(chunk, (size_t)n) != 0) {
            return -1;
        }
        if (nats_session_flush_ctrl() != 0) {
            return -1;
        }
    }
    if (nats_tls_required) {
        return -1;
    }
    if (nats_auth_required && (!cfg->Username || cfg->Username[0] == '\0')) {
        return -1;
    }
    if (nats_connect_send() != 0) {
        return -1;
    }
    if (nats_session_send_subs() != 0) {
        return -1;
    }
    if (nats_registry_subscribe() != 0 || nats_registry_announce_up() != 0) {
        return -1;
    }
    nats_link_set(NATS_LINK_UP);
    return 0;
}

int nats_session_handshake(void) {
    if (nats_session_handshake_try() != 0) {
        nats_io_close();
        nats_link_set(NATS_LINK_DEGRADED);
        nats_server_cursor++;
        return -1;
    }
    return 0;
}

int nats_session_once(void) {
    unsigned char chunk[1024];

    if (nats_session_handshake() != 0) {
        return -1;
    }
    if (nats_client_flush_outbound() != 0) {
        if (nats_link_get() == NATS_LINK_UP) {
            nats_registry_on_link_down();
            nats_session_send_unsubs();
            if (!nats_system_shutdown) {
                nats_stats_inc_reconnects();
            }
        }
        nats_io_close();
        nats_link_set(NATS_LINK_DEGRADED);
        return 1;
    }
    while (!nats_system_shutdown) {
        int n;

        if (nats_registry_tick() != 0) {
            break;
        }
        n = nats_io_read(chunk, sizeof(chunk));

        if (n == -2) {
            if (nats_client_flush_outbound() != 0) {
                break;
            }
            continue;
        }
        if (n <= 0) {
            break;
        }
        if (nats_parser_feed(chunk, (size_t)n) != 0 ||
            nats_session_flush_ctrl() != 0 ||
            nats_client_flush_outbound() != 0) {
            break;
        }
    }
    if (nats_link_get() == NATS_LINK_UP) {
        nats_registry_on_link_down();
        nats_session_send_unsubs();
        if (!nats_system_shutdown) {
            nats_stats_inc_reconnects();
        }
    }
    nats_io_close();
    nats_link_set(NATS_LINK_DEGRADED);
    if (nats_system_shutdown) {
        return 0;
    }
    return 1;
}

void nats_client_reset(void) {
    if (pthread_mutex_lock(&nats_client_mu) == 0) {
        size_t i;

        for (i = 0; i < NATS_OUTBOUND_SLOTS; i++) {
            free(nats_out[i].subject);
            free(nats_out[i].body);
            nats_out[i].subject = NULL;
            nats_out[i].body = NULL;
            nats_out[i].len = 0;
        }
        nats_out_head = 0;
        nats_out_count = 0;
        nats_flush_busy = false;
        nats_max_payload = NATS_DEFAULT_MAX_PAYLOAD;
        pthread_mutex_unlock(&nats_client_mu);
    }
    nats_session_clear_held();
    nats_parser_reset();
    nats_msg_set_handler(nats_on_msg);
    nats_server_cursor = 0;
    nats_registry_reset();
    free(nats_parse_buf);
    nats_parse_buf = NULL;
    nats_parse_len = 0;
    nats_parse_cap = 0;
}

int nats_client_publish(const char *subject, const void *data, size_t len) {
    char *copy_subject;
    unsigned char *copy_body = NULL;
    size_t slot;
    size_t limit;

    if (app_config && app_config->nats.Test.FailNextPublishOnLaunch) {
        app_config->nats.Test.FailNextPublishOnLaunch = false;
        return -1;
    }
    if (!nats_token_ok(subject) || strlen(subject) >= NATS_SUBJECT_CAP ||
        (len > 0 && !data)) {
        return -1;
    }
    if (pthread_mutex_lock(&nats_client_mu) != 0) {
        return -1;
    }
    limit = nats_max_payload;
    if (len > limit || nats_out_count >= NATS_OUTBOUND_SLOTS) {
        pthread_mutex_unlock(&nats_client_mu);
        return -1;
    }
    pthread_mutex_unlock(&nats_client_mu);

    copy_subject = strdup(subject);
    if (!copy_subject) {
        return -1;
    }
    if (len > 0) {
        copy_body = malloc(len);
        if (!copy_body) {
            free(copy_subject);
            return -1;
        }
        memcpy(copy_body, data, len);
    }
    if (pthread_mutex_lock(&nats_client_mu) != 0) {
        free(copy_subject);
        free(copy_body);
        return -1;
    }
    if (len > nats_max_payload || nats_out_count >= NATS_OUTBOUND_SLOTS) {
        pthread_mutex_unlock(&nats_client_mu);
        free(copy_subject);
        free(copy_body);
        return -1;
    }
    slot = (nats_out_head + nats_out_count) % NATS_OUTBOUND_SLOTS;
    nats_out[slot].subject = copy_subject;
    nats_out[slot].body = copy_body;
    nats_out[slot].len = len;
    nats_out_count++;
    pthread_mutex_unlock(&nats_client_mu);
    if (nats_link_get() == NATS_LINK_UP) {
        return nats_client_flush_outbound();
    }
    return 0;
}

int nats_client_flush_outbound(void) {
    if (nats_link_get() != NATS_LINK_UP) {
        return 0;
    }
    if (pthread_mutex_lock(&nats_client_mu) != 0) {
        return -1;
    }
    if (nats_flush_busy) {
        pthread_mutex_unlock(&nats_client_mu);
        return 0;
    }
    nats_flush_busy = true;
    pthread_mutex_unlock(&nats_client_mu);

    while (nats_link_get() == NATS_LINK_UP) {
        char *subject = NULL;
        unsigned char *body = NULL;
        size_t len = 0;
        char header[NATS_CTRL_CAP];
        int header_len;
        bool failed = false;

        if (pthread_mutex_lock(&nats_client_mu) != 0) {
            return -1;
        }
        if (nats_out_count == 0) {
            nats_flush_busy = false;
            pthread_mutex_unlock(&nats_client_mu);
            return 0;
        }
        subject = nats_out[nats_out_head].subject;
        body = nats_out[nats_out_head].body;
        len = nats_out[nats_out_head].len;
        nats_out[nats_out_head].subject = NULL;
        nats_out[nats_out_head].body = NULL;
        nats_out[nats_out_head].len = 0;
        nats_out_head = (nats_out_head + 1) % NATS_OUTBOUND_SLOTS;
        nats_out_count--;
        pthread_mutex_unlock(&nats_client_mu);

        header_len = nats_frame_pub_header(header, sizeof(header), subject, len);
        if (header_len < 0 || nats_io_write(header, (size_t)header_len) != 0) {
            failed = true;
        } else if (len > 0 && nats_io_write(body, len) != 0) {
            failed = true;
        } else if (nats_io_write("\r\n", 2) != 0) {
            failed = true;
        }
        if (failed) {
            if (pthread_mutex_lock(&nats_client_mu) != 0) {
                free(subject);
                free(body);
                return -1;
            }
            nats_out_head = (nats_out_head + NATS_OUTBOUND_SLOTS - 1) % NATS_OUTBOUND_SLOTS;
            nats_out[nats_out_head].subject = subject;
            nats_out[nats_out_head].body = body;
            nats_out[nats_out_head].len = len;
            nats_out_count++;
            nats_flush_busy = false;
            pthread_mutex_unlock(&nats_client_mu);
            return -1;
        }
        nats_stats_inc_published();
        free(subject);
        free(body);
    }

    if (pthread_mutex_lock(&nats_client_mu) == 0) {
        nats_flush_busy = false;
        pthread_mutex_unlock(&nats_client_mu);
    }
    return 0;
}
