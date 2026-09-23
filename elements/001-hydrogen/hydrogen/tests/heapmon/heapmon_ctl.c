/*
 * Read one heapmon dump from the abstract socket named by argv[1]
 * and write it to stdout. Retries briefly so a just-started server
 * has time to bind.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(int argc, char **argv) {
    const char *name;
    int fd = -1;
    int attempt;
    size_t nlen;
    char buf[8192];

    if (argc != 2 || !argv[1] || !argv[1][0]) {
        fprintf(stderr, "usage: heapmon_ctl NAME\n");
        return 2;
    }
    name = argv[1];
    nlen = strlen(name);
    for (attempt = 0; attempt < 100 && fd < 0; attempt++) {
        struct sockaddr_un addr;
        socklen_t len;

        fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (fd < 0) {
            return 1;
        }
        memset(&addr, 0, sizeof addr);
        addr.sun_family = AF_UNIX;
        if (nlen > sizeof addr.sun_path - 2u) {
            nlen = sizeof addr.sun_path - 2u;
        }
        addr.sun_path[0] = '\0';
        memcpy(addr.sun_path + 1, name, nlen);
        len = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + 1u + nlen);
        if (connect(fd, (struct sockaddr *)&addr, len) != 0) {
            close(fd);
            fd = -1;
            usleep(20000);
        }
    }
    if (fd < 0) {
        fprintf(stderr, "heapmon_ctl: connect @%s failed: %s\n", name, strerror(errno));
        return 1;
    }
    if (write(fd, "dump\n", 5) != 5) {
        close(fd);
        return 1;
    }
    for (;;) {
        ssize_t r = read(fd, buf, sizeof buf);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            close(fd);
            return 1;
        }
        if (r == 0) {
            break;
        }
        if (fwrite(buf, 1, (size_t)r, stdout) != (size_t)r) {
            close(fd);
            return 1;
        }
    }
    close(fd);
    return 0;
}
