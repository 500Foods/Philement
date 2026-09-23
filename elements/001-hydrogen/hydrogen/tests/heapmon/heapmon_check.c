/*
 * Self-test for libheapmon.so. Churns the allocator from several threads,
 * leaks a known number of bytes from named functions, then checks the dump.
 *
 * Build without the preload linked in. Run as:
 *   HEAPMON_NAME=name LD_PRELOAD=libheapmon.so ./heapmon_check
 */

#define _GNU_SOURCE

#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define LEAK_N 40
#define LEAK_KEEP 30
#define LEAK_SIZE 64
#define GROW_N 4
#define GROW_SIZE 128
#define DUP_N 3

__attribute__((noinline)) void *heapmon_leak_block(void) {
    void *p = malloc(LEAK_SIZE);
    __asm__ volatile("" : : "r"(p));
    return p;
}

__attribute__((noinline)) void *heapmon_temp_block(void) {
    void *p = malloc(LEAK_SIZE);
    __asm__ volatile("" : : "r"(p));
    free(p);
    return NULL;
}

__attribute__((noinline)) void *heapmon_grow_block(void) {
    void *p = malloc(32);
    void *grown = realloc(p, GROW_SIZE);
    if (!grown) {
        free(p);
        return NULL;
    }
    __asm__ volatile("" : : "r"(grown));
    return grown;
}

__attribute__((noinline)) char *heapmon_dup_block(void) {
    char *p = strdup("heapmon-strdup-leak");
    __asm__ volatile("" : : "r"(p));
    return p;
}

static void *churn(void *arg) {
    int id = *(int *)arg;
    int i;

    for (i = 0; i < 2000; i++) {
        void *p = malloc((size_t)(16 + ((i + id) % 256)));
        void *grown;
        char *s = strdup("churn");
        if (s && s[0] != 'c') {
            free(s);
            free(p);
            continue;
        }
        grown = realloc(p, (size_t)(32 + (i % 128)));
        if (!grown) {
            free(s);
            free(p);
            continue;
        }
        free(s);
        free(grown);
    }
    return NULL;
}

static int read_dump(const char *name, char *buf, size_t cap) {
    int fd = -1;
    int attempt;
    size_t used = 0;
    size_t nlen = strlen(name);

    for (attempt = 0; attempt < 100 && fd < 0; attempt++) {
        struct sockaddr_un addr;
        socklen_t len;

        fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (fd < 0) {
            return -1;
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
        fprintf(stderr, "heapmon_check: connect failed: %s\n", strerror(errno));
        return -1;
    }
    if (write(fd, "dump\n", 5) != 5) {
        close(fd);
        return -1;
    }
    while (used + 1u < cap) {
        ssize_t r = read(fd, buf + used, cap - 1u - used);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            close(fd);
            return -1;
        }
        if (r == 0) {
            break;
        }
        used += (size_t)r;
    }
    buf[used] = '\0';
    close(fd);
    if (used + 1u >= cap) {
        fprintf(stderr, "heapmon_check: dump truncated\n");
        return -1;
    }
    return 0;
}

static long long sum_symbol(const char *dump, const char *name) {
    const char *p = dump;
    long long total = 0;

    while ((p = strstr(p, "SITE\t")) != NULL) {
        unsigned long pc = 0;
        unsigned long off = 0;
        long long live_bytes = 0;
        long long live_count = 0;
        unsigned long long alloc_count = 0;
        unsigned long long free_count = 0;
        char module[512];
        char symbol[512];
        int got;

        module[0] = '\0';
        symbol[0] = '\0';
        got = sscanf(p, "SITE\t%lx\t%lx\t%lld\t%lld\t%llu\t%llu\t%511s\t%511s",
                     &pc, &off, &live_bytes, &live_count, &alloc_count, &free_count,
                     module, symbol);
        if (got == 8 && strstr(symbol, name)) {
            total += live_bytes;
        }
        p += 5;
    }
    return total;
}

int main(void) {
    static char dump[1024u * 1024u];
    const char *name = getenv("HEAPMON_NAME");
    pthread_t threads[4];
    int ids[4];
    void *leaks[LEAK_N];
    int i;
    long long leak_bytes;
    long long grow_bytes;
    long long dup_bytes;
    long long temp_bytes;
    const char *dup_lit = "heapmon-strdup-leak";
    long long dup_expect;

    if (!name || !name[0]) {
        fprintf(stderr, "heapmon_check: HEAPMON_NAME is not set\n");
        return 1;
    }

    free(NULL);
    free(realloc(NULL, 16));

    for (i = 0; i < 4; i++) {
        ids[i] = i;
        if (pthread_create(&threads[i], NULL, churn, &ids[i]) != 0) {
            fprintf(stderr, "heapmon_check: pthread_create failed\n");
            return 1;
        }
    }
    for (i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    for (i = 0; i < LEAK_N; i++) {
        leaks[i] = heapmon_leak_block();
    }
    for (i = 0; i < LEAK_N - LEAK_KEEP; i++) {
        free(leaks[i]);
    }
    for (i = 0; i < 20; i++) {
        (void)heapmon_temp_block();
    }
    for (i = 0; i < GROW_N; i++) {
        void *p = heapmon_grow_block();
        if (!p) {
            fprintf(stderr, "heapmon_check: grow failed\n");
            return 1;
        }
    }
    for (i = 0; i < DUP_N; i++) {
        char *copied = heapmon_dup_block();
        if (!copied) {
            fprintf(stderr, "heapmon_check: strdup failed\n");
            return 1;
        }
    }

    if (read_dump(name, dump, sizeof dump) != 0) {
        return 1;
    }
    if (!strstr(dump, "\nEND\n") && strncmp(dump, "END\n", 4) != 0 && !strstr(dump, "END\n")) {
        fprintf(stderr, "heapmon_check: dump missing END\n%s\n", dump);
        return 1;
    }

    leak_bytes = sum_symbol(dump, "heapmon_leak_block");
    grow_bytes = sum_symbol(dump, "heapmon_grow_block");
    dup_bytes = sum_symbol(dump, "heapmon_dup_block");
    temp_bytes = sum_symbol(dump, "heapmon_temp_block");
    dup_expect = (long long)DUP_N * (long long)(strlen(dup_lit) + 1u);

    if (leak_bytes != (long long)LEAK_KEEP * LEAK_SIZE) {
        fprintf(stderr, "heapmon_check: leak_block live %lld want %d\n",
                leak_bytes, LEAK_KEEP * LEAK_SIZE);
        return 1;
    }
    if (grow_bytes != (long long)GROW_N * GROW_SIZE) {
        fprintf(stderr, "heapmon_check: grow_block live %lld want %d\n",
                grow_bytes, GROW_N * GROW_SIZE);
        return 1;
    }
    if (dup_bytes != dup_expect) {
        fprintf(stderr, "heapmon_check: dup_block live %lld want %lld\n",
                dup_bytes, dup_expect);
        return 1;
    }
    if (temp_bytes != 0) {
        fprintf(stderr, "heapmon_check: temp_block live %lld want 0\n", temp_bytes);
        return 1;
    }
    /* A double-tracked strdup leaves a ghost site inside libc after free. */
    if (sum_symbol(dump, "__strdup") != 0) {
        fprintf(stderr, "heapmon_check: ghost __strdup bytes %lld\n%s\n",
                sum_symbol(dump, "__strdup"), dump);
        return 1;
    }
    if (!strstr(dump, "MALLINFO\t")) {
        fprintf(stderr, "heapmon_check: mallinfo missing\n");
        return 1;
    }
    printf("heapmon self-test ok\n");
    return 0;
}
