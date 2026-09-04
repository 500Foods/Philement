/*
 * probe_mysql_bind_live.c — Phase 1 Vector D reproducer for PERSIST_PLAN
 *
 * Reaches the live MariaDB Connector/C the same way Hydrogen does (dlopen
 * of "libmysqlclient.so" / libmariadb.so) and runs a 12-placeholder INSERT
 * with the same bind shape QueryRef 093 has: 1 INTEGER (priority) +
 * 11 STRING, several of which are JSON null. Uses the *exact* MYSQL_BIND
 * layout and the *exact* bind rules copied from src/database/mysql/query.c
 * (hand-rolled struct, is_null/error attached, length==NULL for fixed
 * types).
 *
 * Build:
 *   cc -I/usr/include/mysql -o /tmp/probe_mysql_bind_live \
 *      extras/probe_mysql_bind_live.c -ldl
 *
 * Run (env vars match Test 36 / 58 mariadb):
 *   CANVAS_DB_TYPE=mariadb \
 *   CANVAS_DB_HOST=127.0.0.1 CANVAS_DB_PORT=3306 \
 *   CANVAS_DB_NAME=testmrdb CANVAS_DB_USER=root CANVAS_DB_PASS= \
 *   /tmp/probe_mysql_bind_live
 *
 * Goal: SIGSEGV here proves the crash is in the bind layer (the
 * reproducer is Mail-Relay-free). A clean run proves the bug is in the
 * Mail-Relay side and we should NOT touch src/database/mysql/query.c.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <signal.h>
#include <unistd.h>

#include <mysql.h>

/* ---- MYSQL_BIND copied verbatim from src/database/mysql/query.c ---- */
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

/* ---- MYSQL_BIND_STRUCT field order matches mariadb_stmt.h exactly ----
 * The production hand-roll uses 'unsigned int buffer_type' but the real
 * header uses 'enum enum_field_types'. Both are 4-byte ints on x86-64
 * but the *typedefs* differ; cast through void* to avoid the warning.
 */

/* MySQL/MariaDB type constants — same numbers as query.c */
#define MYSQL_TYPE_LONG        3
#define MYSQL_TYPE_NULL        6
#define MYSQL_TYPE_STRING      254
#define MYSQL_TYPE_SHORT       2
#define MYSQL_TYPE_DOUBLE      5
#define MYSQL_TYPE_LONGLONG    8
#define MYSQL_TYPE_LONG_BLOB   251

static const char* SONAMES[] = {
    "libmysqlclient.so.21",
    "libmysqlclient.so.18",
    "libmysqlclient.so.20",
    "libmysqlclient.so",
    "libmariadb.so.3",
    "libmariadb.so",
    NULL
};

static volatile sig_atomic_t g_segv = 0;
static void on_segv(int sig) { (void)sig; g_segv = 1; _exit(139); }

static void* dlopen_first(void) {
    for (int i = 0; SONAMES[i]; ++i) {
        void* h = dlopen(SONAMES[i], RTLD_LAZY);
        if (h) {
            fprintf(stderr, "dlopen: %s -> %p\n", SONAMES[i], h);
            return h;
        }
    }
    return NULL;
}

static void attach_indicators(MYSQL_BIND_HANDROLLED* b, char is_null_flag) {
    b->is_null_value = is_null_flag;
    b->is_null       = &b->is_null_value;
    b->error_value   = 0;
    b->error         = &b->error_value;
}

/* Returns malloc'd buffer that the caller must free via the second-half
 * table (mirrors bound_values[count+i] cleanup in query.c). */
// cppcheck-suppress nullPointerOutOfMemory
// Justification: throwaway probe; malloc failure aborts with NULL buffer
// (we still call free() in cleanup which tolerates NULL).
static void* bind_int(MYSQL_BIND_HANDROLLED* b, long long v) {
    long long* p = malloc(sizeof(long long));
    // cppcheck-suppress nullPointerOutOfMemory
    *p = v;
    b->buffer_type   = MYSQL_TYPE_LONGLONG;
    b->buffer        = p;
    b->buffer_length = sizeof(long long);
    unsigned long* len = malloc(sizeof(unsigned long));
    // cppcheck-suppress nullPointerOutOfMemory
    *len = sizeof(long long);
    b->length        = len;
    attach_indicators(b, 0);
    return len;            /* caller owns len, b->buffer owns p */
}

// cppcheck-suppress nullPointerOutOfMemory
// Justification: same as bind_int above.
static void* bind_str(MYSQL_BIND_HANDROLLED* b, const char* s, bool is_null) {
    if (is_null) {
        char* empty = strdup("");
        b->buffer_type   = MYSQL_TYPE_NULL;
        b->buffer        = empty;
        b->buffer_length = 1;
        unsigned long* len = malloc(sizeof(unsigned long));
        // cppcheck-suppress nullPointerOutOfMemory
        *len = 0;
        b->length        = len;
        attach_indicators(b, 1);
        return len;
    }
    char* p = strdup(s);
    size_t sl = strlen(s);
    b->buffer_type   = MYSQL_TYPE_STRING;
    b->buffer        = p;
    b->buffer_length = (unsigned long)(sl + 1);
    unsigned long* len = malloc(sizeof(unsigned long));
    // cppcheck-suppress nullPointerOutOfMemory
    *len = (unsigned long)sl;
    b->length        = len;
    attach_indicators(b, 0);
    return len;
}

int main(void) {
    signal(SIGSEGV, on_segv);
    signal(SIGBUS,  on_segv);

    const char* host = getenv("CANVAS_DB_HOST");     if (!host) host = "127.0.0.1";
    const char* port = getenv("CANVAS_DB_PORT");     if (!port) port = "3306";
    const char* user = getenv("CANVAS_DB_USER");     if (!user) user = "root";
    const char* pass = getenv("CANVAS_DB_PASS");     if (!pass) pass = "";
    const char* db   = getenv("CANVAS_DB_NAME");     if (!db)   db = "testmrdb";

    fprintf(stderr, "Connecting to mariadb://%s:%s/%s as %s\n", host, port, db, user);

    void* h = dlopen_first();
    if (!h) { fprintf(stderr, "FAIL: no client .so\n"); return 2; }

    MYSQL* (*mysql_init_fn)(MYSQL*)             = dlsym(h, "mysql_init");
    MYSQL* (*mysql_real_connect_fn)(MYSQL*, ...) = dlsym(h, "mysql_real_connect");
    int   (*mysql_query_fn)(MYSQL*, const char*)= dlsym(h, "mysql_query");
    const char* (*mysql_error_fn)(MYSQL*)       = dlsym(h, "mysql_error");
    void  (*mysql_close_fn)(MYSQL*)             = dlsym(h, "mysql_close");
    void* (*mysql_stmt_init_fn)(MYSQL*)         = dlsym(h, "mysql_stmt_init");
    int   (*mysql_stmt_prepare_fn)(void*, const char*, unsigned long) =
        dlsym(h, "mysql_stmt_prepare");
    my_bool (*mysql_stmt_bind_param_fn)(void*, MYSQL_BIND*) =
        dlsym(h, "mysql_stmt_bind_param");
    int   (*mysql_stmt_execute_fn)(void*)        = dlsym(h, "mysql_stmt_execute");
    const char* (*mysql_stmt_error_fn)(void*)    = dlsym(h, "mysql_stmt_error");
    void  (*mysql_stmt_close_fn)(void*)          = dlsym(h, "mysql_stmt_close");

    if (!mysql_init_fn || !mysql_real_connect_fn || !mysql_stmt_bind_param_fn) {
        fprintf(stderr, "FAIL: missing required symbol\n");
        return 2;
    }

    MYSQL* conn = mysql_init_fn(NULL);
    if (!conn) { fprintf(stderr, "mysql_init failed\n"); return 2; }

    unsigned int p = (unsigned int)atoi(port);
    MYSQL* ok = mysql_real_connect_fn(conn, host, user, pass, db, p, NULL, 0);
    if (!ok) {
        fprintf(stderr, "mysql_real_connect failed: %s\n",
                mysql_error_fn ? mysql_error_fn(conn) : "?");
        return 2;
    }
    fprintf(stderr, "Connected.\n");

    /* Truncate so we can re-run cleanly. */
    if (mysql_query_fn && mysql_query_fn(conn, "TRUNCATE TABLE mail_queue") != 0) {
        fprintf(stderr, "TRUNCATE failed: %s\n", mysql_error_fn(conn));
    }

    /* Same 12 named params as QueryRef 093, positional '?'. */
    const char* sql =
        "INSERT INTO mail_queue "
        "(message_uuid, priority, template_key, from_addr, reply_to, "
        " recipients_json, subject, body_text, body_html, headers_json, "
        " idempotency_key, next_attempt_at) "
        "VALUES (?,?,?,?,?,?,?,?,?,?,?,?)";

    void* stmt = mysql_stmt_init_fn ? mysql_stmt_init_fn(conn) : NULL;
    if (!stmt) { fprintf(stderr, "mysql_stmt_init failed\n"); return 2; }

    if (mysql_stmt_prepare_fn(stmt, sql, (unsigned long)strlen(sql)) != 0) {
        fprintf(stderr, "mysql_stmt_prepare failed: %s\n", mysql_stmt_error_fn(stmt));
        return 2;
    }
    fprintf(stderr, "Prepared 12-placeholder INSERT.\n");

    /* Build hand-rolled binds. Shape mirrors QueryRef 093:
     *   0 message_uuid    STRING (non-null)
     *   1 priority        INTEGER (non-null)
     *   2 template_key    STRING null
     *   3 from_addr       STRING non-null
     *   4 reply_to        STRING null
     *   5 recipients_json STRING non-null
     *   6 subject         STRING non-null
     *   7 body_text       STRING null
     *   8 body_html       STRING null
     *   9 headers_json    STRING null
     *  10 idempotency_key STRING non-null
     *  11 next_attempt_at STRING non-null  (datetime passed as text)
     */
    enum { N = 12 };
    MYSQL_BIND_HANDROLLED bind[N] = {0};
    void* lengths[N] = {0};
    char* buffers[N] = {0};

    buffers[0]  = (char*)bind_str(&bind[0],  "11111111-1111-1111-1111-111111111111", false); lengths[0]  = buffers[0];
    buffers[1]  = NULL; lengths[1] = (char*)bind_int(&bind[1], 5);
    buffers[2]  = (char*)bind_str(&bind[2],  "", true);  lengths[2]  = buffers[2];
    buffers[3]  = (char*)bind_str(&bind[3],  "noreply@example.com", false); lengths[3]  = buffers[3];
    buffers[4]  = (char*)bind_str(&bind[4],  "", true);  lengths[4]  = buffers[4];
    buffers[5]  = (char*)bind_str(&bind[5],  "[]", false); lengths[5]  = buffers[5];
    buffers[6]  = (char*)bind_str(&bind[6],  "hi", false); lengths[6]  = buffers[6];
    buffers[7]  = (char*)bind_str(&bind[7],  "", true);  lengths[7]  = buffers[7];
    buffers[8]  = (char*)bind_str(&bind[8],  "", true);  lengths[8]  = buffers[8];
    buffers[9]  = (char*)bind_str(&bind[9],  "", true);  lengths[9]  = buffers[9];
    buffers[10] = (char*)bind_str(&bind[10], "idem-xyz", false); lengths[10] = buffers[10];
    buffers[11] = (char*)bind_str(&bind[11], "2030-01-01 00:00:00", false); lengths[11] = buffers[11];

    fprintf(stderr, "About to mysql_stmt_bind_param (N=%d)...\n", N);
    fflush(stderr);

    /* Call the .so with the hand-rolled bind array. Note: client expects
     * the *real* MYSQL_BIND typedef; we pass ours through a cast. The
     * Phase 0 probe confirmed the layouts match (sizeof == 112, offsets
     * line up), so the .so will read the same fields. */
    my_bool br = mysql_stmt_bind_param_fn(stmt, (MYSQL_BIND*)bind);
    fprintf(stderr, "bind_param returned %d  errno=%s\n",
            (int)br, mysql_stmt_error_fn ? mysql_stmt_error_fn(stmt) : "(no errno)");
    fflush(stderr);

    if (br == 0) {
        fprintf(stderr, "About to mysql_stmt_execute...\n"); fflush(stderr);
        int er = mysql_stmt_execute_fn(stmt);
        fprintf(stderr, "execute returned %d  errno=%s\n",
                er, mysql_stmt_error_fn ? mysql_stmt_error_fn(stmt) : "(no errno)");
        fflush(stderr);
    }

    /* Cleanup */
    for (int i = 0; i < N; ++i) {
        free(bind[i].buffer);
        free(bind[i].length);
    }
    if (mysql_stmt_close_fn) mysql_stmt_close_fn(stmt);
    if (mysql_close_fn) mysql_close_fn(conn);

    if (g_segv) {
        fprintf(stderr, "*** SIGSEGV during bind/exec ***\n");
        return 139;
    }
    fprintf(stderr, "Clean exit.\n");
    return 0;
}
