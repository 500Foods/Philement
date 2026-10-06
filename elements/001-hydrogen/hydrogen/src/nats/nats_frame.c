/*
 * NATS control-line builders. Payloads are written separately.
 */

#include <src/hydrogen.h>

#include <src/nats/nats_internal.h>

#include <stdio.h>
#include <string.h>

bool nats_token_ok(const char *text) {
    const char *cursor;

    if (!text || text[0] == '\0') {
        return false;
    }
    for (cursor = text; *cursor != '\0'; cursor++) {
        if (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') {
            return false;
        }
    }
    return true;
}

const char *nats_next_token(const char *text, char *out, size_t cap) {
    size_t used = 0;

    if (!text || !out || cap < 2) {
        return NULL;
    }
    while (*text == ' ') {
        text++;
    }
    if (*text == '\0') {
        return NULL;
    }
    while (*text != '\0' && *text != ' ') {
        if (used + 1 >= cap) {
            return NULL;
        }
        out[used++] = *text++;
    }
    out[used] = '\0';
    return text;
}

int nats_frame_sub(char *dst, size_t cap, const char *subject, const char *queue, int sid) {
    int written;

    if (!dst || cap < 8 || !nats_token_ok(subject) || sid < 1) {
        return -1;
    }
    if (queue && queue[0] != '\0') {
        if (!nats_token_ok(queue)) {
            return -1;
        }
        written = snprintf(dst, cap, "SUB %s %s %d\r\n", subject, queue, sid);
    } else {
        written = snprintf(dst, cap, "SUB %s %d\r\n", subject, sid);
    }
    if (written < 0 || (size_t)written >= cap) {
        return -1;
    }
    return written;
}

int nats_frame_unsub(char *dst, size_t cap, int sid) {
    int written;

    if (!dst || cap < 8 || sid < 1) {
        return -1;
    }
    written = snprintf(dst, cap, "UNSUB %d\r\n", sid);
    if (written < 0 || (size_t)written >= cap) {
        return -1;
    }
    return written;
}

int nats_frame_pub_header(char *dst, size_t cap, const char *subject, size_t len) {
    int written;

    if (!dst || !nats_token_ok(subject)) {
        return -1;
    }
    written = snprintf(dst, cap, "PUB %s %zu\r\n", subject, len);
    if (written < 0 || (size_t)written >= cap) {
        return -1;
    }
    return written;
}

int nats_connect_send(void) {
    const NATSConfig *cfg;
    json_t *obj;
    char *json_text = NULL;
    char *line = NULL;
    size_t line_cap;
    int written;
    int rc = -1;

    if (!app_config) {
        return -1;
    }
    cfg = &app_config->nats;
    obj = json_object();
    if (!obj) {
        return -1;
    }
    json_object_set_new(obj, "verbose", json_false());
    json_object_set_new(obj, "pedantic", json_true());
    json_object_set_new(obj, "protocol", json_integer(1));
    json_object_set_new(obj, "lang", json_string("c"));
    json_object_set_new(obj, "version", json_string(VERSION));
    json_object_set_new(obj, "no_echo", json_true());
    if (cfg->InstanceId && cfg->InstanceId[0] != '\0') {
        json_object_set_new(obj, "name", json_string(cfg->InstanceId));
    }
    if (cfg->Username && cfg->Username[0] != '\0') {
        json_object_set_new(obj, "user", json_string(cfg->Username));
    }
    if (cfg->Password && cfg->Password[0] != '\0') {
        json_object_set_new(obj, "pass", json_string(cfg->Password));
    }
    json_text = json_dumps(obj, JSON_COMPACT);
    json_decref(obj);
    if (!json_text) {
        return -1;
    }
    line_cap = strlen(json_text) + 16;
    line = malloc(line_cap);
    if (!line) {
        free(json_text);
        return -1;
    }
    written = snprintf(line, line_cap, "CONNECT %s\r\n", json_text);
    free(json_text);
    if (written > 0 && (size_t)written < line_cap) {
        rc = nats_io_write(line, (size_t)written);
    }
    free(line);
    return rc;
}
