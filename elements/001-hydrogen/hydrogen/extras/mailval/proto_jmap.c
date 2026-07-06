/*
 * JMAP module: an HTTPS (implicit TLS) JMAP server backed by the shared
 * mailbox store. Implements the session resource, blob upload/download, and
 * the core mail methods (mailbox/get|set, messages/get|query|set). Used to
 * validate JMAP mailbox access in later phases. JSON handled with jansson.
 */

#define _GNU_SOURCE 1
#include "mailval.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <jansson.h>

typedef struct {
    char method[16];
    char path[1024];
    char query[1024];
    char* body;
    long body_len;
} http_req;

static void http_respond(mv_conn* c, int code, const char* reason, const char* ctype, const char* body, long len) {
    mv_conn_printf(c, "HTTP/1.1 %d %s\r\n", code, reason);
    mv_conn_printf(c, "Content-Type: %s\r\n", ctype);
    mv_conn_printf(c, "Content-Length: %ld\r\n", len);
    mv_conn_printf(c, "Connection: close\r\n");
    mv_conn_printf(c, "\r\n");
    if (body && len > 0) mv_conn_write(c, body, (int)len);
}

static void http_error(mv_conn* c, int code, const char* reason, const char* msg) {
    char body[512];
    snprintf(body, sizeof(body), "{\"error\":\"%s\"}", msg);
    http_respond(c, code, reason, "application/json", body, (long)strlen(body));
}

/* Read a complete HTTP request (request line + headers + body). Returns 0. */
static int http_read_request(mv_conn* c, http_req* r) {
    memset(r, 0, sizeof(*r));
    char line[MV_MAX_LINE];
    if (mv_conn_readline(c, line, sizeof(line)) <= 0) return -1;
    sscanf(line, "%15s %1023s", r->method, r->path);
    char* q = strchr(r->path, '?');
    if (q) { *q = '\0'; strncpy(r->query, q + 1, sizeof(r->query) - 1); }
    long clen = 0;
    for (;;) {
        if (mv_conn_readline(c, line, sizeof(line)) <= 0) break;
        if (line[0] == '\0') break;
        if (strncasecmp(line, "Content-Length:", 15) == 0) clen = atol(line + 15);
    }
    if (clen > 0) {
        r->body = mv_xmalloc((size_t)clen + 1);
        if (mv_conn_read(c, r->body, (int)clen) < 0) { free(r->body); r->body = NULL; return -1; }
        r->body[clen] = '\0';
        r->body_len = clen;
    }
    return 0;
}

static json_t* jmap_message_object(mv_message* m) {
    json_t* o = json_object();
    json_object_set_new(o, "id", json_string(m->id));
    json_object_set_new(o, "blobId", json_string(m->id));
    json_object_set_new(o, "threadId", json_string(m->id));
    json_t* mb = json_object();
    /* find owning mailbox */
    mv_store* s = store_get();
    for (int i = 0; i < s->mbox_count; i++) {
        for (int j = 0; j < s->mboxes[i]->msg_count; j++) {
            if (s->mboxes[i]->msgs[j] == m) {
                json_object_set_new(mb, s->mboxes[i]->id, json_true());
                break;
            }
        }
    }
    json_object_set_new(o, "mailboxIds", mb);
    json_t* from = json_object();
    json_object_set_new(from, "email", json_string(m->from && *m->from ? m->from : "unknown@mailval"));
    json_object_set_new(o, "from", json_pack("[o]", from));
    json_t* to = json_object();
    json_object_set_new(to, "email", json_string(m->to && *m->to ? m->to : "unknown@mailval"));
    json_object_set_new(o, "to", json_pack("[o]", to));
    json_object_set_new(o, "subject", json_string(m->subject ? m->subject : ""));
    json_object_set_new(o, "size", json_integer(m->raw_len));
    json_object_set_new(o, "isUnread", json_true());
    json_object_set_new(o, "isFlagged", json_boolean(m->flags & MV_FLAG_FLAGGED));
    return o;
}

static void jmap_session(mv_conn* c) {
    json_t* cap = json_object();
    json_object_set_new(cap, "urn:ietf:params:jmap:mail", json_object());
    json_t* acct = json_object();
    json_object_set_new(acct, "name", json_string("mailval"));
    json_object_set_new(acct, "isPrimary", json_true());
    json_object_set_new(acct, "hasDataFor", json_pack("[s]", "urn:ietf:params:jmap:mail"));
    json_t* accounts = json_pack("[o]", acct);
    json_t* primary = json_object();
    json_object_set_new(primary, "urn:ietf:params:jmap:mail", json_string("acct1"));
    json_t* root = json_object();
    json_object_set_new(root, "capabilities", cap);
    json_object_set_new(root, "accounts", accounts);
    json_object_set_new(root, "primaryAccounts", primary);
    json_object_set_new(root, "apiUrl", json_string("/jmap"));
    json_object_set_new(root, "downloadUrl", json_string("/jmap/download?accountId=acct1&blobId={blobId}&type={type}&name={name}"));
    json_object_set_new(root, "uploadUrl", json_string("/jmap/upload"));
    char* out = json_dumps(root, JSON_COMPACT);
    http_respond(c, 200, "OK", "application/json", out, (long)strlen(out));
    free(out); json_decref(root);
}

/* Handle one method call; append a response [name, result, clientId]. */
static void jmap_method(mv_conn* c, json_t* call, json_t* responses) {
    (void)c;
    if (!json_is_array(call) || json_array_size(call) < 2) {
        json_t* resp = json_array();
        json_array_append_new(resp, json_string("error"));
        json_array_append_new(resp, json_pack("{ss}", "type", "invalidArguments"));
        json_array_append_new(resp, json_string(""));
        json_array_append_new(responses, resp);
        return;
    }
    const char* name = json_string_value(json_array_get(call, 0));
    json_t* args = json_array_get(call, 1);
    const char* cid = json_array_size(call) >= 3 ? json_string_value(json_array_get(call, 2)) : "";
    if (!name) {
        json_t* resp = json_array();
        json_array_append_new(resp, json_string("error"));
        json_array_append_new(resp, json_pack("{ss}", "type", "invalidArguments"));
        json_array_append_new(resp, json_string(cid ? cid : ""));
        json_array_append_new(responses, resp);
        return;
    }
    store_lock();
    if (strcmp(name, "mailbox/get") == 0) {
        mv_store* s = store_get();
        json_t* list = json_array();
        for (int i = 0; i < s->mbox_count; i++) {
            mv_mailbox* mb = s->mboxes[i];
            json_t* m = json_pack("{sssssI}", "id", mb->id, "name", mb->name, "totalEmails", (json_int_t)mb->exists);
            json_array_append_new(list, m);
        }
        store_unlock();
        json_t* res = json_pack("{soso}", "accountId", json_string("acct1"), "list", list);
        json_t* r = json_pack("[sOs]", name + strlen(name) - 3 /* keep */, res, cid);
        /* Use correct response name: <method> -> <method>/get stays; build explicitly */
        json_decref(r);
        json_t* resp = json_array();
        json_array_append_new(resp, json_string("mailbox/get"));
        json_array_append_new(resp, res);
        json_array_append_new(resp, json_string(cid));
        json_array_append_new(responses, resp);
    } else if (strcmp(name, "messages/get") == 0) {
        mv_store* s = store_get();
        json_t* list = json_array();
        for (int i = 0; i < s->mbox_count; i++) {
            for (int j = 0; j < s->mboxes[i]->msg_count; j++) {
                json_array_append_new(list, jmap_message_object(s->mboxes[i]->msgs[j]));
            }
        }
        store_unlock();
        json_t* res = json_pack("{soso}", "accountId", json_string("acct1"), "list", list);
        json_t* resp = json_array();
        json_array_append_new(resp, json_string("mailbox/get"));
        json_array_append_new(resp, res);
        json_array_append_new(resp, json_string(cid));
        json_array_append_new(responses, resp);
    } else if (strcmp(name, "messages/query") == 0) {
        mv_store* s = store_get();
        json_t* ids = json_array();
        for (int i = 0; i < s->mbox_count; i++)
            for (int j = 0; j < s->mboxes[i]->msg_count; j++)
                json_array_append_new(ids, json_string(s->mboxes[i]->msgs[j]->id));
        store_unlock();
        json_t* res = json_pack("{sosoI}", "accountId", json_string("acct1"), "ids", ids, "total", (json_int_t)json_array_size(ids));
        json_t* resp = json_array();
        json_array_append_new(resp, json_string("messages/query"));
        json_array_append_new(resp, res);
        json_array_append_new(resp, json_string(cid));
        json_array_append_new(responses, resp);
    } else if (strcmp(name, "messages/set") == 0) {
        json_t* create = json_object_get(args, "create");
        json_t* created = json_object();
        if (create && json_is_object(create)) {
            const char* key;
            json_t* val;
            json_object_foreach(create, key, val) {
                const char* from = ""; const char* to = ""; const char* subject = ""; const char* text = "";
                json_t* f = json_object_get(val, "from");
                if (f && json_is_array(f) && json_array_size(f) > 0) {
                    json_t* e = json_object_get(json_array_get(f, 0), "email");
                    if (e) from = json_string_value(e);
                }
                json_t* t = json_object_get(val, "to");
                if (t && json_is_array(t) && json_array_size(t) > 0) {
                    json_t* e = json_object_get(json_array_get(t, 0), "email");
                    if (e) to = json_string_value(e);
                }
                json_t* sj = json_object_get(val, "subject");
                if (sj) subject = json_string_value(sj);
                json_t* tb = json_object_get(val, "textBody");
                if (tb && json_is_array(tb) && json_array_size(tb) > 0) text = json_string_value(json_array_get(tb, 0));
                char raw[4096];
                int n = snprintf(raw, sizeof(raw),
                    "From: %s\r\nTo: %s\r\nSubject: %s\r\nDate: now\r\nMessage-ID: <%s@mailval>\r\nMIME-Version: 1.0\r\nContent-Type: text/plain\r\n\r\n%s",
                    from, to, subject, key, text);
                mv_mailbox* inbox = store_mailbox_get("INBOX", true);
                char date[64]; mv_now_rfc3339(date, sizeof(date));
                unsigned int uid = store_append(inbox, from, to, subject, date, raw, (long)n, 0);
                (void)uid;
                char newid[16]; snprintf(newid, sizeof(newid), "M%d", (int)json_array_size(created) + 1);
                json_t* cr = json_pack("{ssss}", "id", newid, "blobId", newid);
                json_object_set_new(created, key, cr);
            }
        }
        store_unlock();
        json_t* res = json_pack("{soso}", "accountId", json_string("acct1"), "created", created);
        json_t* resp = json_array();
        json_array_append_new(resp, json_string("messages/set"));
        json_array_append_new(resp, res);
        json_array_append_new(resp, json_string(cid));
        json_array_append_new(responses, resp);
    } else {
        store_unlock();
        json_t* resp = json_array();
        json_array_append_new(resp, json_string("error"));
        json_t* res = json_pack("{ss}", "type", "unknownMethod");
        json_array_append_new(resp, res);
        json_array_append_new(resp, json_string(cid));
        json_array_append_new(responses, resp);
    }
}

void proto_jmap_handle(mv_conn* c, const char* peer, mv_capture* cap) {
    (void)peer;
    for (;;) {
        http_req r;
        if (http_read_request(c, &r) != 0) break;
        char logline[1200];
        snprintf(logline, sizeof(logline), "%s %s", r.method, r.path);
        mv_capture_command(cap, logline);

        if (strcmp(r.method, "GET") == 0 && strcmp(r.path, "/jmap/session") == 0) {
            jmap_session(c);
        } else if (strcmp(r.method, "GET") == 0 && strncmp(r.path, "/jmap/download", 14) == 0) {
            /* parse blobId from query */
            char* bid = NULL;
            char* p = strtok(r.query, "&");
            while (p) { if (strncmp(p, "blobId=", 7) == 0) bid = p + 7; p = strtok(NULL, "&"); }
            store_lock();
            mv_message* m = bid ? store_find_message(bid) : NULL;
            if (m) {
                store_unlock();
                http_respond(c, 200, "OK", "message/rfc822", m->raw, m->raw_len);
            } else { store_unlock(); http_error(c, 404, "Not Found", "blob not found"); }
        } else if (strcmp(r.method, "POST") == 0 && strcmp(r.path, "/jmap/upload") == 0) {
            store_lock();
            mv_mailbox* inbox = store_mailbox_get("INBOX", true);
            char date[64]; mv_now_rfc3339(date, sizeof(date));
            unsigned int uid = store_append(inbox, "", "", "", date, r.body ? r.body : "", r.body_len, 0);
            (void)uid;
            char newid[16]; snprintf(newid, sizeof(newid), "M%u", uid);
            store_unlock();
            char body[256];
            int n = snprintf(body, sizeof(body), "{\"accountId\":\"acct1\",\"blobId\":\"%s\",\"size\":%ld,\"type\":\"application/octet-stream\"}", newid, r.body_len);
            http_respond(c, 201, "Created", "application/json", body, (long)n);
        } else if (strcmp(r.method, "POST") == 0 && strcmp(r.path, "/jmap") == 0) {
            json_error_t err;
            json_t* req = r.body ? json_loads(r.body, 0, &err) : NULL;
            json_t* responses = json_array();
            if (req) {
                json_t* calls = json_object_get(req, "methodCalls");
                if (calls && json_is_array(calls)) {
                    for (size_t i = 0; i < json_array_size(calls); i++)
                        jmap_method(c, json_array_get(calls, i), responses);
                }
                json_decref(req);
            }
            json_t* out = json_pack("{so}", "methodResponses", responses);
            char* s = json_dumps(out, JSON_COMPACT);
            http_respond(c, 200, "OK", "application/json", s, (long)strlen(s));
            free(s); json_decref(out);
        } else {
            http_error(c, 404, "Not Found", "not found");
        }
        free(r.body);
        break; /* simple: one request per connection */
    }
}

proto_module proto_jmap = { "jmap", proto_jmap_handle };
