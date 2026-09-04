/*
 * probe_mysql_bind.c — Phase 0 ABI measurement for PERSIST_PLAN
 *
 * Prints sizeof(MYSQL_BIND) + offsetof for the fields that matter to
 * mysql_stmt_bind_param from the *real* client header, then does the
 * same for a local typedef that mirrors the hand-rolled struct in
 * src/database/mysql/query.c. Also dlopen()s each candidate SONAME,
 * dlsym()s mysql_stmt_bind_param, and prints the resolved .so path
 * (via dladdr).
 *
 * Build (from elements/001-hydrogen/hydrogen):
 *   cc -I/usr/include/mysql -o /tmp/probe_mysql_bind \
 *       extras/probe_mysql_bind.c -ldl
 *
 * Then run /tmp/probe_mysql_bind. Exits 0 if sizeof and key offsets
 * match; exits 1 otherwise. Output is meant to be pasted verbatim
 * into the PERSIST_PLAN Working Log.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <dlfcn.h>

/* Real client header. On this distro mysql.h pulls in mariadb_stmt.h. */
#include <mysql.h>

/* ---- Hand-rolled mirror of src/database/mysql/query.c::MYSQL_BIND ----
 * Field order, names, and types copied verbatim from query.c lines 76-99
 * so that offsetof() reflects the layout Hydrogen actually uses.  If the
 * production struct changes, update this mirror.
 */
typedef struct MYSQL_BIND_HANDROLLED {
    unsigned long* length;
    char*          is_null;
    void*          buffer;
    char*          error;
    union {
        unsigned char* row_ptr;
        char*          indicator;
    } u;
    void (*store_param_func)(void*, struct MYSQL_BIND_HANDROLLED*);
    void (*fetch_result)(struct MYSQL_BIND_HANDROLLED*, void*, unsigned char**);
    void (*skip_result)(struct MYSQL_BIND_HANDROLLED*, void*, unsigned char**);
    unsigned long  buffer_length;
    unsigned long  offset;
    unsigned long  length_value;
    unsigned int   flags;
    unsigned int   pack_length;
    unsigned int   buffer_type;
    char           error_value;
    char           is_unsigned;
    char           long_data_used;
    char           is_null_value;
    void*          extension;
} MYSQL_BIND_HANDROLLED;

static const char* SONAMES[] = {
    "libmysqlclient.so.21",
    "libmysqlclient.so.18",
    "libmysqlclient.so.20",
    "libmysqlclient.so",
    "libmariadb.so.3",
    "libmariadb.so",
    NULL
};

static int fail = 0;

static void report(const char* tag, size_t real_size, size_t hand_size) {
    printf("%-12s sizeof = %3zu  (hand-rolled = %3zu)  %s\n",
           tag, real_size, hand_size,
           real_size == hand_size ? "OK" : "MISMATCH");
    if (real_size != hand_size) fail = 1;
}

static void offset(const char* field, size_t real_off, size_t hand_off) {
    printf("    %-16s offset real=%3zu  hand=%3zu  %s\n",
           field, real_off, hand_off,
           real_off == hand_off ? "OK" : "MISMATCH");
    if (real_off != hand_off) fail = 1;
}

int main(void) {
    printf("=== Phase 0 ABI probe (PERSIST_PLAN) ===\n");
    printf("sizeof(void*)         = %zu\n", sizeof(void*));
    printf("sizeof(unsigned long) = %zu\n", sizeof(unsigned long));
    printf("sizeof(unsigned int)  = %zu\n", sizeof(unsigned int));
    printf("\n");

    printf("MYSQL_BIND (real header)        sizeof = %zu\n",
           sizeof(MYSQL_BIND));
    printf("MYSQL_BIND_HANDROLLED (mirror)  sizeof = %zu\n",
           sizeof(MYSQL_BIND_HANDROLLED));
    report("MYSQL_BIND", sizeof(MYSQL_BIND), sizeof(MYSQL_BIND_HANDROLLED));
    printf("\n");

    printf("Field offsets:\n");
    offset("length",          offsetof(MYSQL_BIND, length),
                             offsetof(MYSQL_BIND_HANDROLLED, length));
    offset("is_null",         offsetof(MYSQL_BIND, is_null),
                             offsetof(MYSQL_BIND_HANDROLLED, is_null));
    offset("buffer",          offsetof(MYSQL_BIND, buffer),
                             offsetof(MYSQL_BIND_HANDROLLED, buffer));
    offset("error",           offsetof(MYSQL_BIND, error),
                             offsetof(MYSQL_BIND_HANDROLLED, error));
    offset("buffer_length",   offsetof(MYSQL_BIND, buffer_length),
                             offsetof(MYSQL_BIND_HANDROLLED, buffer_length));
    offset("length_value",    offsetof(MYSQL_BIND, length_value),
                             offsetof(MYSQL_BIND_HANDROLLED, length_value));
    offset("buffer_type",     offsetof(MYSQL_BIND, buffer_type),
                             offsetof(MYSQL_BIND_HANDROLLED, buffer_type));
    offset("error_value",     offsetof(MYSQL_BIND, error_value),
                             offsetof(MYSQL_BIND_HANDROLLED, error_value));
    offset("is_null_value",   offsetof(MYSQL_BIND, is_null_value),
                             offsetof(MYSQL_BIND_HANDROLLED, is_null_value));
    offset("extension",       offsetof(MYSQL_BIND, extension),
                             offsetof(MYSQL_BIND_HANDROLLED, extension));
    printf("\n");

    printf("sizeof(my_bool) = %zu  (typedef in mysql.h line 40: char)\n",
           sizeof(((MYSQL_BIND*)0)->error_value));
    printf("\n");

    printf("Resolved .so for mysql_stmt_bind_param:\n");
    for (int i = 0; SONAMES[i]; ++i) {
        void* h = dlopen(SONAMES[i], RTLD_LAZY);
        if (!h) {
            printf("  %-26s dlopen: %s\n", SONAMES[i], dlerror());
            continue;
        }
        void* sym = dlsym(h, "mysql_stmt_bind_param");
        if (!sym) {
            printf("  %-26s dlsym(mysql_stmt_bind_param): %s\n",
                   SONAMES[i], dlerror());
            dlclose(h);
            continue;
        }
        Dl_info info;
        if (dladdr(sym, &info) && info.dli_fname) {
            printf("  %-26s -> %s (%p)\n",
                   SONAMES[i], info.dli_fname, sym);
        } else {
            printf("  %-26s -> dladdr failed (%p)\n", SONAMES[i], sym);
        }
        dlclose(h);
    }
    printf("\n");

    printf("=== %s ===\n", fail ? "MISMATCH DETECTED" : "OK");
    return fail ? 1 : 0;
}
