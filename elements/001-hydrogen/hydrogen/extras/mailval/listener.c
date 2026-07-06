#include "mailval.h"
#include "capture.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int mv_conn_raw_read(mv_conn* c, char* b, int n) {
    if (c->tls) {
        int r = SSL_read(c->ssl, b, n);
        if (r > 0) return r;
        int e = SSL_get_error(c->ssl, r);
        if (e == SSL_ERROR_WANT_READ || e == SSL_ERROR_WANT_WRITE) return -2; /* retry */
        return -1;
    }
    ssize_t r = recv(c->fd, b, (size_t)n, 0);
    return r > 0 ? (int)r : -1;
}

static int mv_conn_raw_write(mv_conn* c, const char* b, int n) {
    if (c->tls) {
        int r = SSL_write(c->ssl, b, n);
        if (r > 0) return r;
        int e = SSL_get_error(c->ssl, r);
        if (e == SSL_ERROR_WANT_READ || e == SSL_ERROR_WANT_WRITE) return -2;
        return -1;
    }
    ssize_t r = send(c->fd, b, (size_t)n, 0);
    if (r > 0) return (int)r;
    if (r < 0 && errno == EAGAIN) return -2;
    return -1;
}

int mv_conn_read(mv_conn* c, char* buf, int n) {
    int got = 0;
    while (got < n) {
        int r = mv_conn_raw_read(c, buf + got, n - got);
        if (r == -2) continue;
        if (r < 0) return (got > 0) ? got : -1;
        got += r;
    }
    return got;
}

int mv_conn_readline(mv_conn* c, char* buf, int n) {
    int i = 0;
    while (i < n - 1) {
        char ch = 0;
        int r = mv_conn_raw_read(c, &ch, 1);
        if (r == -2) continue;
        if (r < 0) { if (i == 0) return -1; break; }
        if (ch == '\n') break;
        if (ch == '\r') continue;
        buf[i++] = ch;
    }
    buf[i] = '\0';
    return i;
}

int mv_conn_write(mv_conn* c, const char* data, int len) {
    int sent = 0;
    while (sent < len) {
        int r = mv_conn_raw_write(c, data + sent, len - sent);
        if (r == -2) continue;
        if (r < 0) return -1;
        sent += r;
    }
    return 0;
}

int mv_conn_printf(mv_conn* c, const char* fmt, ...) {
    char tmp[MV_MAX_LINE * 2];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0) return -1;
    if (n >= (int)sizeof(tmp)) n = (int)sizeof(tmp) - 1;
    return mv_conn_raw_write(c, tmp, n);
}

int mv_conn_starttls(mv_conn* c) {
    if (c->tls || !c->ctx) return -1;
    SSL* ssl = mv_tls_accept_fd(c->ctx, c->fd);
    if (!ssl) return -1;
    c->ssl = ssl;
    c->tls = true;
    return 0;
}

void mv_conn_close(mv_conn* c) {
    if (c->tls && c->ssl) { SSL_shutdown(c->ssl); SSL_free(c->ssl); c->ssl = NULL; }
    if (c->fd >= 0) close(c->fd);
    c->fd = -1;
}

typedef struct {
    mv_conn conn;
    char peer[64];
    proto_module* mod;
    SSL_CTX* ctx;
} session_arg;

static void* session_thread(void* arg) {
    session_arg* sa = arg;
    mv_capture* cap = mv_capture_create(g_cfg.data_dir, sa->mod->name, sa->peer);
    sa->mod->handle(&sa->conn, sa->peer, cap);
    mv_capture_close(cap);
    mv_conn_close(&sa->conn);
    free(sa);
    return NULL;
}

int mv_listen_and_serve(const listen_spec* spec) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)spec->port);
    addr.sin_addr.s_addr = (spec->host && strcmp(spec->host, "*") != 0)
        ? inet_addr(spec->host) : INADDR_ANY;
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(fd); return -1;
    }
    if (listen(fd, 16) < 0) { perror("listen"); close(fd); return -1; }
    if (g_cfg.verbose) fprintf(stderr, "mailval: %s listening on %d%s\n",
                               spec->mod->name, spec->port, spec->implicit_tls ? " (implicit TLS)" : "");
    for (;;) {
        struct sockaddr_in peer;
        socklen_t plen = sizeof(peer);
        int cfd = accept(fd, (struct sockaddr*)&peer, &plen);
        if (cfd < 0) continue;
        session_arg* sa = mv_xcalloc(1, sizeof(*sa));
        sa->conn.fd = cfd;
        sa->conn.ssl = NULL;
        sa->conn.tls = false;
        sa->conn.ctx = spec->ctx;
        sa->mod = spec->mod;
        snprintf(sa->peer, sizeof(sa->peer), "%s:%d",
                 inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
        if (spec->implicit_tls && spec->ctx) {
            SSL* ssl = mv_tls_accept_fd(spec->ctx, cfd);
            if (!ssl) { close(cfd); free(sa); continue; }
            sa->conn.ssl = ssl;
            sa->conn.tls = true;
        }
        pthread_t tid;
        if (pthread_create(&tid, NULL, session_thread, sa) != 0) {
            mv_conn_close(&sa->conn);
            free(sa);
            continue;
        }
        pthread_detach(tid);
    }
}
