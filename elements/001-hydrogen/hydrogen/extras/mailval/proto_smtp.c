/*
 * SMTP module: full receive path with STARTTLS, AUTH capture (secrets masked),
 * and delivery into the shared mailbox store. Messages land in INBOX where
 * IMAP/JMAP can later serve them.
 */

#define _GNU_SOURCE 1
#include "mailval.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

static void smtp_greet(mv_conn* c) {
    mv_conn_printf(c, "220 mailval ESMTP ready\r\n");
}

/* Extract a header value (case-insensitive) from a raw message. Returns a
 * malloc'd string (caller frees) or NULL. */
static char* smtp_header(const char* raw, const char* name) {
    size_t nl = strlen(name);
    const char* p = raw;
    while ((p = strcasestr(p, name)) != NULL) {
        /* header must be at line start or preceded by newline */
        if (p == raw || p[-1] == '\n') {
            p += nl;
            while (*p == ' ' || *p == ':') p++;
            const char* end = p;
            while (*end && *end != '\n') end++;
            size_t len = (size_t)(end - p);
            while (len > 0 && (p[len-1] == '\r' || p[len-1] == ' ')) len--;
            char* out = mv_xmalloc(len + 1);
            memcpy(out, p, len);
            out[len] = '\0';
            return out;
        }
        p += nl;
    }
    return NULL;
}

static void smtp_handle_data(mv_conn* c, mv_capture* cap, const char* from, const char* to) {
    mv_buf body;
    mv_buf_init(&body);
    for (;;) {
        char line[MV_MAX_LINE];
        int n = mv_conn_readline(c, line, sizeof(line));
        if (n < 0) break;
        if (strcmp(line, ".") == 0) break;
        /* Unstuff a leading dot per RFC 5321. */
        if (line[0] == '.' && line[1] != '\0') {
            mv_buf_append_str(&body, line + 1);
        } else {
            mv_buf_append_str(&body, line);
        }
        mv_buf_append_str(&body, "\r\n");
    }
    char* raw = body.data ? body.data : mv_xstrdup("");
    char* sf = smtp_header(raw, "From:");
    char* st = smtp_header(raw, "To:");
    char* ss = smtp_header(raw, "Subject:");
    char date[64];
    mv_now_rfc3339(date, sizeof(date));

    mv_capture_meta(cap, "envelope_from", from && *from ? from : (sf ? sf : ""));
    mv_capture_meta(cap, "envelope_to", to && *to ? to : (st ? st : ""));
    mv_capture_meta(cap, "subject", ss ? ss : "");

    store_lock();
    mv_mailbox* inbox = store_mailbox_get("INBOX", true);
    unsigned int uid = store_append(inbox, from, to,
        ss ? ss : "", date, raw, (long)body.len, 0);
    store_unlock();

    mv_capture_meta(cap, "stored_uid", uid ? "yes" : "no");
    mv_conn_printf(c, "250 OK: message accepted (uid %u)\r\n", uid);

    free(sf); free(st); free(ss); mv_buf_free(&body);
}

static void smtp_auth_plain(mv_conn* c, mv_capture* cap, const char* b64) {
    unsigned char dec[256];
    int n = mv_base64_decode(b64, dec, sizeof(dec));
    char user[128]; user[0] = '\0';
    if (n > 0) {
        /* form: ^@user^@pass */
        int i = 0;
        while (i < n && dec[i]) i++;
        i++; /* skip NUL */
        int u = 0;
        while (i < n && dec[i] && u < (int)sizeof(user) - 1) user[u++] = (char)dec[i++];
        user[u] = '\0';
    }
    mv_capture_meta(cap, "auth_method", "PLAIN");
    mv_capture_meta(cap, "auth_user", user);
    mv_capture_meta(cap, "auth_secret", "[masked]");
    mv_conn_printf(c, "235 Authentication successful\r\n");
}

static void smtp_auth_login(mv_conn* c, mv_capture* cap) {
    char b64user[256]; b64user[0] = '\0';
    mv_conn_printf(c, "334 VXNlcm5hbWU6\r\n"); /* base64("Username:") */
    mv_conn_readline(c, b64user, sizeof(b64user));
    unsigned char dec[256];
    int n = mv_base64_decode(b64user, dec, sizeof(dec));
    char user[128]; user[0] = '\0';
    if (n > 0) {
        int k = 0;
        while (k < n && dec[k]) k++;
        k++; /* skip NUL */
        int x = 0;
        while (k < n && dec[k] && x < (int)sizeof(user) - 1) user[x++] = (char)dec[k++];
        user[x] = '\0';
    }
    mv_conn_printf(c, "334 UGFzc3dvcmQ6\r\n"); /* base64("Password:") */
    char b64pass[256];
    mv_conn_readline(c, b64pass, sizeof(b64pass));
    mv_capture_meta(cap, "auth_method", "LOGIN");
    mv_capture_meta(cap, "auth_user", user);
    mv_capture_meta(cap, "auth_secret", "[masked]");
    mv_conn_printf(c, "235 Authentication successful\r\n");
}

void proto_smtp_handle(mv_conn* c, const char* peer, mv_capture* cap) {
    (void)peer;
    bool tls_avail = (g_cfg.cert_file != NULL);
    char mail_from[1024]; mail_from[0] = '\0';
    char rcpt_to[1024]; rcpt_to[0] = '\0';

    smtp_greet(c);
    for (;;) {
        char line[MV_MAX_LINE];
        int n = mv_conn_readline(c, line, sizeof(line));
        if (n < 0) break;
        mv_capture_command(cap, line);
        char cmd[64]; char arg[MV_MAX_LINE];
        cmd[0] = arg[0] = '\0';
        sscanf(line, "%63s %16383[^\n]", cmd, arg);

        if (mv_ieq(cmd, "QUIT")) {
            mv_conn_printf(c, "221 Bye\r\n");
            mv_capture_reply(cap, "221 Bye");
            break;
        } else if (mv_ieq(cmd, "EHLO") || mv_ieq(cmd, "HELO")) {
            mv_conn_printf(c, "250-mailval greets %s\r\n", arg);
            mv_conn_printf(c, "250-PIPELINING\r\n");
            mv_conn_printf(c, "250-SIZE 10485760\r\n");
            mv_conn_printf(c, "250-8BITMIME\r\n");
            if (tls_avail && !c->tls) mv_conn_printf(c, "250-STARTTLS\r\n");
            mv_conn_printf(c, "250 AUTH LOGIN PLAIN\r\n");
        } else if (mv_ieq(cmd, "STARTTLS")) {
            if (!tls_avail || c->tls) {
                mv_conn_printf(c, "503 STARTTLS unavailable\r\n");
            } else {
                mv_conn_printf(c, "220 Ready to start TLS\r\n");
                mv_capture_reply(cap, "220 Ready to start TLS");
                if (mv_conn_starttls(c) != 0) break;
            }
        } else if (mv_ieq(cmd, "AUTH")) {
            if (strncasecmp(arg, "PLAIN", 5) == 0) {
                char* sp = arg + 5;
                while (*sp == ' ') sp++;
                smtp_auth_plain(c, cap, sp);
            } else if (strncasecmp(arg, "LOGIN", 5) == 0) {
                smtp_auth_login(c, cap);
            } else {
                mv_conn_printf(c, "504 Mechanism not supported\r\n");
            }
        } else if (mv_ieq(cmd, "MAIL")) {
            char* p = strcasestr(line, "FROM:");
            if (p) { p += 5; while (*p == ' ') p++; char* e = p + strcspn(p, ">\r\n "); if (*e) *e='\0'; if (*p=='<') p++; char* c2 = strchr(p, '>'); if (c2) *c2='\0'; strncpy(mail_from, p, sizeof(mail_from)-1); }
            mv_conn_printf(c, "250 OK\r\n");
        } else if (mv_ieq(cmd, "RCPT")) {
            char* p = strcasestr(line, "TO:");
            if (p) { p += 3; while (*p == ' ') p++; char* e = p + strcspn(p, ">\r\n "); if (*e) *e='\0'; if (*p=='<') p++; char* c2 = strchr(p, '>'); if (c2) *c2='\0'; strncpy(rcpt_to, p, sizeof(rcpt_to)-1); }
            mv_conn_printf(c, "250 OK\r\n");
        } else if (mv_ieq(cmd, "DATA")) {
            mv_conn_printf(c, "354 End data with <CR><LF>.<CR><LF>\r\n");
            mv_capture_reply(cap, "354 End data");
            smtp_handle_data(c, cap, mail_from, rcpt_to);
        } else if (mv_ieq(cmd, "RSET")) {
            mail_from[0] = rcpt_to[0] = '\0';
            mv_conn_printf(c, "250 OK\r\n");
        } else if (mv_ieq(cmd, "NOOP")) {
            mv_conn_printf(c, "250 OK\r\n");
        } else {
            mv_conn_printf(c, "500 Syntax error\r\n");
        }
    }
}

proto_module proto_smtp = { "smtp", proto_smtp_handle };
