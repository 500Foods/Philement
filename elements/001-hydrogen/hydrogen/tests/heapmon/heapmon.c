/*
 * LD_PRELOAD heap monitor for test 44.
 *
 * Interposes the public allocator entry points and keeps a fixed table of
 * still-live blocks keyed by the caller instruction. A dump (abstract socket
 * named by HEAPMON_NAME) reports glibc mallinfo2 plus one line per caller:
 * bytes and blocks allocated and not yet freed.
 *
 * Tables are prefaulted at startup so their own pages sit in the warmup RSS,
 * not in the steady-state slope. Allocations that never reach these hooks
 * (libc-internal malloc, or a library with a private arena) show up as a gap
 * between mallinfo2 in-use and the tracked total.
 */

#define _GNU_SOURCE

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <malloc.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <sys/socket.h>
#include <sys/un.h>

#define PTR_SLOTS (1u << 19) /* 524288 */
#define SITE_SLOTS (1u << 16) /* 65536 */
#define PTR_EMPTY ((uintptr_t)0)
#define PTR_TOMB ((uintptr_t)1)
#define PTR_LOCK ((uintptr_t)2)

typedef void *(*malloc_fn)(size_t);
typedef void (*free_fn)(void *);
typedef void *(*realloc_fn)(void *, size_t);
typedef void *(*calloc_fn)(size_t, size_t);
typedef void *(*aligned_alloc_fn)(size_t, size_t);
typedef int (*posix_memalign_fn)(void **, size_t, size_t);
typedef void *(*memalign_fn)(size_t, size_t);
typedef char *(*strdup_fn)(const char *);
typedef char *(*strndup_fn)(const char *, size_t);
typedef struct {
    _Atomic uintptr_t ptr;
    _Atomic uintptr_t pc;
    _Atomic uint64_t size;
} ptr_slot;

typedef struct {
    _Atomic uintptr_t pc;
    _Atomic int64_t live_bytes;
    _Atomic int64_t live_count;
    _Atomic uint64_t alloc_count;
    _Atomic uint64_t free_count;
} site_slot;

static ptr_slot ptrs[PTR_SLOTS];
static site_slot sites[SITE_SLOTS];

static char boot[2u * 1024u * 1024u];
static _Atomic size_t boot_off;
static _Atomic int init_state;
static __thread int in_hook;

static malloc_fn real_malloc;
static free_fn real_free;
static realloc_fn real_realloc;
static calloc_fn real_calloc;
static aligned_alloc_fn real_aligned_alloc;
static posix_memalign_fn real_posix_memalign;
static memalign_fn real_memalign;
static strdup_fn real_strdup;
static strndup_fn real_strndup;
static _Atomic uint64_t stat_untracked;
static _Atomic uint64_t stat_site_overflow;

static char sock_name[100];

static uint32_t mix64(uintptr_t x) {
    uint64_t z = (uint64_t)x;
    z ^= z >> 30;
    z *= 0xbf58476d1ce4e5b9ULL;
    z ^= z >> 27;
    z *= 0x94d049bb133111ebULL;
    z ^= z >> 31;
    return (uint32_t)z;
}

static int is_boot(const void *p) {
    const char *c = (const char *)p;
    return c >= boot && c < boot + sizeof boot;
}

static void *boot_alloc(size_t n) {
    size_t off;

    if (n == 0) {
        n = 1;
    }
    n = (n + 15u) & ~(size_t)15u;
    off = atomic_fetch_add(&boot_off, n);
    if (off + n > sizeof boot) {
        return NULL;
    }
    return boot + off;
}

static void *boot_aligned(size_t align, size_t size) {
    uintptr_t raw;
    uintptr_t aligned;
    size_t pad;
    const char *block;

    if (align < 16u) {
        align = 16u;
    }
    if ((align & (align - 1u)) != 0) {
        return NULL;
    }
    pad = align - 1u + sizeof(void *);
    block = boot_alloc(size + pad);
    if (!block) {
        return NULL;
    }
    raw = (uintptr_t)block + sizeof(void *);
    aligned = (raw + (align - 1u)) & ~(uintptr_t)(align - 1u);
    return (void *)aligned;
}

static void resolve_syms(void) {
    real_malloc = (malloc_fn)dlsym(RTLD_NEXT, "malloc");
    real_free = (free_fn)dlsym(RTLD_NEXT, "free");
    real_realloc = (realloc_fn)dlsym(RTLD_NEXT, "realloc");
    real_calloc = (calloc_fn)dlsym(RTLD_NEXT, "calloc");
    real_aligned_alloc = (aligned_alloc_fn)dlsym(RTLD_NEXT, "aligned_alloc");
    real_posix_memalign = (posix_memalign_fn)dlsym(RTLD_NEXT, "posix_memalign");
    real_memalign = (memalign_fn)dlsym(RTLD_NEXT, "memalign");
    real_strdup = (strdup_fn)dlsym(RTLD_NEXT, "strdup");
    real_strndup = (strndup_fn)dlsym(RTLD_NEXT, "strndup");
    if (!real_malloc || !real_free || !real_realloc || !real_calloc) {
        fprintf(stderr, "heapmon: dlsym of allocator symbols failed\n");
        abort();
    }
}

static void ensure_real(void) {
    int expected = 0;

    if (atomic_load(&init_state) == 2) {
        return;
    }
    if (!atomic_compare_exchange_strong(&init_state, &expected, 1)) {
        while (atomic_load(&init_state) != 2) {
            sched_yield();
        }
        return;
    }
    in_hook++;
    resolve_syms();
    in_hook--;
    atomic_store(&init_state, 2);
}

/* True once the real libc symbols are resolved. in_hook is separate: those
 * calls still use the real allocator, they just are not recorded. */
static int ready(void) {
    return atomic_load(&init_state) == 2;
}

static int find_site(uintptr_t pc, uint32_t *out) {
    uint32_t start = mix64(pc) & (SITE_SLOTS - 1u);
    uint32_t n;

    for (n = 0; n < SITE_SLOTS; n++) {
        uint32_t i = (start + n) & (SITE_SLOTS - 1u);
        uintptr_t cur = atomic_load(&sites[i].pc);

        if (cur == pc) {
            *out = i;
            return 1;
        }
        if (cur == 0) {
            uintptr_t expected = 0;
            if (atomic_compare_exchange_strong(&sites[i].pc, &expected, pc)) {
                *out = i;
                return 1;
            }
            if (expected == pc) {
                *out = i;
                return 1;
            }
        }
    }
    return 0;
}

static void site_apply(uint32_t slot, int64_t bytes, int64_t live, uint64_t allocs, uint64_t frees) {
    atomic_fetch_add(&sites[slot].live_bytes, bytes);
    atomic_fetch_add(&sites[slot].live_count, live);
    if (allocs > 0) {
        atomic_fetch_add(&sites[slot].alloc_count, allocs);
    }
    if (frees > 0) {
        atomic_fetch_add(&sites[slot].free_count, frees);
    }
}

static int site_apply_pc(uintptr_t pc, int64_t bytes, int64_t live, uint64_t allocs, uint64_t frees) {
    uint32_t slot = 0;

    if (!find_site(pc, &slot)) {
        atomic_fetch_add(&stat_site_overflow, 1);
        return 0;
    }
    site_apply(slot, bytes, live, allocs, frees);
    return 1;
}

static int ptr_insert(uintptr_t key, uint64_t size, uintptr_t pc) {
    uint32_t start = mix64(key >> 4) & (PTR_SLOTS - 1u);
    uint32_t n = 0;
    uint32_t guard = 0;

    while (n < PTR_SLOTS && guard < (PTR_SLOTS * 2u)) {
        uint32_t i = (start + n) & (PTR_SLOTS - 1u);
        uintptr_t cur = atomic_load(&ptrs[i].ptr);

        guard++;
        if (cur == key) {
            atomic_store(&ptrs[i].size, size);
            atomic_store(&ptrs[i].pc, pc);
            return 1;
        }
        if (cur != PTR_EMPTY && cur != PTR_TOMB) {
            n++;
            continue;
        }
        {
            uintptr_t expected = cur;
            if (!atomic_compare_exchange_strong(&ptrs[i].ptr, &expected, PTR_LOCK)) {
                continue;
            }
        }
        atomic_store(&ptrs[i].size, size);
        atomic_store(&ptrs[i].pc, pc);
        atomic_store(&ptrs[i].ptr, key);
        return 1;
    }
    return 0;
}

static int ptr_take(uintptr_t key, uintptr_t *pc, uint64_t *size) {
    uint32_t start = mix64(key >> 4) & (PTR_SLOTS - 1u);
    uint32_t n;

    for (n = 0; n < PTR_SLOTS; n++) {
        uint32_t i = (start + n) & (PTR_SLOTS - 1u);
        uintptr_t cur = atomic_load(&ptrs[i].ptr);

        if (cur == PTR_EMPTY) {
            return 0;
        }
        if (cur != key) {
            continue;
        }
        *size = atomic_load(&ptrs[i].size);
        *pc = atomic_load(&ptrs[i].pc);
        if (atomic_compare_exchange_strong(&ptrs[i].ptr, &cur, PTR_TOMB)) {
            return 1;
        }
    }
    return 0;
}

static void track_block(const void *p, size_t size, uintptr_t pc) {
    uint32_t slot = 0;

    if (!p || is_boot(p) || pc == 0) {
        return;
    }
    if (!find_site(pc, &slot)) {
        atomic_fetch_add(&stat_untracked, 1);
        return;
    }
    /* Publish the byte count before the pointer is findable by free.
     * The caller has not returned this pointer yet, so no other thread
     * can free it between these two steps. */
    site_apply(slot, (int64_t)size, 1, 1, 0);
    if (!ptr_insert((uintptr_t)p, (uint64_t)size, pc)) {
        site_apply(slot, -(int64_t)size, -1, 0, 0);
        atomic_fetch_sub(&sites[slot].alloc_count, 1);
        atomic_fetch_add(&stat_untracked, 1);
    }
}

static void untrack_block(const void *p) {
    uintptr_t pc = 0;
    uint64_t size = 0;

    if (!p || is_boot(p)) {
        return;
    }
    if (!ptr_take((uintptr_t)p, &pc, &size)) {
        return;
    }
    if (!site_apply_pc(pc, -(int64_t)size, -1, 0, 1)) {
        return;
    }
}

static void send_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t w = send(fd, buf, n, MSG_NOSIGNAL);
        if (w < 0) {
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        if (w == 0) {
            return;
        }
        buf += w;
        n -= (size_t)w;
    }
}

static void sanitize(char *s) {
    for (; *s; s++) {
        if (*s == '\t' || *s == '\n' || *s == '\r') {
            *s = ' ';
        }
    }
}

static void dump_to_fd(int fd) {
    char line[4600];
    struct mallinfo2 mi;
    int64_t tracked_live = 0;
    int64_t tracked_count = 0;
    uint32_t i;
    int n;

    in_hook++;
    mi = mallinfo2();
    n = snprintf(line, sizeof line, "HEAPMON\t1\n");
    if (n > 0) {
        send_all(fd, line, (size_t)n);
    }
    n = snprintf(line, sizeof line,
                 "MALLINFO\tuordblks\t%zu\tfordblks\t%zu\tarena\t%zu\thblkhd\t%zu\tkeepcost\t%zu\n",
                 mi.uordblks, mi.fordblks, mi.arena, mi.hblkhd, mi.keepcost);
    if (n > 0) {
        send_all(fd, line, (size_t)n);
    }

    for (i = 0; i < SITE_SLOTS; i++) {
        uintptr_t pc = atomic_load(&sites[i].pc);
        int64_t live_bytes;
        int64_t live_count;
        uint64_t alloc_count;
        uint64_t free_count;
        Dl_info info;
        const char *module = "-";
        char symbol[256];
        uintptr_t off = 0;

        if (pc == 0) {
            continue;
        }
        live_bytes = atomic_load(&sites[i].live_bytes);
        live_count = atomic_load(&sites[i].live_count);
        alloc_count = atomic_load(&sites[i].alloc_count);
        free_count = atomic_load(&sites[i].free_count);
        if (live_bytes == 0 && live_count == 0) {
            continue;
        }
        tracked_live += live_bytes;
        tracked_count += live_count;
        symbol[0] = '-';
        symbol[1] = '\0';
        memset(&info, 0, sizeof info);
        if (dladdr((void *)pc, &info)) {
            if (info.dli_fname && info.dli_fname[0]) {
                module = info.dli_fname;
            }
            if (info.dli_fbase) {
                off = pc - (uintptr_t)info.dli_fbase;
            }
            if (info.dli_sname && info.dli_sname[0]) {
                unsigned long sym_off = 0;
                if (info.dli_saddr) {
                    sym_off = (unsigned long)(pc - (uintptr_t)info.dli_saddr);
                }
                snprintf(symbol, sizeof symbol, "%s+0x%lx", info.dli_sname, sym_off);
            }
        }
        sanitize(symbol);
        n = snprintf(line, sizeof line,
                     "SITE\t%lx\t%lx\t%lld\t%lld\t%llu\t%llu\t%s\t%s\n",
                     (unsigned long)pc, (unsigned long)off,
                     (long long)live_bytes, (long long)live_count,
                     (unsigned long long)alloc_count, (unsigned long long)free_count,
                     module, symbol);
        if (n > 0) {
            send_all(fd, line, (size_t)n);
        }
    }

    n = snprintf(line, sizeof line,
                 "TOTALS\ttracked_live\t%lld\ttracked_count\t%lld\tuntracked\t%llu\tsite_overflow\t%llu\ttables_bytes\t%zu\n",
                 (long long)tracked_live, (long long)tracked_count,
                 (unsigned long long)atomic_load(&stat_untracked),
                 (unsigned long long)atomic_load(&stat_site_overflow),
                 sizeof ptrs + sizeof sites);
    if (n > 0) {
        send_all(fd, line, (size_t)n);
    }
    n = snprintf(line, sizeof line, "END\n");
    if (n > 0) {
        send_all(fd, line, (size_t)n);
    }
    in_hook--;
}

static void *server_main(void *arg) {
    struct sockaddr_un addr;
    size_t nlen;
    socklen_t len;
    int fd;

    (void)arg;
    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        return NULL;
    }
    memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    nlen = strlen(sock_name);
    if (nlen > sizeof addr.sun_path - 2u) {
        nlen = sizeof addr.sun_path - 2u;
    }
    addr.sun_path[0] = '\0';
    memcpy(addr.sun_path + 1, sock_name, nlen);
    len = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + 1u + nlen);
    if (bind(fd, (struct sockaddr *)&addr, len) != 0) {
        fprintf(stderr, "heapmon: bind @%s failed: %s\n", sock_name, strerror(errno));
        close(fd);
        return NULL;
    }
    if (listen(fd, 4) != 0) {
        close(fd);
        return NULL;
    }
    for (;;) {
        int client = accept4(fd, NULL, NULL, SOCK_CLOEXEC);
        char sink[64];
        if (client < 0) {
            if (errno == EINTR) {
                continue;
            }
            continue;
        }
        (void)read(client, sink, sizeof sink);
        dump_to_fd(client);
        close(client);
    }
}

__attribute__((constructor))
static void heapmon_init(void) {
    const char *name;
    pthread_t thread;

    ensure_real();
    /* Dirty every table page before the workload so later RSS samples do not
     * charge first-touch of the monitor itself to the steady-state slope. */
    memset(ptrs, 0, sizeof ptrs);
    memset(sites, 0, sizeof sites);
    name = getenv("HEAPMON_NAME");
    if (!name || !name[0]) {
        return;
    }
    snprintf(sock_name, sizeof sock_name, "%s", name);
    if (pthread_create(&thread, NULL, server_main, NULL) == 0) {
        pthread_detach(thread);
    }
}

void *malloc(size_t size) {
    void *p;
    size_t req = size;

    if (!ready()) {
        if (atomic_load(&init_state) == 0) {
            ensure_real();
        }
        if (!ready()) {
            return boot_alloc(size == 0 ? 1 : size);
        }
    }
    p = real_malloc(size);
    if (!p || in_hook) {
        return p;
    }
    in_hook++;
    track_block(p, req, (uintptr_t)__builtin_return_address(0));
    in_hook--;
    return p;
}

void free(void *p) {
    if (!p || is_boot(p)) {
        return;
    }
    if (atomic_load(&init_state) != 2) {
        return;
    }
    if (!in_hook) {
        in_hook++;
        untrack_block(p);
        in_hook--;
    }
    real_free(p);
}

void *calloc(size_t nmemb, size_t size) {
    void *p;
    size_t bytes;

    if (nmemb != 0 && size > SIZE_MAX / nmemb) {
        errno = ENOMEM;
        return NULL;
    }
    bytes = nmemb * size;
    if (!ready()) {
        if (atomic_load(&init_state) == 0) {
            ensure_real();
        }
        if (!ready()) {
            p = boot_alloc(bytes == 0 ? 1 : bytes);
            if (p) {
                memset(p, 0, bytes);
            }
            return p;
        }
    }
    p = real_calloc(nmemb, size);
    if (!p || in_hook) {
        return p;
    }
    in_hook++;
    track_block(p, bytes, (uintptr_t)__builtin_return_address(0));
    in_hook--;
    return p;
}

static void *realloc_at(void *p, size_t size, uintptr_t caller) {
    uintptr_t old_pc = 0;
    uint64_t old_sz = 0;
    int had = 0;
    void *np;

    if (!p) {
        if (!ready() || in_hook) {
            return malloc(size);
        }
        np = real_malloc(size);
        if (!np) {
            return NULL;
        }
        in_hook++;
        track_block(np, size, caller);
        in_hook--;
        return np;
    }
    if (is_boot(p) || !ready()) {
        /* Bootstrap blocks have no recorded size and must not be passed to
         * real_free. Drop them; only the init path hits this. */
        return malloc(size);
    }
    if (in_hook) {
        return real_realloc(p, size);
    }

    in_hook++;
    had = ptr_take((uintptr_t)p, &old_pc, &old_sz);
    in_hook--;

    np = real_realloc(p, size);

    in_hook++;
    if (!np && size > 0) {
        /* glibc left the old block in place. Put the same accounting back. */
        if (had && !ptr_insert((uintptr_t)p, old_sz, old_pc)) {
            site_apply_pc(old_pc, -(int64_t)old_sz, -1, 0, 1);
            atomic_fetch_add(&stat_untracked, 1);
        }
    } else {
        if (had) {
            site_apply_pc(old_pc, -(int64_t)old_sz, -1, 0, 1);
        }
        if (np) {
            track_block(np, size, had ? old_pc : caller);
        }
    }
    in_hook--;
    return np;
}

void *realloc(void *p, size_t size) {
    uintptr_t caller = (uintptr_t)__builtin_return_address(0);
    return realloc_at(p, size, caller);
}

void *reallocarray(void *p, size_t nmemb, size_t size) {
    uintptr_t caller = (uintptr_t)__builtin_return_address(0);

    if (nmemb != 0 && size > SIZE_MAX / nmemb) {
        errno = ENOMEM;
        return NULL;
    }
    return realloc_at(p, nmemb * size, caller);
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
    int rc;

    if (!ready()) {
        void *p;
        if (atomic_load(&init_state) == 0) {
            ensure_real();
        }
        if (!ready()) {
            p = boot_aligned(alignment, size);
            if (!p) {
                return ENOMEM;
            }
            *memptr = p;
            return 0;
        }
    }
    if (!real_posix_memalign) {
        return ENOMEM;
    }
    rc = real_posix_memalign(memptr, alignment, size);
    if (rc == 0 && *memptr && !in_hook) {
        in_hook++;
        track_block(*memptr, size, (uintptr_t)__builtin_return_address(0));
        in_hook--;
    }
    return rc;
}

void *aligned_alloc(size_t alignment, size_t size) {
    void *p;

    if (!ready()) {
        if (atomic_load(&init_state) == 0) {
            ensure_real();
        }
        if (!ready()) {
            return boot_aligned(alignment, size);
        }
    }
    if (!real_aligned_alloc) {
        return NULL;
    }
    p = real_aligned_alloc(alignment, size);
    if (!p || in_hook) {
        return p;
    }
    in_hook++;
    track_block(p, size, (uintptr_t)__builtin_return_address(0));
    in_hook--;
    return p;
}

void *memalign(size_t alignment, size_t size) {
    void *p;

    if (!ready()) {
        if (atomic_load(&init_state) == 0) {
            ensure_real();
        }
        if (!ready()) {
            return boot_aligned(alignment, size);
        }
    }
    if (!real_memalign) {
        void *out = NULL;
        int rc;

        if (!real_posix_memalign) {
            return NULL;
        }
        rc = real_posix_memalign(&out, alignment, size);
        if (rc != 0 || !out) {
            return NULL;
        }
        if (!in_hook) {
            in_hook++;
            track_block(out, size, (uintptr_t)__builtin_return_address(0));
            in_hook--;
        }
        return out;
    }
    p = real_memalign(alignment, size);
    if (!p || in_hook) {
        return p;
    }
    in_hook++;
    track_block(p, size, (uintptr_t)__builtin_return_address(0));
    in_hook--;
    return p;
}

/* libc strdup calls public malloc. Hold in_hook across that call so the
 * block is recorded once, against the real caller, not against libc. */
static char *strdup_at(const char *s, uintptr_t caller) {
    size_t n = strlen(s) + 1u;
    char *p;

    if (!ready()) {
        if (atomic_load(&init_state) == 0) {
            ensure_real();
        }
        if (!ready()) {
            p = boot_alloc(n);
            if (p) {
                memcpy(p, s, n);
            }
            return p;
        }
    }
    if (in_hook) {
        p = real_strdup ? real_strdup(s) : real_malloc(n);
        if (p && !real_strdup) {
            memcpy(p, s, n);
        }
        return p;
    }
    in_hook++;
    if (real_strdup) {
        p = real_strdup(s);
    } else {
        p = real_malloc(n);
        if (p) {
            memcpy(p, s, n);
        }
    }
    in_hook--;
    if (!p) {
        return NULL;
    }
    in_hook++;
    track_block(p, n, caller);
    in_hook--;
    return p;
}

static char *strndup_at(const char *s, size_t nlen, uintptr_t caller) {
    const char *end = memchr(s, '\0', nlen);
    size_t n = end ? (size_t)(end - s) : nlen;
    char *p;

    if (!ready()) {
        if (atomic_load(&init_state) == 0) {
            ensure_real();
        }
        if (!ready()) {
            p = boot_alloc(n + 1u);
            if (p) {
                memcpy(p, s, n);
                p[n] = '\0';
            }
            return p;
        }
    }
    if (in_hook) {
        p = real_strndup ? real_strndup(s, nlen) : real_malloc(n + 1u);
        if (p && !real_strndup) {
            memcpy(p, s, n);
            p[n] = '\0';
        }
        return p;
    }
    in_hook++;
    if (real_strndup) {
        p = real_strndup(s, nlen);
    } else {
        p = real_malloc(n + 1u);
        if (p) {
            memcpy(p, s, n);
            p[n] = '\0';
        }
    }
    in_hook--;
    if (!p) {
        return NULL;
    }
    in_hook++;
    track_block(p, n + 1u, caller);
    in_hook--;
    return p;
}

char *strdup(const char *s) {
    uintptr_t caller = (uintptr_t)__builtin_return_address(0);
    return strdup_at(s, caller);
}

char *strndup(const char *s, size_t nlen) {
    uintptr_t caller = (uintptr_t)__builtin_return_address(0);
    return strndup_at(s, nlen, caller);
}
