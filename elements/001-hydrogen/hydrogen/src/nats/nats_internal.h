/*
 * Shared NATS client declarations. One connection, no pool.
 */

#ifndef NATS_INTERNAL_H
#define NATS_INTERNAL_H

#include <src/nats/nats.h>

#include <sys/socket.h>

#define NATS_HOST_CAP 256
#define NATS_SUBJECT_CAP 256
#define NATS_SID_CAP 32
#define NATS_REPLY_CAP 256
#define NATS_CTRL_CAP 1024
#define NATS_LINE_CAP 4096
#define NATS_OUTBOUND_SLOTS 8
#define NATS_DEFAULT_MAX_PAYLOAD (1024 * 1024)
#define NATS_PAYLOAD_CEILING (8 * 1024 * 1024)
#define NATS_LINK_DOWN 0
#define NATS_LINK_DEGRADED 1
#define NATS_LINK_UP 2

void nats_link_set(int state);
int nats_link_get(void);

void nats_io_use_config(void);
int nats_io_connect(const char *host, int port, int timeout_seconds);
int nats_io_read(void *buf, size_t len);
int nats_io_write(const void *buf, size_t len);
void nats_io_close(void);

int nats_io_tcp_connect(void *ctx, const char *host, int port, int timeout_seconds);
int nats_io_tcp_read(void *ctx, void *buf, size_t len);
int nats_io_tcp_write(void *ctx, const void *buf, size_t len);
void nats_io_tcp_close(void *ctx);
int nats_io_connect_fd(int fd, const struct sockaddr *addr, socklen_t len,
                       int timeout_seconds);

int nats_io_mock_connect(void *ctx, const char *host, int port, int timeout_seconds);
int nats_io_mock_read(void *ctx, void *buf, size_t len);
int nats_io_mock_write(void *ctx, const void *buf, size_t len);
void nats_io_mock_close(void *ctx);

bool nats_token_ok(const char *text);
const char *nats_next_token(const char *text, char *out, size_t cap);
int nats_frame_sub(char *dst, size_t cap, const char *subject, const char *queue, int sid);
int nats_frame_unsub(char *dst, size_t cap, int sid);
int nats_frame_pub_header(char *dst, size_t cap, const char *subject, size_t len);
int nats_connect_send(void);

const char *nats_broadcast_suffix(const char *event);
bool nats_broadcast_data_ok(const json_t *data);
int nats_broadcast_timestamp(char *dst, size_t cap);

int nats_session_once(void);
int nats_session_flush_ctrl(void);

void nats_reconnect_wake(void);
void nats_reconnect_wait(int seconds);
void *nats_reconnect_thread(void *arg);

void nats_on_msg(const char *subject, const char *sid, const char *reply,
                 const void *data, size_t len);

void nats_dispatch_message(const char *wire_subject, const void *data, size_t len);
void nats_invalidate_query_ref(const char *database, json_int_t query_ref);
void nats_dispatch_invalidate(const char *database, json_int_t query_ref,
                              const char *reason);
void nats_dispatch_set_invalidate(void (*hook)(const char *, json_int_t,
                                               const char *));
bool nats_dispatch_fields_ok(const json_t *root);
bool nats_dispatch_subject_ok(const char *wire_subject, const json_t *root);
bool nats_dispatch_is_self(const json_t *root);

int nats_parse_append(const void *data, size_t len);
void nats_parse_consume(size_t count);
void nats_parser_reset(void);
int nats_parser_drain(void);
int nats_parse_line(const char *line);
int nats_parse_info(const char *json_text);
int nats_parse_msg_header(const char *rest);
int nats_ctrl_append(const char *text, size_t len);
void nats_session_clear_held(void);
int nats_session_send_subs(void);
int nats_session_handshake_try(void);

#endif /* NATS_INTERNAL_H */
