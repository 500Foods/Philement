/*
 * Generic TCP/TLS listener. Binds a port, accepts connections (optionally
 * wrapping in TLS for implicit-TLS ports or leaving clear for STARTTLS), and
 * dispatches each connection to a protocol module. The module may later call
 * mv_conn_starttls() to upgrade a cleartext connection.
 */

#ifndef MV_LISTENER_H
#define MV_LISTENER_H

#include <openssl/ssl.h>

typedef struct proto_module proto_module;

typedef struct {
    int fd;
    SSL* ssl;
    bool tls;
    SSL_CTX* ctx;   /* available for STARTTLS upgrade */
} mv_conn;

/* Read one line (terminated by \n; trailing \r and \n stripped). Returns the
 * number of characters placed in buf (excluding NUL) or -1 on EOF/error. */
int mv_conn_readline(mv_conn* c, char* buf, int n);

/* Read up to n bytes (blocking until n or EOF). Returns count or -1 on error. */
int mv_conn_read(mv_conn* c, char* buf, int n);

/* Write raw bytes. Returns 0 on success, -1 on error. */
int mv_conn_write(mv_conn* c, const char* data, int len);

/* Formatted write. Returns 0 on success. */
int mv_conn_printf(mv_conn* c, const char* fmt, ...);

/* Upgrade a cleartext connection to TLS using c->ctx. Returns 0 on success. */
int mv_conn_starttls(mv_conn* c);

/* Close connection (and TLS session if any). */
void mv_conn_close(mv_conn* c);

typedef struct {
    const char* host;
    int port;
    bool implicit_tls;
    SSL_CTX* ctx;
    proto_module* mod;
} listen_spec;

/* Block: listen and serve until process exit. Returns 0 on clean exit. */
int mv_listen_and_serve(const listen_spec* spec);

#endif /* MV_LISTENER_H */
