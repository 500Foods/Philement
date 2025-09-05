// getdb2ver.c — robust DB2 client version sniff from libdb2.so (no connection, no external cmds)
#define _GNU_SOURCE
#include <dlfcn.h>
#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

/* ---------- helpers to find mapped lib path ---------- */
static const char* find_lib_path_from_maps(const char* needle) {
    static char path[4096];
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return NULL;
    char line[8192];
    while (fgets(line, sizeof(line), f)) {
        if (!strstr(line, needle)) continue;
        char* s = strchr(line, '/');
        if (!s) continue;
        size_t len = strlen(s);
        while (len && (s[len-1] == '\n' || s[len-1] == ' ')) s[--len] = 0;
        strncpy(path, s, sizeof(path)-1);
        path[sizeof(path)-1] = 0;
        fclose(f);
        return path;
    }
    fclose(f);
    return NULL;
}

/* ---------- IPv4 filters ---------- */
static int is_rfc1918_or_local_ip4(const char* v) {
    int a,b,c,d;
    if (sscanf(v, "%d.%d.%d.%d", &a,&b,&c,&d) != 4) return 0;
    if (a<0||a>255||b<0||b>255||c<0||c>255||d<0||d>255) return 0;
    if (a == 10) return 1;
    if (a == 127) return 1;
    if (a == 192 && b == 168) return 1;
    if (a == 169 && b == 254) return 1;
    if (a == 172 && (b >= 16 && b <= 31)) return 1;
    return 0;
}

/* ---------- version acceptance ---------- */
static int parse_major(const char* v) { int maj=-1; sscanf(v, "%d", &maj); return maj; }
static int plausible_db2_version(const char* v, int dots) {
    int maj = parse_major(v);
    if (maj < 8 || maj > 15) return 0;        // drop nonsense like 27.*
    if (dots < 1 || dots > 3) return 0;
    return 1;
}

/* ---------- ranking ---------- */
typedef struct { char text[64]; int dots; int score; } verhit;

static int kw_nearby(const char* hay, size_t start, size_t end) {
    static const char* kws[] = {"DB2","IBM","Data Server","Driver","ODBC","CLI","db2"};
    size_t len = strlen(hay);
    size_t lo = start > 400 ? start - 400 : 0;
    size_t hi = end + 400 < len ? end + 400 : len;
    for (int i = 0; i < (int)(sizeof(kws)/sizeof(kws[0])); ++i)
        if (memmem(hay + lo, hi - lo, kws[i], strlen(kws[i]))) return 1;
    return 0;
}

static int score_hit(const char* hay, size_t start, size_t end, int dots, const char* vstr) {
    // Hard drops first
    char pre  = (start>0) ? hay[start-1] : '\0';
    char post = hay[end];
    if (pre >= '0' && pre <= '9') return -99999;               // started mid-number
    if (!kw_nearby(hay, start, end)) return -99999;            // must be near DB2-ish text
    if (dots == 3 && is_rfc1918_or_local_ip4(vstr)) return -99999;
    if (!plausible_db2_version(vstr, dots)) return -99999;

    // Base score: more dots is better
    int score = dots * 100;

    // Prefer majors we actually expect
    int maj = parse_major(vstr);
    if (maj == 11) score += 25;                                // DB2 11.x is very common

    // Heuristic nudge: 11.1.*.* and 11.5.*.* are typical
    int minor = -1; sscanf(vstr, "%*d.%d", &minor);
    if (maj == 11 && (minor == 1 || minor == 5)) score += 15;

    // Light penalty if next/prev is URL-ish
    if (pre == '/' || pre == ':' || pre == '@' || post == '/' || post == ':') score -= 10;

    return score;
}

/* ---------- scan file for versions ---------- */
static int scan_file_for_version(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "open(%s) failed\n", path); return 2; }

    // Tighter regex: each segment 1–3 digits; total 2–4 segments.
    regex_t rx;
    regcomp(&rx, "([0-9]{1,2}\\.[0-9]{1,3}(\\.[0-9]{1,3}){0,2})", REG_EXTENDED);

    char buf[1<<16];
    char carry[512] = {0};
    size_t carry_len = 0;

    verhit best = {{0}, 0, -1};
    while (1) {
        size_t n = fread(buf, 1, sizeof(buf), f);
        if (!n) break;

        static char str[1<<18];
        size_t k = 0;
        if (carry_len) { memcpy(str, carry, carry_len); k = carry_len; carry_len = 0; }
        for (size_t i = 0; i < n; ++i) {
            unsigned char c = (unsigned char)buf[i];
            if ((c >= 32 && c < 127) || c=='\n' || c=='\r' || c=='\t')
                str[k++] = (char)c;
            else if (k && str[k-1] != '\n')
                str[k++] = '\n';
            if (k >= sizeof(str)-2) { str[k++] = '\n'; break; }
        }
        str[k] = 0;

        size_t tail = (k > sizeof(carry) ? sizeof(carry) : k);
        if (tail) { memcpy(carry, str + k - tail, tail); carry_len = tail; }

        const char* p = str;
        size_t off = 0;
        regmatch_t m[1];
        while (!regexec(&rx, p, 1, m, 0)) {
            size_t s = off + (size_t)m[0].rm_so;
            size_t e = off + (size_t)m[0].rm_eo;
            int len = (int)(m[0].rm_eo - m[0].rm_so);
            if (len > 0 && len < 63) {
                char v[64]; memcpy(v, p + m[0].rm_so, len); v[len] = 0;
                int dots = 0; for (int i=0;i<len;i++) if (v[i]=='.') dots++;

                int sc = score_hit(str, s, e, dots, v);
                if (sc > best.score) {
                    strncpy(best.text, v, sizeof(best.text)-1);
                    best.text[sizeof(best.text)-1] = 0;
                    best.dots = dots;
                    best.score = sc;
                }
            }
            p  += m[0].rm_eo;
            off += m[0].rm_eo;
        }
    }
    fclose(f);
    regfree(&rx);

    if (best.score < 0) {
        fprintf(stderr, "No acceptable version string found inside %s\n", path);
        return 1;
    }
    printf("DB2 client library version (heuristic): %s\n", best.text);
    return 0;
}

int main(void) {
    const char* libname = getenv("DB2_LIB");
    if (!libname || !*libname) libname = "libdb2.so";

    void* h = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
    if (!h) { fprintf(stderr, "dlopen(%s): %s\n", libname, dlerror()); return 2; }

    // Prefer dladdr(dlsym("SQLAllocHandle"))
    void* sym = dlsym(h, "SQLAllocHandle");
    const char* path = NULL;
    if (sym) {
        Dl_info di = {0};
        if (dladdr(sym, &di) && di.dli_fname && di.dli_fname[0] == '/')
            path = di.dli_fname;
    }
    if (!path) path = find_lib_path_from_maps("libdb2.so");

    if (!path) {
        fprintf(stderr, "Could not determine on-disk path for libdb2.so (dladdr/maps)\n");
        dlclose(h);
        return 2;
    }

    int rc = scan_file_for_version(path);
    dlclose(h);
    return rc;
}

