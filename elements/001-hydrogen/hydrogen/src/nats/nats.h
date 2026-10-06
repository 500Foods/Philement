/*
 * NATS subsystem public surface.
 *
 * One plaintext connection. Publish, subscribe, and the reconnect loop
 * share that socket. nats_broadcast sends one JSON envelope through
 * nats_client_publish. io->write returns 0 on a full write and -1 on
 * error. io->read returns a byte count, 0 on EOF, -1 on error, and -2
 * when the read timed out with no bytes.
 */

#ifndef NATS_H
#define NATS_H

#include <stddef.h>
#include <stdbool.h>

#include <jansson.h>

typedef struct NatsIo {
    int (*connect_fn)(void *ctx, const char *host, int port, int timeout_seconds);
    int (*read_fn)(void *ctx, void *buf, size_t len);
    int (*write_fn)(void *ctx, const void *buf, size_t len);
    void (*close_fn)(void *ctx);
    void *ctx;
} NatsIo;

typedef void (*NatsMsgHandler)(const char *subject, const char *sid,
                               const char *reply, const void *data, size_t len);

int nats_start(void);
void nats_shutdown(void);

/* down, degraded, or up. Unknown values are down. */
const char *nats_link_name(int state);
const char *nats_link_state_name(void);

void nats_io_install(const NatsIo *io);
void nats_msg_set_handler(NatsMsgHandler handler);

int nats_session_handshake(void);
int nats_session_send_unsubs(void);
int nats_parser_feed(const void *data, size_t len);
int nats_parser_take_ctrl(void *dst, size_t cap);

int nats_client_publish(const char *subject, const void *data, size_t len);
int nats_broadcast(const char *event, const json_t *data);

/* Same envelope as nats_broadcast. The subject suffix is the event.
 * Does not evict. cache.invalidate_by_ref and app_state return -1. */
int nats_publish_event(const char *event, const json_t *data);

#define NATS_REGISTRY_COPY_CAP 64

/* state is "Starting" or "Alive". Stopping peers are not copied. */
typedef struct NatsRegistryCopy {
    char id[128];
    char state[16];
    int websocket_connections;
} NatsRegistryCopy;

int nats_registry_copy(NatsRegistryCopy *out, size_t cap);

int nats_client_flush_outbound(void);
void nats_client_reset(void);

int nats_reconnect_delay_seconds(int failure_number);
bool nats_reconnect_should_retry(int failure_number);

#endif /* NATS_H */
