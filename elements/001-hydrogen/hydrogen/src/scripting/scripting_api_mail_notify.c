/*
 * Scripting Subsystem - Host API: H.mail, H.notify
 *
 * H.mail.send queues mail through Mail Relay: template mode
 * (mailrelay_send_template) or freeform mode (mailrelay_send_direct).
 * H.notify remains a deferred-error compatibility stub.
 */

#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/mailrelay/mailrelay.h>
#include <src/utils/utils_uuid.h>

#include "scripting_api.h"
#include "scripting_handle.h"
#include "scripting_api_mail_notify.h"

#define MAIL_LUA_MAX_RECIPIENTS 256
#define MAIL_LUA_MAX_TEMPLATE_KEY_LEN 256
#define MAIL_LUA_MAX_IDEMPOTENCY_LEN 512
#define MAIL_LUA_MAX_SUBJECT_LEN 998
#define MAIL_LUA_MAX_BODY_LEN (1024 * 1024)
#define MAIL_LUA_ERR_CAP 512

/* Phase 7A.4: explicit deferred compatibility; no channel→template map yet. */
static const char* NOTIFY_DEFERRED_ERROR = "notify: deferred to mailrelay rules";

/* Owned parse buffers freed by free_mail_parse (struct defined in
 * scripting_api_mail_notify.h). */void free_mail_parse(MailLuaParse* p) {
    int i;

    if (!p) {
        return;
    }
    free(p->template_key);
    p->template_key = NULL;
    free(p->subject);
    p->subject = NULL;
    free(p->text_body);
    p->text_body = NULL;
    free(p->html_body);
    p->html_body = NULL;
    free(p->from);
    p->from = NULL;
    free(p->reply_to);
    p->reply_to = NULL;
    if (!p->idempotency_generated) {
        free(p->idempotency_key);
    }
    p->idempotency_key = NULL;
    p->idempotency_generated = false;
    if (p->to) {
        for (i = 0; i < p->to_count; i++) {
            free(p->to[i]);
        }
        free(p->to);
        p->to = NULL;
    }
    p->to_count = 0;
    if (p->cc) {
        for (i = 0; i < p->cc_count; i++) {
            free(p->cc[i]);
        }
        free(p->cc);
        p->cc = NULL;
    }
    p->cc_count = 0;
    if (p->bcc) {
        for (i = 0; i < p->bcc_count; i++) {
            free(p->bcc[i]);
        }
        free(p->bcc);
        p->bcc = NULL;
    }
    p->bcc_count = 0;
    mailrelay_template_params_free(&p->params);
}

void mail_parse_init(MailLuaParse* p) {
    memset(p, 0, sizeof(*p));
    mailrelay_template_params_init(&p->params);
}

/* Parse string or array of strings into an owned char** list. */
bool parse_recipient_field(lua_State* L, int table_idx, const char* field,
                                  char*** out_items, int* out_count,
                                  char* err, size_t err_cap) {
    int abs_idx;
    int t;
    int n;
    int i;
    char** items;

    *out_items = NULL;
    *out_count = 0;

    lua_getfield(L, table_idx, field);
    t = lua_type(L, -1);
    if (t == LUA_TNIL) {
        lua_pop(L, 1);
        return true;
    }
    if (t == LUA_TSTRING) {
        const char* s = lua_tostring(L, -1);
        items = calloc(1, sizeof(char*));
        if (!items || !s) {
            free(items);
            lua_pop(L, 1);
            snprintf(err, err_cap, "MAIL_PARAM_MISSING: field '%s' allocation failed", field);
            return false;
        }
        items[0] = strdup(s);
        if (!items[0]) {
            free(items);
            lua_pop(L, 1);
            snprintf(err, err_cap, "MAIL_PARAM_MISSING: field '%s' allocation failed", field);
            return false;
        }
        *out_items = items;
        *out_count = 1;
        lua_pop(L, 1);
        return true;
    }
    if (t != LUA_TTABLE) {
        lua_pop(L, 1);
        snprintf(err, err_cap, "MAIL_PARAM_MISSING: field '%s' must be a string or array", field);
        return false;
    }

    abs_idx = lua_gettop(L);
    n = (int)lua_rawlen(L, abs_idx);
    if (n == 0) {
        lua_pop(L, 1);
        return true;
    }
    if (n > MAIL_LUA_MAX_RECIPIENTS) {
        lua_pop(L, 1);
        snprintf(err, err_cap, "MAIL_PARAM_MISSING: field '%s' exceeds maximum of %d items",
                 field, MAIL_LUA_MAX_RECIPIENTS);
        return false;
    }

    items = calloc((size_t)n, sizeof(char*));
    if (!items) {
        lua_pop(L, 1);
        snprintf(err, err_cap, "MAIL_PARAM_MISSING: field '%s' allocation failed", field);
        return false;
    }

    for (i = 1; i <= n; i++) {
        lua_rawgeti(L, abs_idx, i);
        if (!lua_isstring(L, -1)) {
            int j;
            for (j = 0; j < i - 1; j++) {
                free(items[j]);
            }
            free(items);
            lua_pop(L, 2);
            snprintf(err, err_cap, "MAIL_PARAM_MISSING: field '%s' must contain only strings", field);
            return false;
        }
        items[i - 1] = strdup(lua_tostring(L, -1));
        if (!items[i - 1]) {
            int j;
            for (j = 0; j < i - 1; j++) {
                free(items[j]);
            }
            free(items);
            lua_pop(L, 2);
            snprintf(err, err_cap, "MAIL_PARAM_MISSING: field '%s' allocation failed", field);
            return false;
        }
        lua_pop(L, 1);
    }

    *out_items = items;
    *out_count = n;
    lua_pop(L, 1);
    return true;
}

bool parse_params_table(lua_State* L, int table_idx, MailRelayTemplateParams* params,
                               char* err, size_t err_cap) {
    lua_getfield(L, table_idx, "params");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return true;
    }
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        snprintf(err, err_cap, "MAIL_PARAM_MISSING: params must be a table of strings");
        return false;
    }

    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (!lua_isstring(L, -2) || !lua_isstring(L, -1)) {
            lua_pop(L, 3);
            snprintf(err, err_cap, "MAIL_PARAM_MISSING: params keys and values must be strings");
            return false;
        }
        if (!mailrelay_template_params_add(params, lua_tostring(L, -2), lua_tostring(L, -1))) {
            lua_pop(L, 3);
            snprintf(err, err_cap, "MAIL_PARAM_MISSING: failed to add parameter");
            return false;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return true;
}

/* Optional string field: copy non-empty string into *out (caller frees). */
bool parse_optional_string_field(lua_State* L, int table_idx, const char* field,
                                        char** out, size_t max_len,
                                        char* err, size_t err_cap) {
    *out = NULL;
    lua_getfield(L, table_idx, field);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return true;
    }
    if (!lua_isstring(L, -1)) {
        lua_pop(L, 1);
        snprintf(err, err_cap, "MAIL_PARAM_MISSING: field '%s' must be a string", field);
        return false;
    }
    {
        const char* s = lua_tostring(L, -1);
        size_t len = strlen(s);
        if (len == 0) {
            lua_pop(L, 1);
            return true;
        }
        if (len > max_len) {
            lua_pop(L, 1);
            snprintf(err, err_cap, "MAIL_PARAM_MISSING: field '%s' is too long", field);
            return false;
        }
        *out = strdup(s);
        lua_pop(L, 1);
        if (!*out) {
            snprintf(err, err_cap, "MAIL_PARAM_MISSING: allocation failed");
            return false;
        }
    }
    return true;
}

/*
 * Parse the Lua message table at stack index 1 into owned buffers.
 * Accepts either template mode (template/template_key) or freeform mode
 * (subject + text_body/html_body/body). Mixed mode is rejected.
 * Returns false with err filled on validation failure.
 */
bool parse_mail_message(lua_State* L, MailLuaParse* p) {
    bool has_template;
    bool has_freeform;

    if (!lua_istable(L, 1)) {
        snprintf(p->err, sizeof(p->err), "MAIL_PARAM_MISSING: message table required");
        return false;
    }

    lua_getfield(L, 1, "template");
    if (lua_isstring(L, -1) && lua_tostring(L, -1)[0] != '\0') {
        const char* tk = lua_tostring(L, -1);
        if (strlen(tk) > MAIL_LUA_MAX_TEMPLATE_KEY_LEN) {
            lua_pop(L, 1);
            snprintf(p->err, sizeof(p->err), "MAIL_PARAM_MISSING: template_key is too long");
            return false;
        }
        p->template_key = strdup(tk);
        lua_pop(L, 1);
        if (!p->template_key) {
            snprintf(p->err, sizeof(p->err), "MAIL_PARAM_MISSING: allocation failed");
            return false;
        }
    } else {
        lua_pop(L, 1);
        lua_getfield(L, 1, "template_key");
        if (lua_isstring(L, -1) && lua_tostring(L, -1)[0] != '\0') {
            const char* tk = lua_tostring(L, -1);
            if (strlen(tk) > MAIL_LUA_MAX_TEMPLATE_KEY_LEN) {
                lua_pop(L, 1);
                snprintf(p->err, sizeof(p->err), "MAIL_PARAM_MISSING: template_key is too long");
                return false;
            }
            p->template_key = strdup(tk);
            lua_pop(L, 1);
            if (!p->template_key) {
                snprintf(p->err, sizeof(p->err), "MAIL_PARAM_MISSING: allocation failed");
                return false;
            }
        } else {
            lua_pop(L, 1);
        }
    }

    if (!parse_optional_string_field(L, 1, "subject", &p->subject, MAIL_LUA_MAX_SUBJECT_LEN,
                                     p->err, sizeof(p->err))) {
        return false;
    }
    if (!parse_optional_string_field(L, 1, "text_body", &p->text_body, MAIL_LUA_MAX_BODY_LEN,
                                     p->err, sizeof(p->err))) {
        return false;
    }
    if (!parse_optional_string_field(L, 1, "html_body", &p->html_body, MAIL_LUA_MAX_BODY_LEN,
                                     p->err, sizeof(p->err))) {
        return false;
    }
    if (!p->text_body) {
        if (!parse_optional_string_field(L, 1, "body", &p->text_body, MAIL_LUA_MAX_BODY_LEN,
                                         p->err, sizeof(p->err))) {
            return false;
        }
    }
    if (!parse_optional_string_field(L, 1, "from", &p->from, MV_ADDR_LEN,
                                     p->err, sizeof(p->err))) {
        return false;
    }
    if (!parse_optional_string_field(L, 1, "reply_to", &p->reply_to, MV_ADDR_LEN,
                                     p->err, sizeof(p->err))) {
        return false;
    }

    has_template = (p->template_key != NULL);
    has_freeform = (p->subject != NULL) || (p->text_body != NULL) || (p->html_body != NULL);

    if (has_template && has_freeform) {
        snprintf(p->err, sizeof(p->err),
                 "MAIL_PARAM_MISSING: use either template or subject/body, not both");
        return false;
    }
    if (!has_template && !has_freeform) {
        snprintf(p->err, sizeof(p->err),
                 "MAIL_PARAM_MISSING: template or subject+body is required");
        return false;
    }
    if (has_freeform) {
        if (!p->subject) {
            snprintf(p->err, sizeof(p->err), "MAIL_PARAM_MISSING: subject is required");
            return false;
        }
        if (!p->text_body && !p->html_body) {
            snprintf(p->err, sizeof(p->err),
                     "MAIL_PARAM_MISSING: text_body or html_body is required");
            return false;
        }
    }

    lua_getfield(L, 1, "idempotency_key");
    if (lua_isstring(L, -1)) {
        const char* ik = lua_tostring(L, -1);
        if (strlen(ik) > MAIL_LUA_MAX_IDEMPOTENCY_LEN) {
            lua_pop(L, 1);
            snprintf(p->err, sizeof(p->err), "MAIL_PARAM_MISSING: idempotency_key is too long");
            return false;
        }
        if (ik[0] != '\0') {
            p->idempotency_key = strdup(ik);
            if (!p->idempotency_key) {
                lua_pop(L, 1);
                snprintf(p->err, sizeof(p->err), "MAIL_PARAM_MISSING: allocation failed");
                return false;
            }
        }
    }
    lua_pop(L, 1);

    if (!p->idempotency_key) {
        generate_uuid(p->idempotency_owned);
        p->idempotency_key = p->idempotency_owned;
        p->idempotency_generated = true;
    }

    if (!parse_recipient_field(L, 1, "to", &p->to, &p->to_count, p->err, sizeof(p->err))) {
        return false;
    }
    if (!parse_recipient_field(L, 1, "cc", &p->cc, &p->cc_count, p->err, sizeof(p->err))) {
        return false;
    }
    if (!parse_recipient_field(L, 1, "bcc", &p->bcc, &p->bcc_count, p->err, sizeof(p->err))) {
        return false;
    }
    if (p->to_count == 0 && p->cc_count == 0 && p->bcc_count == 0) {
        snprintf(p->err, sizeof(p->err),
                 "MAIL_RECIPIENT_INVALID: at least one recipient is required");
        return false;
    }

    if (has_template) {
        if (!parse_params_table(L, 1, &p->params, p->err, sizeof(p->err))) {
            return false;
        }
    }

    lua_getfield(L, 1, "priority");
    if (lua_isnumber(L, -1)) {
        p->priority = (int)lua_tointeger(L, -1);
    } else if (!lua_isnil(L, -1)) {
        lua_pop(L, 1);
        snprintf(p->err, sizeof(p->err), "MAIL_PARAM_MISSING: priority must be a number");
        return false;
    }
    lua_pop(L, 1);

    return true;
}

void status_to_mail_error(MailRelayStatus status, const char* producer_err,
                                 char* out, size_t out_cap) {
    if (producer_err && producer_err[0] != '\0') {
        snprintf(out, out_cap, "%s", producer_err);
        return;
    }
    switch (status) {
        case MAILRELAY_DISABLED:
            snprintf(out, out_cap, "MAIL_DISABLED: mail relay is disabled");
            break;
        case MAILRELAY_SHUTDOWN:
            snprintf(out, out_cap, "MAIL_DISABLED: mail relay is not running");
            break;
        case MAILRELAY_QUEUE_FULL:
            snprintf(out, out_cap, "MAIL_QUEUE_FULL: queue is at capacity");
            break;
        case MAILRELAY_OK:
        case MAILRELAY_INVALID_ARGS:
        case MAILRELAY_TIMEOUT:
        case MAILRELAY_PERSIST_FAILED:
        default:
            snprintf(out, out_cap, "MAIL_INTERNAL_ERROR: send failed (status=%d)", (int)status);
            break;
    }
}

/*
 * H.mail.send(message, opts?) -> handle
 *
 * Dual mode: template (template/template_key + optional params) or freeform
 * (subject + text_body/html_body/body). Optional idempotency_key auto UUID.
 */
int H_lua_mail_send(lua_State* L) {
    MailLuaParse parse;
    MailRelaySendTemplateResponse resp;
    char producer_err[MAIL_LUA_ERR_CAP];
    MailRelayStatus status;
    H_Handle* h;
    const char* const* to_ptrs = NULL;
    const char* const* cc_ptrs = NULL;
    const char* const* bcc_ptrs = NULL;

    h = H_Handle_new(L, H_HK_MAIL);
    if (!h) {
        return 0;
    }

    mail_parse_init(&parse);
    if (!parse_mail_message(L, &parse)) {
        h->mail_error = strdup(parse.err);
        free_mail_parse(&parse);
        return 1;
    }

    mailrelay_send_template_response_init(&resp);
    producer_err[0] = '\0';

    if (parse.to_count > 0) {
        to_ptrs = (const char* const*)parse.to;
    }
    if (parse.cc_count > 0) {
        cc_ptrs = (const char* const*)parse.cc;
    }
    if (parse.bcc_count > 0) {
        bcc_ptrs = (const char* const*)parse.bcc;
    }

    if (parse.template_key) {
        MailRelaySendTemplateRequest req;
        char ts_buf[32];
        time_t now;
        struct tm tm_buf;

        memset(&req, 0, sizeof(req));
        req.template_key = parse.template_key;
        req.to = to_ptrs;
        req.to_count = parse.to_count;
        req.cc = cc_ptrs;
        req.cc_count = parse.cc_count;
        req.bcc = bcc_ptrs;
        req.bcc_count = parse.bcc_count;
        req.params = &parse.params;
        req.idempotency_key = parse.idempotency_key;
        req.priority = parse.priority;
        req.from = parse.from;
        req.reply_to = parse.reply_to;
        req.app_name = (app_config && app_config->server.server_name)
                           ? app_config->server.server_name : "Hydrogen";
        req.server_name = req.app_name;
        now = time(NULL);
        if (gmtime_r(&now, &tm_buf) != NULL) {
            if (strftime(ts_buf, sizeof(ts_buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf) > 0) {
                req.timestamp = ts_buf;
            }
        }
        status = mailrelay_send_template(&req, &resp, producer_err, sizeof(producer_err));
    } else {
        MailRelaySendDirectRequest req;

        memset(&req, 0, sizeof(req));
        req.to = to_ptrs;
        req.to_count = parse.to_count;
        req.cc = cc_ptrs;
        req.cc_count = parse.cc_count;
        req.bcc = bcc_ptrs;
        req.bcc_count = parse.bcc_count;
        req.from = parse.from;
        req.reply_to = parse.reply_to;
        req.subject = parse.subject;
        req.text_body = parse.text_body;
        req.html_body = parse.html_body;
        req.idempotency_key = parse.idempotency_key;
        req.priority = parse.priority;
        status = mailrelay_send_direct(&req, &resp, producer_err, sizeof(producer_err));
    }

    if (status == MAILRELAY_OK) {
        h->mail_message_id = resp.message_id;
        h->mail_status = resp.status;
        resp.message_id = NULL;
        resp.status = NULL;
    } else {
        char mapped[MAIL_LUA_ERR_CAP];
        status_to_mail_error(status, producer_err, mapped, sizeof(mapped));
        h->mail_error = strdup(mapped);
    }

    mailrelay_send_template_response_free(&resp);
    free_mail_parse(&parse);
    return 1;
}

/*
 * H.mail.send_sync(message, opts?) -> result, err
 *
 * Submit + wait for queue accept only (not SMTP delivery).
 */
int H_lua_mail_send_sync(lua_State* L) {
    int n_pushed;
    H_Handle* h;
    int n;

    n_pushed = H_lua_mail_send(L);
    if (n_pushed == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "H.mail.send_sync: handle allocation failed");
        return 2;
    }
    h = H_Handle_check(L, -1);
    if (!h) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "H.mail.send_sync: handle creation failed");
        return 2;
    }
    n = H_lua_mail_notify_wait_one(L, h);
    lua_remove(L, -(n + 1));
    return n;
}

/*
 * H.notify.send(message, opts?) -> handle
 *
 * Phase 7A.4: stable deferred error (no silent success, no enqueue).
 * Real channel→template rule mapping is deferred.
 */
int H_lua_notify_send(lua_State* L) {
    H_Handle* h = H_Handle_new(L, H_HK_NOTIFY);
    if (!h) {
        return 0;
    }
    /* Message table is accepted for API stability; not interpreted in 7A.4. */
    h->notify_error = strdup(NOTIFY_DEFERRED_ERROR);
    return 1;
}

/*
 * H.notify.send_sync(message, opts?) -> result, err
 */
int H_lua_notify_send_sync(lua_State* L) {
    int n_pushed = H_lua_notify_send(L);
    H_Handle* h;
    int n;

    if (n_pushed == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "H.notify.send_sync: handle allocation failed");
        return 2;
    }
    h = H_Handle_check(L, -1);
    if (!h) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "H.notify.send_sync: handle creation failed");
        return 2;
    }
    n = H_lua_mail_notify_wait_one(L, h);
    lua_remove(L, -(n + 1));
    return n;
}

/*
 * Wait on a MAIL or NOTIFY handle.
 * Success: { message_id, status }, nil
 * Error:   nil, error_string
 */
int H_lua_mail_notify_wait_one(lua_State* L, H_Handle* h) {
    if (!h) {
        lua_pushnil(L);
        lua_pushstring(L, "H.wait: invalid handle");
        return 2;
    }
    if (h->consumed) {
        lua_pushnil(L);
        lua_pushstring(L, "H.wait: handle already consumed");
        return 2;
    }
    if (h->kind != H_HK_MAIL && h->kind != H_HK_NOTIFY) {
        lua_pushnil(L);
        lua_pushstring(L, "H.wait: handle is not a mail or notify handle");
        return 2;
    }

    if (h->kind == H_HK_NOTIFY) {
        const char* err = h->notify_error ? h->notify_error : NOTIFY_DEFERRED_ERROR;
        lua_pushnil(L);
        lua_pushstring(L, err);
        h->consumed = true;
        return 2;
    }

    if (h->mail_error) {
        lua_pushnil(L);
        lua_pushstring(L, h->mail_error);
        h->consumed = true;
        return 2;
    }

    if (!h->mail_message_id || !h->mail_status) {
        lua_pushnil(L);
        lua_pushstring(L, "MAIL_INTERNAL_ERROR: mail handle missing result");
        h->consumed = true;
        return 2;
    }

    lua_newtable(L);
    lua_pushstring(L, h->mail_message_id);
    lua_setfield(L, -2, "message_id");
    lua_pushstring(L, h->mail_status);
    lua_setfield(L, -2, "status");
    lua_pushnil(L);
    h->consumed = true;
    return 2;
}

void H_lua_install_mail_notify(lua_State* L) {
    if (!L) {
        return;
    }

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_mail_notify: H table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, -1, "mail");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
    }
    lua_pushcfunction(L, H_lua_mail_send);
    lua_setfield(L, -2, "send");
    lua_pushcfunction(L, H_lua_mail_send_sync);
    lua_setfield(L, -2, "send_sync");
    lua_setfield(L, -2, "mail");

    lua_getfield(L, -1, "notify");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
    }
    lua_pushcfunction(L, H_lua_notify_send);
    lua_setfield(L, -2, "send");
    lua_pushcfunction(L, H_lua_notify_send_sync);
    lua_setfield(L, -2, "send_sync");
    lua_setfield(L, -2, "notify");

    lua_pop(L, 1);
}
