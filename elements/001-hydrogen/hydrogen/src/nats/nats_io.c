/*
 * NATS socket seam. The real path is a plaintext TCP client.
 * Tests replace the table and never open a socket.
 */

#include <src/hydrogen.h>

#include <src/nats/nats_internal.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>

static NatsIo nats_io_current;
static int nats_fd = -1;

int nats_io_connect_fd(int fd, const struct sockaddr *addr, socklen_t len,
                       int timeout_seconds) {
    int flags;
    int rc;
    int so_error = 0;
    socklen_t so_len = sizeof(so_error);
    struct pollfd ready;

    if (fd < 0 || !addr || timeout_seconds < 1) {
        return -1;
    }
    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return -1;
    }
    rc = connect(fd, addr, len);
    if (rc == 0) {
        return 0;
    }
    if (errno != EINPROGRESS) {
        return -1;
    }
    memset(&ready, 0, sizeof(ready));
    ready.fd = fd;
    ready.events = POLLOUT;
    rc = poll(&ready, 1, timeout_seconds * 1000);
    if (rc <= 0) {
        return -1;
    }
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &so_len) < 0 || so_error != 0) {
        return -1;
    }
    return 0;
}

int nats_io_tcp_connect(void *ctx, const char *host, int port, int timeout_seconds) {
    struct addrinfo hints;
    struct addrinfo *listed = NULL;
    struct addrinfo *item;
    char port_text[8];
    int lookup;

    (void)ctx;
    if (!host || port < 1 || port > 65535 || port == 6222) {
        return -1;
    }
    if (timeout_seconds < 1) {
        timeout_seconds = 10;
    }
    if (timeout_seconds > 120) {
        timeout_seconds = 120;
    }
    if (nats_fd >= 0) {
        close(nats_fd);
        nats_fd = -1;
    }
    if (snprintf(port_text, sizeof(port_text), "%d", port) < 0) {
        return -1;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    lookup = getaddrinfo(host, port_text, &hints, &listed);
    if (lookup != 0) {
        return -1;
    }
    for (item = listed; item != NULL; item = item->ai_next) {
        int fd;

        fd = socket(item->ai_family, item->ai_socktype, item->ai_protocol);
        if (fd < 0) {
            continue;
        }
        if (nats_io_connect_fd(fd, item->ai_addr, item->ai_addrlen, timeout_seconds) == 0) {
            nats_fd = fd;
            freeaddrinfo(listed);
            return 0;
        }
        close(fd);
    }
    freeaddrinfo(listed);
    return -1;
}

int nats_io_tcp_read(void *ctx, void *buf, size_t len) {
    struct pollfd ready;
    int rc;
    ssize_t got;

    (void)ctx;
    if (nats_fd < 0 || !buf || len == 0) {
        return -1;
    }
    memset(&ready, 0, sizeof(ready));
    ready.fd = nats_fd;
    ready.events = POLLIN;
    rc = poll(&ready, 1, 200);
    if (rc == 0 || (rc < 0 && errno == EINTR)) {
        return -2;
    }
    if (rc < 0) {
        return -1;
    }
    got = recv(nats_fd, buf, len, 0);
    if (got == 0) {
        return 0;
    }
    if (got < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return -2;
        }
        return -1;
    }
    return (int)got;
}

int nats_io_tcp_write(void *ctx, const void *buf, size_t len) {
    const unsigned char *cursor = buf;
    size_t left = len;

    (void)ctx;
    if (nats_fd < 0 || (!buf && len > 0)) {
        return -1;
    }
    while (left > 0) {
        struct pollfd ready;
        ssize_t sent;
        int rc;

        memset(&ready, 0, sizeof(ready));
        ready.fd = nats_fd;
        ready.events = POLLOUT;
        rc = poll(&ready, 1, 10000);
        if (rc == 0) {
            return -1;
        }
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        sent = send(nats_fd, cursor, left, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (sent == 0) {
            return -1;
        }
        cursor += sent;
        left -= (size_t)sent;
    }
    return 0;
}

void nats_io_tcp_close(void *ctx) {
    (void)ctx;
    if (nats_fd >= 0) {
        close(nats_fd);
        nats_fd = -1;
    }
}

int nats_io_mock_connect(void *ctx, const char *host, int port, int timeout_seconds) {
    (void)ctx;
    (void)host;
    (void)port;
    (void)timeout_seconds;
    return -1;
}

int nats_io_mock_read(void *ctx, void *buf, size_t len) {
    (void)ctx;
    (void)buf;
    (void)len;
    return -1;
}

int nats_io_mock_write(void *ctx, const void *buf, size_t len) {
    (void)ctx;
    (void)buf;
    (void)len;
    return -1;
}

void nats_io_mock_close(void *ctx) {
    (void)ctx;
}

void nats_io_install(const NatsIo *io) {
    nats_io_close();
    if (!io || !io->connect_fn || !io->read_fn || !io->write_fn || !io->close_fn) {
        nats_io_current.connect_fn = nats_io_tcp_connect;
        nats_io_current.read_fn = nats_io_tcp_read;
        nats_io_current.write_fn = nats_io_tcp_write;
        nats_io_current.close_fn = nats_io_tcp_close;
        nats_io_current.ctx = NULL;
        return;
    }
    nats_io_current = *io;
}

void nats_io_use_config(void) {
    NatsIo mock;

    /* Take the mock addresses on every call so the dead-code gate keeps them. */
    mock.connect_fn = nats_io_mock_connect;
    mock.read_fn = nats_io_mock_read;
    mock.write_fn = nats_io_mock_write;
    mock.close_fn = nats_io_mock_close;
    mock.ctx = NULL;
    if (app_config && app_config->nats.Test.MockConnection) {
        nats_io_install(&mock);
        return;
    }
    nats_io_install(NULL);
}

int nats_io_connect(const char *host, int port, int timeout_seconds) {
    if (!nats_io_current.connect_fn) {
        nats_io_install(NULL);
    }
    return nats_io_current.connect_fn(nats_io_current.ctx, host, port, timeout_seconds);
}

int nats_io_read(void *buf, size_t len) {
    if (!nats_io_current.read_fn) {
        return -1;
    }
    return nats_io_current.read_fn(nats_io_current.ctx, buf, len);
}

int nats_io_write(const void *buf, size_t len) {
    if (!nats_io_current.write_fn) {
        return -1;
    }
    return nats_io_current.write_fn(nats_io_current.ctx, buf, len);
}

void nats_io_close(void) {
    if (nats_io_current.close_fn) {
        nats_io_current.close_fn(nats_io_current.ctx);
    }
}
