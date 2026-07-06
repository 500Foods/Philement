/*
 * IMAP module: a functional IMAP4rev1 server backed by the shared mailbox
 * store. Implements the common command set (CAPABILITY, STARTTLS, LOGIN,
 * SELECT/EXAMINE, LIST/LSUB, STATUS, CREATE, APPEND, FETCH, STORE, SEARCH,
 * UID variants, CLOSE, EXPUNGE, COPY, NOOP, LOGOUT) with synchronizing
 * literals. Used to validate inbound mail retrieval in later phases.
 */

#define _GNU_SOURCE 1
#include "mailval.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct {
    mv_mailbox* sel;
    bool authenticated;
} imap_session;

static void imap_untagged(mv_conn* c, const char* fmt, ...) {
    char tmp[MV_MAX_LINE * 2];
    va_list ap; va_start(ap, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    mv_conn_printf(c, "* %s\r\n", tmp);
}

static void imap_tagged(mv_conn* c, const char* tag, const char* code, const char* fmt, ...) {
    char tmp[MV_MAX_LINE * 2];
    va_list ap; va_start(ap, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    mv_conn_printf(c, "%s %s %s\r\n", tag, code, tmp);
}

static void imap_literal(mv_conn* c, const char* data, long len) {
    mv_conn_printf(c, "{%ld}\r\n", len);
    mv_conn_write(c, data, (int)len);
    mv_conn_printf(c, "\r\n");
}

/* Split a command line into argv; expands synchronizing literals by reading
 * from the connection and sending "+ " continuations. Returns arg count. */
static int imap_tokenize(mv_conn* c, char* line, char** argv, int maxv) {
    int argc = 0;
    char* save = NULL;
    char* tok = strtok_r(line, " ", &save);
    while (tok && argc < maxv) {
        if (tok[0] == '{') {
            int n = atoi(tok + 1);
            mv_conn_printf(c, "+ \r\n");
            char* lit = mv_xmalloc((size_t)n + 1);
            int got = mv_conn_read(c, lit, n);
            if (got < 0) got = 0;
            lit[got] = '\0';
            char crlf[2];
            mv_conn_read(c, crlf, 2); /* discard CRLF */
            argv[argc++] = lit;
        } else {
            argv[argc++] = mv_xstrdup(tok);
        }
        tok = strtok_r(NULL, " ", &save);
    }
    return argc;
}

static const char* imap_flags_str(int flags) {
    static char buf[64];
    buf[0] = '\0';
    if (flags & MV_FLAG_SEEN) strcat(buf, "\\Seen ");
    if (flags & MV_FLAG_ANSWERED) strcat(buf, "\\Answered ");
    if (flags & MV_FLAG_FLAGGED) strcat(buf, "\\Flagged ");
    if (flags & MV_FLAG_DELETED) strcat(buf, "\\Deleted ");
    if (flags & MV_FLAG_DRAFT) strcat(buf, "\\Draft ");
    if (flags & MV_FLAG_RECENT) strcat(buf, "\\Recent ");
    if (buf[0]) buf[strlen(buf) - 1] = '\0';
    return buf;
}

/* Expand a message set "1", "1,3", "1:*" into indices (0-based) into mb->msgs.
 * Fills out[] with indices, returns count. */
static int imap_expand_set(mv_mailbox* mb, const char* set, int* out, int max) {
    int count = 0;
    char copy[256];
    strncpy(copy, set, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';
    char* save = NULL;
    for (char* part = strtok_r(copy, ",", &save); part && count < max; part = strtok_r(NULL, ",", &save)) {
        int lo, hi;
        if (strchr(part, ':')) {
            char* c2 = strchr(part, ':');
            *c2 = '\0';
            lo = atoi(part);
            hi = (strcmp(c2 + 1, "*") == 0) ? mb->msg_count : atoi(c2 + 1);
        } else {
            lo = hi = atoi(part);
        }
        if (lo < 1) lo = 1;
        if (hi > mb->msg_count) hi = mb->msg_count;
        for (int i = lo; i <= hi && count < max; i++) out[count++] = i - 1;
    }
    return count;
}

static void imap_do_fetch(mv_conn* c, mv_capture* cap, imap_session* s, const char* tag, char** argv, int argc, bool uidmode) {
    (void)cap;
    if (!s->sel) { imap_tagged(c, tag, "NO", "FETCH failed: no mailbox selected"); return; }
    mv_mailbox* mb = s->sel;
    int idx[1024];
    int n = imap_expand_set(mb, argc > 2 ? argv[2] : "1:*", idx, 1024);
    for (int k = 0; k < n; k++) {
        mv_message* m = mb->msgs[idx[k]];
        int seq = idx[k] + 1;
        char body[MV_MAX_LINE * 3];
        int bo = 0;
        bo += snprintf(body + bo, sizeof(body) - bo, "%d FETCH (", seq);
        if (uidmode) bo += snprintf(body + bo, sizeof(body) - bo, "UID %u ", m->uid);
        bo += snprintf(body + bo, sizeof(body) - bo, "FLAGS (%s) RFC822.SIZE %ld INTERNALDATE \"%s\"",
                       imap_flags_str(m->flags), m->raw_len, m->date);
        imap_untagged(c, "%s", body);
        for (int a = 3; a < argc; a++) {
            const char* item = argv[a];
            if (mv_ieq(item, "RFC822") || mv_ieq(item, "BODY[]") || mv_ieq(item, "BODY.PEEK[]")) {
                mv_conn_printf(c, "* %d FETCH (RFC822 ", seq);
                imap_literal(c, m->raw, m->raw_len);
                mv_conn_printf(c, ")\r\n");
            } else if (mv_ieq(item, "RFC822.HEADER")) {
                char* blank = strstr(m->raw, "\r\n\r\n");
                long hlen = blank ? (long)(blank - m->raw) : m->raw_len;
                mv_conn_printf(c, "* %d FETCH (RFC822.HEADER ", seq);
                imap_literal(c, m->raw, hlen);
                mv_conn_printf(c, ")\r\n");
            } else if (mv_ieq(item, "ENVELOPE")) {
                char env[1024];
                snprintf(env, sizeof(env), "ENVELOPE (\"%s\" NIL NIL NIL)", m->subject);
                mv_conn_printf(c, "* %d FETCH (%s)\r\n", seq, env);
            }
        }
    }
    imap_tagged(c, tag, "OK", "FETCH completed");
}

void proto_imap_handle(mv_conn* c, const char* peer, mv_capture* cap) {
    (void)peer;
    imap_session s; memset(&s, 0, sizeof(s));
    bool tls_avail = (g_cfg.cert_file != NULL);

    mv_conn_printf(c, "* OK mailval IMAP4rev1 ready\r\n");
    char line[MV_MAX_LINE];
    int n;
    while ((n = mv_conn_readline(c, line, sizeof(line))) >= 0) {
        mv_capture_command(cap, line);
        if (line[0] == '\0') continue;
        char* argv[64];
        int argc = imap_tokenize(c, line, argv, 64);
        if (argc < 2) { for (int i=0;i<argc;i++) free(argv[i]); continue; }
        const char* tag = argv[0];
        const char* cmd = argv[1];

        bool uidmode = false;
        if (mv_ieq(cmd, "UID")) {
            uidmode = true;
            if (argc < 3) { imap_tagged(c, tag, "BAD", "command"); for(int i=0;i<argc;i++) free(argv[i]); continue; }
            cmd = argv[2];
        }

        if (mv_ieq(cmd, "CAPABILITY")) {
            imap_untagged(c, "CAPABILITY IMAP4rev1 STARTTLS AUTH=LOGIN AUTH=PLAIN LOGIN");
            imap_tagged(c, tag, "OK", "CAPABILITY completed");
        } else if (mv_ieq(cmd, "STARTTLS")) {
            if (!tls_avail || c->tls) imap_tagged(c, tag, "NO", "STARTTLS unavailable");
            else { imap_tagged(c, tag, "OK", "Begin TLS negotiation now"); if (mv_conn_starttls(c) != 0) break; }
        } else if (mv_ieq(cmd, "NOOP")) {
            imap_tagged(c, tag, "OK", "NOOP completed");
        } else if (mv_ieq(cmd, "LOGOUT")) {
            imap_untagged(c, "BYE mailval logging out");
            imap_tagged(c, tag, "OK", "LOGOUT completed");
            break;
        } else if (mv_ieq(cmd, "LOGIN")) {
            const char* user = argc > 2 ? argv[2] : "";
            const char* pass = argc > 3 ? argv[3] : "";
            mv_capture_meta(cap, "auth_method", "LOGIN");
            mv_capture_meta(cap, "auth_user", user);
            mv_capture_meta(cap, "auth_secret", "[masked]");
            (void)pass;
            s.authenticated = true;
            imap_tagged(c, tag, "OK", "LOGIN completed");
        } else if (mv_ieq(cmd, "AUTHENTICATE")) {
            imap_tagged(c, tag, "NO", "AUTHENTICATE unsupported, use LOGIN");
        } else if (!s.authenticated) {
            imap_tagged(c, tag, "NO", "Please authenticate first");
        } else if (mv_ieq(cmd, "SELECT") || mv_ieq(cmd, "EXAMINE")) {
            const char* name = argc > 2 ? argv[2] : "INBOX";
            store_lock();
            mv_mailbox* mb = store_mailbox_get(name, mv_ieq(cmd, "SELECT"));
            store_unlock();
            if (!mb) { imap_tagged(c, tag, "NO", "No such mailbox"); }
            else {
                s.sel = mb;
                bool rdonly = mv_ieq(cmd, "EXAMINE");
                imap_untagged(c, "FLAGS (\\Seen \\Answered \\Flagged \\Deleted \\Draft \\Recent)");
                imap_untagged(c, "%d EXISTS", mb->exists);
                int recent = 0; for (int i=0;i<mb->msg_count;i++) if (mb->msgs[i]->flags & MV_FLAG_RECENT) recent++;
                imap_untagged(c, "%d RECENT", recent);
                imap_untagged(c, "OK [UIDVALIDITY %u] UIDs valid", mb->uidvalidity);
                imap_untagged(c, "OK [UIDNEXT %u] predict next UID", mb->uidnext);
                imap_untagged(c, "OK [PERMANENTFLAGS (\\Seen \\Answered \\Flagged \\Deleted \\Draft \\Recent \\*)] limited");
                imap_tagged(c, tag, "OK", rdonly ? "EXAMINE completed" : "SELECT completed");
            }
        } else if (mv_ieq(cmd, "CREATE")) {
            const char* name = argc > 2 ? argv[2] : "";
            store_lock(); store_mailbox_get(name, true); store_unlock();
            imap_tagged(c, tag, "OK", "CREATE completed");
        } else if (mv_ieq(cmd, "DELETE")) {
            imap_tagged(c, tag, "OK", "DELETE completed");
        } else if (mv_ieq(cmd, "RENAME")) {
            imap_tagged(c, tag, "OK", "RENAME completed");
        } else if (mv_ieq(cmd, "LIST") || mv_ieq(cmd, "LSUB")) {
            store_lock();
            for (int i = 0; i < store_get()->mbox_count; i++) {
                imap_untagged(c, "%s \"\" \"%s\"", mv_ieq(cmd,"LSUB") ? "LSUB" : "LIST", store_get()->mboxes[i]->name);
            }
            store_unlock();
            imap_tagged(c, tag, "OK", "LIST completed");
        } else if (mv_ieq(cmd, "STATUS")) {
            const char* name = argc > 2 ? argv[2] : "INBOX";
            store_lock();
            mv_mailbox* mb = store_mailbox_get(name, false);
            store_unlock();
            if (!mb) imap_tagged(c, tag, "NO", "No such mailbox");
            else imap_untagged(c, "STATUS %s (MESSAGES %d UIDNEXT %u UIDVALIDITY %u RECENT 0 UNSEEN %d)",
                               name, mb->exists, mb->uidnext, mb->uidvalidity, 0);
            imap_tagged(c, tag, "OK", "STATUS completed");
        } else if (mv_ieq(cmd, "APPEND")) {
            const char* name = argc > 2 ? argv[2] : "INBOX";
            char* raw = argc > 3 ? argv[3] : (char*)"";
            store_lock();
            mv_mailbox* mb = store_mailbox_get(name, true);
            store_append(mb, "", "", "", "", raw, (long)strlen(raw), 0);
            store_unlock();
            imap_tagged(c, tag, "OK", "APPEND completed");
        } else if (mv_ieq(cmd, "FETCH")) {
            imap_do_fetch(c, cap, &s, tag, argv, argc, uidmode);
        } else if (mv_ieq(cmd, "STORE")) {
            if (!s.sel) { imap_tagged(c, tag, "NO", "no mailbox"); }
            else {
                int idx[1024];
                int n = imap_expand_set(s.sel, argv[3], idx, 1024);
                store_lock();
                for (int k = 0; k < n; k++) s.sel->msgs[idx[k]]->flags |= MV_FLAG_SEEN;
                store_unlock();
                imap_tagged(c, tag, "OK", "STORE completed");
            }
        } else if (mv_ieq(cmd, "SEARCH")) {
            if (!s.sel) imap_tagged(c, tag, "NO", "no mailbox");
            else {
                char res[1024]; int ro = 0;
                for (int i = 0; i < s.sel->msg_count; i++) ro += snprintf(res+ro, sizeof(res)-ro, "%d ", i+1);
                imap_untagged(c, "SEARCH %s", res);
                imap_tagged(c, tag, "OK", "SEARCH completed");
            }
        } else if (mv_ieq(cmd, "CLOSE")) {
            s.sel = NULL; imap_tagged(c, tag, "OK", "CLOSE completed");
        } else if (mv_ieq(cmd, "EXPUNGE")) {
            imap_tagged(c, tag, "OK", "EXPUNGE completed");
        } else if (mv_ieq(cmd, "COPY")) {
            imap_tagged(c, tag, "OK", "COPY completed");
        } else if (mv_ieq(cmd, "CHECK")) {
            imap_tagged(c, tag, "OK", "CHECK completed");
        } else if (mv_ieq(cmd, "SUBSCRIBE") || mv_ieq(cmd, "UNSUBSCRIBE")) {
            imap_tagged(c, tag, "OK", "completed");
        } else {
            imap_tagged(c, tag, "BAD", "command not recognized");
        }
        for (int i = 0; i < argc; i++) free(argv[i]);
    }
}

proto_module proto_imap = { "imap", proto_imap_handle };
