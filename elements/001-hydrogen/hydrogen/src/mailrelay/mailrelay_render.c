/*
 * Mail Relay RFC 5322 message rendering implementation.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay_render.h>
#include <src/mailrelay/mailrelay_test_seams.h>
#include <src/mailrelay/mailrelay_message.h>

#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

void render_grow(char** buf, const size_t* len, size_t* cap, size_t extra) {
    if (*len + extra + 1 <= *cap) return;
    size_t new_cap = *cap;
    while (new_cap < *len + extra + 1) {
        size_t tmp_cap = new_cap * 2;
        if (tmp_cap < new_cap) return;
        new_cap = tmp_cap;
    }
    char* tmp = realloc(*buf, new_cap);
    if (!tmp) return;
    *buf = tmp;
    *cap = new_cap;
}

void render_mem(char** buf, size_t* len, size_t* cap, const char* data, size_t data_len) {
    render_grow(buf, len, cap, data_len);
    memcpy(*buf + *len, data, data_len);
    *len += data_len;
    (*buf)[*len] = '\0';
}

void render_str(char** buf, size_t* len, size_t* cap, const char* s) {
    render_mem(buf, len, cap, s, strlen(s));
}

void render_header(char** buf, size_t* len, size_t* cap, const char* name, const char* value) {
    if (!value || !value[0]) return;
    size_t nlen = strlen(name);
    size_t vlen = strlen(value);
    render_grow(buf, len, cap, nlen + 2 + vlen + 2);
    memcpy(*buf + *len, name, nlen);
    *len += nlen;
    (*buf)[(*len)++] = ':';
    (*buf)[(*len)++] = ' ';
    memcpy(*buf + *len, value, vlen);
    *len += vlen;
    (*buf)[(*len)++] = '\r';
    (*buf)[(*len)++] = '\n';
    (*buf)[*len] = '\0';
}

void render_header_list(char** buf, size_t* len, size_t* cap, const char* name, char* const* arr, int count) {
    if (!arr || count <= 0) return;
    size_t total = 0;
    int items = 0;
    for (int i = 0; i < count; i++) {
        if (arr[i] && arr[i][0]) {
            total += strlen(arr[i]);
            if (items > 0) total += 2;
            items++;
        }
    }
    if (total == 0) return;

    char* list = malloc(total + 1);
    if (!list) return;
    list[0] = '\0';
    for (int i = 0; i < count; i++) {
        if (arr[i] && arr[i][0]) {
            if (list[0]) strcat(list, ", ");
            strcat(list, arr[i]);
        }
    }
    render_header(buf, len, cap, name, list);
    free(list);
}

char* render_boundary(const char* message_id) {
    size_t id_len = strlen(message_id);
    size_t bound_len = 2;
    for (size_t i = 0; i < id_len; i++) {
        unsigned char c = (unsigned char)message_id[i];
        if (isalnum(c) || c == '_' || c == '-') bound_len++;
    }
    char* b = malloc(bound_len + 1);
    if (!b) return NULL;
    b[0] = '-';
    b[1] = '-';
    size_t j = 2;
    for (size_t i = 0; i < id_len; i++) {
        unsigned char c = (unsigned char)message_id[i];
        if (isalnum(c) || c == '_' || c == '-') {
            b[j++] = (char)c;
        }
    }
    b[j] = '\0';
    return b;
}

void render_mime_part(char** buf, size_t* len, size_t* cap, const char* boundary, const char* part_type, const char* body) {
    size_t prefix = 2 + strlen(boundary) + 64;
    char* tmp = malloc(prefix + 1);
    if (!tmp) return;
    snprintf(tmp, prefix, "\r\n--%s\r\nContent-Type: %s; charset=UTF-8\r\nContent-Transfer-Encoding: 7bit\r\n\r\n", boundary, part_type);
    render_str(buf, len, cap, tmp);
    if (body) render_str(buf, len, cap, body);
    free(tmp);
}

void render_rfc822_date(char* buf, size_t cap, time_t t) {
    struct tm tm_buf;
    gmtime_r(&t, &tm_buf);
    strftime(buf, cap, "%a, %d %b %Y %H:%M:%S +0000", &tm_buf);
}

bool mailrelay_render_message(const MailRelayMessage* m, const char* default_from, const char* app_name, char** out) {
    if (!m || !out) return false;

    const char* from = m->from ? m->from : default_from;
    if (!from) return false;

    size_t cap = 4096;
    size_t len = 0;
    char* result = malloc(cap);
    if (!result) return false;
    result[0] = '\0';

    char date_buf[64];
    time_t now = mailrelay_seam_time();
    render_rfc822_date(date_buf, sizeof(date_buf), now);
    render_header(&result, &len, &cap, "Date", date_buf);
    render_header(&result, &len, &cap, "From", from);
    if (m->reply_to && m->reply_to[0]) {
        render_header(&result, &len, &cap, "Reply-To", m->reply_to);
    }
    render_header_list(&result, &len, &cap, "To", m->to, m->to_count);
    if (m->cc_count > 0) {
        render_header_list(&result, &len, &cap, "Cc", m->cc, m->cc_count);
    }
    render_header(&result, &len, &cap, "Subject", m->subject);

    char mid[256];
    mailrelay_seam_message_id(mid, sizeof(mid), app_name);
    render_header(&result, &len, &cap, "Message-ID", mid);

    bool has_text = m->text_body && m->text_body[0];
    bool has_html = m->html_body && m->html_body[0];

    if (has_text && has_html) {
        char* boundary = render_boundary(mid);
        if (!boundary) {
            free(result);
            return false;
        }
        char ct[512];
        snprintf(ct, sizeof(ct), "multipart/alternative; boundary=\"%s\"", boundary);
        render_header(&result, &len, &cap, "MIME-Version", "1.0");
        render_header(&result, &len, &cap, "Content-Type", ct);

        render_mime_part(&result, &len, &cap, boundary, "text/plain", m->text_body);
        render_mime_part(&result, &len, &cap, boundary, "text/html", m->html_body);

        char suffix[64];
        snprintf(suffix, sizeof(suffix), "\r\n--%s--\r\n", boundary);
        render_str(&result, &len, &cap, suffix);
        free(boundary);
    } else if (has_text) {
        render_header(&result, &len, &cap, "Content-Type", "text/plain; charset=UTF-8");
        render_mem(&result, &len, &cap, "\r\n", 2);
        render_str(&result, &len, &cap, m->text_body);
        render_mem(&result, &len, &cap, "\r\n", 2);
    } else if (has_html) {
        render_header(&result, &len, &cap, "Content-Type", "text/html; charset=UTF-8");
        render_mem(&result, &len, &cap, "\r\n", 2);
        render_str(&result, &len, &cap, m->html_body);
        render_mem(&result, &len, &cap, "\r\n", 2);
    } else {
        render_mem(&result, &len, &cap, "\r\n", 2);
    }

    *out = result;
    return true;
}
