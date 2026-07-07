#include "capture.h"
#include "mailval.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

struct mv_capture {
    char* path;
    FILE* fp;
    pthread_mutex_t lock;
    int msg_seq;
    int meta_count;
    bool first;   /* first array element (controls leading comma) */
};

/* Minimal JSON string escaper into a caller buffer (consumed immediately). */
static void mv_json_escape(const char* in, char* out, size_t out_cap) {
    size_t o = 0;
    out[o++] = '"';
    for (size_t i = 0; in && in[i] && o + 6 < out_cap; i++) {
        unsigned char c = (unsigned char)in[i];
        if (c == '"' || c == '\\') {
            out[o++] = '\\'; out[o++] = (char)c;
        } else if (c == '\n') {
            out[o++] = '\\'; out[o++] = 'n';
        } else if (c == '\r') {
            out[o++] = '\\'; out[o++] = 'r';
        } else if (c == '\t') {
            out[o++] = '\\'; out[o++] = 't';
        } else if (c < 0x20) {
            snprintf(out + o, out_cap - o, "\\u%04x", c);
            o += strlen(out + o);
        } else {
            out[o++] = (char)c;
        }
    }
    out[o++] = '"'; out[o] = '\0';
}

mv_capture* mv_capture_create(const char* data_dir, const char* proto, const char* peer) {
    mv_capture* c = mv_xcalloc(1, sizeof(*c));
    pthread_mutex_init(&c->lock, NULL);
    static int counter = 0;
    c->first = true;
    c->path = mv_xmalloc(MV_MAX_PATH);
    snprintf(c->path, MV_MAX_PATH, "%s/mailval_%s_%lld_%d_%d.json",
             data_dir ? data_dir : ".", proto, (long long)time(NULL), counter++, (int)getpid());
    c->fp = fopen(c->path, "w");
    if (!c->fp) c->fp = stdout;
    fprintf(c->fp, "{\n");
    fprintf(c->fp, "  \"protocol\": \"%s\",\n", proto);
    char esc[4096];
    mv_json_escape(peer, esc, sizeof(esc));
    fprintf(c->fp, "  \"peer\": %s,\n", esc);
    fprintf(c->fp, "  \"commands\": [\n");
    return c;
}

static void mv_capture_line(mv_capture* c, const char* dir, const char* text, const char* key, const char* value) {
    if (!c || !c->fp) return;
    pthread_mutex_lock(&c->lock);
    if (!c->first) fprintf(c->fp, ",\n");
    c->first = false;
    char d[64];
    mv_json_escape(dir, d, sizeof(d));
    fprintf(c->fp, "    {\"seq\": %d, \"dir\": %s", c->msg_seq++, d);
    if (text) {
        char t[16384];
        mv_json_escape(text, t, sizeof(t));
        fprintf(c->fp, ", \"text\": %s", t);
    }
    if (key) {
        char k[4096];
        char v[16384];
        mv_json_escape(key, k, sizeof(k));
        mv_json_escape(value ? value : "", v, sizeof(v));
        fprintf(c->fp, ", \"key\": %s, \"value\": %s", k, v);
    }
    fprintf(c->fp, "}");
    fflush(c->fp);
    pthread_mutex_unlock(&c->lock);
}

void mv_capture_command(mv_capture* c, const char* line) {
    mv_capture_line(c, "command", line, NULL, NULL);
}

void mv_capture_reply(mv_capture* c, const char* line) {
    mv_capture_line(c, "reply", line, NULL, NULL);
}

void mv_capture_meta(mv_capture* c, const char* key, const char* value) {
    mv_capture_line(c, "meta", NULL, key, value);
    if (c) c->meta_count++;
}

void mv_capture_close(mv_capture* c) {
    if (!c) return;
    if (c->fp && c->fp != stdout) {
        fprintf(c->fp, "\n  ],\n");
        fprintf(c->fp, "  \"meta_count\": %d,\n", c->meta_count);
        fprintf(c->fp, "  \"closed\": true\n");
        fprintf(c->fp, "}\n");
        fclose(c->fp);
    }
    pthread_mutex_destroy(&c->lock);
    free(c->path);
    free(c);
}
