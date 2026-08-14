/*
 * Conduit signed webhook handler (LUA_CLIENT Phase 14).
 */

#include <src/hydrogen.h>

#include <src/api/api_utils.h>
#include <src/api/conduit/script/script.h>
#include <src/utils/utils_crypto.h>

#include "webhook.h"

static conduit_webhook_submit_fn g_submit_hook = NULL;
static conduit_webhook_wait_fn g_wait_hook = NULL;
static conduit_webhook_send_json_fn g_send_json_hook = NULL;
static conduit_webhook_buffer_fn g_buffer_hook = NULL;

void conduit_webhook_set_submit_hook(conduit_webhook_submit_fn fn) {
    g_submit_hook = fn;
}

void conduit_webhook_set_wait_hook(conduit_webhook_wait_fn fn) {
    g_wait_hook = fn;
}

void conduit_webhook_set_send_json_hook(conduit_webhook_send_json_fn fn) {
    g_send_json_hook = fn;
}

void conduit_webhook_set_buffer_hook(conduit_webhook_buffer_fn fn) {
    g_buffer_hook = fn;
}

enum MHD_Result conduit_webhook_send_json(struct MHD_Connection *connection,
                                          json_t *json_obj,
                                          unsigned int status_code) {
    if (g_send_json_hook) {
        return g_send_json_hook(connection, json_obj, status_code);
    }
    return api_send_json_response(connection, json_obj, status_code);
}

char *conduit_webhook_extract_hook(const char *path) {
    const char *p = path;

    if (!path) {
        return NULL;
    }
    if (strncmp(path, "conduit/webhook/", 16) == 0) {
        p = path + 16;
    } else if (strncmp(path, "webhook/", 8) == 0) {
        p = path + 8;
    } else {
        return NULL;
    }
    if (!p[0] || strchr(p, '/')) {
        return NULL;
    }
    return strdup(p);
}

char *conduit_webhook_extract_hook_from_url(const char *url) {
    const char *p;

    if (!url) {
        return NULL;
    }
    p = strstr(url, "/webhook/");
    if (!p) {
        return NULL;
    }
    p += 9;
    if (!p[0] || strchr(p, '/')) {
        return NULL;
    }
    return strdup(p);
}

char *conduit_webhook_extract_kv(const char *header, const char *key) {
    const char *p;
    size_t key_len;
    char *out;
    size_t n;

    if (!header || !key) {
        return NULL;
    }
    key_len = strlen(key);
    p = header;
    while (p && *p) {
        while (*p == ' ' || *p == ',') {
            p++;
        }
        if (strncmp(p, key, key_len) == 0 && p[key_len] == '=') {
            p += key_len + 1;
            n = 0;
            while (p[n] && p[n] != ',' && p[n] != ' ') {
                n++;
            }
            if (n == 0) {
                return NULL;
            }
            out = malloc(n + 1);
            if (!out) {
                return NULL;
            }
            memcpy(out, p, n);
            out[n] = '\0';
            return out;
        }
        p = strchr(p, ',');
    }
    return NULL;
}

char *conduit_webhook_extract_timestamp(const char *header) {
    return conduit_webhook_extract_kv(header, "t");
}

char *conduit_webhook_extract_signature_token(const char *header) {
    char *v1;
    const char *p;
    const char *prefix = "sha256=";

    if (!header || !header[0]) {
        return NULL;
    }
    v1 = conduit_webhook_extract_kv(header, "v1");
    if (v1) {
        return v1;
    }
    p = header;
    while (*p == ' ') {
        p++;
    }
    if (strncmp(p, prefix, 7) == 0) {
        return strdup(p + 7);
    }
    return strdup(p);
}

char *conduit_webhook_bytes_to_hex(const unsigned char *bytes, size_t len) {
    static const char hex[] = "0123456789abcdef";
    char *out;
    size_t i;

    if (!bytes) {
        return NULL;
    }
    out = malloc(len * 2 + 1);
    if (!out) {
        return NULL;
    }
    for (i = 0; i < len; i++) {
        out[i * 2] = hex[(bytes[i] >> 4) & 0x0f];
        out[i * 2 + 1] = hex[bytes[i] & 0x0f];
    }
    out[len * 2] = '\0';
    return out;
}

bool conduit_webhook_hex_equal(const char *a, const char *b) {
    size_t i;
    size_t len;
    unsigned char diff;

    if (!a || !b) {
        return false;
    }
    len = strlen(a);
    if (len == 0 || len != strlen(b)) {
        return false;
    }
    diff = 0;
    for (i = 0; i < len; i++) {
        unsigned char ca = (unsigned char)tolower((unsigned char)a[i]);
        unsigned char cb = (unsigned char)tolower((unsigned char)b[i]);
        diff = (unsigned char)(diff | (ca ^ cb));
    }
    return diff == 0;
}

bool conduit_webhook_verify_hmac(const unsigned char *body, size_t body_len,
                                 const char *secret,
                                 const char *header_value,
                                 const char *hmac_algo) {
    const unsigned char *payload = body;
    size_t payload_len = body_len;
    char *signed_buf = NULL;
    char *token = NULL;
    unsigned char *mac = NULL;
    char *hex = NULL;
    unsigned int mac_len = 0;
    bool ok = false;
    const unsigned char empty = 0;

    if (!secret || !secret[0] || !header_value) {
        return false;
    }
    if (!payload || payload_len == 0) {
        payload = &empty;
        payload_len = 0;
    }

    if (hmac_algo && strcmp(hmac_algo, "sha256-timestamp") == 0) {
        char *ts = conduit_webhook_extract_timestamp(header_value);
        size_t ts_len;

        if (!ts) {
            return false;
        }
        ts_len = strlen(ts);
        signed_buf = malloc(ts_len + 1 + payload_len);
        if (!signed_buf) {
            free(ts);
            return false;
        }
        memcpy(signed_buf, ts, ts_len);
        signed_buf[ts_len] = '.';
        if (payload_len > 0) {
            memcpy(signed_buf + ts_len + 1, payload, payload_len);
        }
        payload = (const unsigned char *)signed_buf;
        payload_len = ts_len + 1 + payload_len;
        free(ts);
    }

    token = conduit_webhook_extract_signature_token(header_value);
    if (!token) {
        free(signed_buf);
        return false;
    }

    mac = utils_hmac_sha256(payload, payload_len, secret, strlen(secret),
                            &mac_len);
    if (!mac || mac_len == 0) {
        free(token);
        free(signed_buf);
        free(mac);
        return false;
    }

    hex = conduit_webhook_bytes_to_hex(mac, mac_len);
    ok = conduit_webhook_hex_equal(hex, token);

    free(hex);
    free(mac);
    free(token);
    free(signed_buf);
    return ok;
}

char *conduit_webhook_build_params_json(const char *hook,
                                        const unsigned char *body,
                                        size_t body_len,
                                        const char *content_type,
                                        json_t *headers) {
    json_t *root;
    json_t *body_js;
    char *dumped;

    root = json_object();
    if (!root) {
        return NULL;
    }
    json_object_set_new(root, "hook", json_string(hook ? hook : ""));
    if (body && body_len > 0) {
        body_js = json_stringn((const char *)body, body_len);
    } else {
        body_js = json_string("");
    }
    if (!body_js) {
        json_decref(root);
        return NULL;
    }
    json_object_set_new(root, "body", body_js);
    if (headers && json_is_object(headers)) {
        json_object_set(root, "headers", headers);
    } else {
        json_object_set_new(root, "headers", json_object());
    }
    json_object_set_new(root, "content_type",
                        json_string(content_type ? content_type : ""));
    dumped = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    return dumped;
}

json_t *conduit_webhook_error_json(const char *error_code, const char *message) {
    json_t *obj = json_object();

    if (!obj) {
        return NULL;
    }
    json_object_set_new(obj, "success", json_false());
    json_object_set_new(obj, "error",
                        json_string(error_code ? error_code : "error"));
    json_object_set_new(obj, "message",
                        json_string(message ? message : ""));
    return obj;
}

enum MHD_Result send_webhook_error(struct MHD_Connection *connection,
                                   const char *error_code,
                                   const char *message,
                                   unsigned int http_status) {
    json_t *obj = conduit_webhook_error_json(error_code, message);

    if (!obj) {
        return MHD_NO;
    }
    return conduit_webhook_send_json(connection, obj, http_status);
}

json_t *conduit_webhook_collect_headers(struct MHD_Connection *connection,
                                        const char *signature_header) {
    json_t *headers;
    const char *names[] = {
        "Content-Type",
        "User-Agent",
        "X-Request-Id",
        NULL
    };
    size_t i;
    const char *val;

    headers = json_object();
    if (!headers) {
        return NULL;
    }
    for (i = 0; names[i]; i++) {
        val = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, names[i]);
        if (val) {
            json_object_set_new(headers, names[i], json_string(val));
        }
    }
    if (signature_header && signature_header[0]) {
        val = MHD_lookup_connection_value(connection, MHD_HEADER_KIND,
                                          signature_header);
        if (val) {
            json_object_set_new(headers, signature_header, json_string(val));
        }
    }
    return headers;
}

enum MHD_Result handle_webhook_post(struct MHD_Connection *connection,
                                    const char *hook_name,
                                    const char *upload_data,
                                    size_t *upload_data_size,
                                    void **con_cls) {
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result;
    const WebhookHook *hook;
    const char *sig_header;
    const char *secret;
    const char *content_type;
    json_t *headers = NULL;
    char *params_json = NULL;
    char *job_id = NULL;
    ScriptingInvokeError inv_err;
    int timeout_s = CONDUIT_WEBHOOK_TIMEOUT_DEFAULT_S;

    if (g_buffer_hook) {
        buf_result = g_buffer_hook("POST", upload_data, upload_data_size,
                                   con_cls, &buffer);
    } else {
        buf_result = api_buffer_post_data("POST", upload_data, upload_data_size,
                                          con_cls, &buffer);
    }
    switch (buf_result) {
        case API_BUFFER_CONTINUE:
            return MHD_YES;
        case API_BUFFER_ERROR:
            return api_send_error_and_cleanup(connection, con_cls,
                                              "Request processing error",
                                              MHD_HTTP_INTERNAL_SERVER_ERROR);
        case API_BUFFER_METHOD_ERROR:
            api_free_post_buffer(con_cls);
            return send_webhook_error(connection, "method_not_allowed",
                                      "Use POST for /api/conduit/webhook/{hook}",
                                      MHD_HTTP_METHOD_NOT_ALLOWED);
        case API_BUFFER_COMPLETE:
            break;
        default:
            api_free_post_buffer(con_cls);
            return send_webhook_error(connection, "internal_error",
                                      "Unexpected buffer state",
                                      MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    if (!app_config || !app_config->webhooks.Enabled) {
        api_free_post_buffer(con_cls);
        return send_webhook_error(connection, "webhooks_disabled",
                                  "Webhooks are not enabled",
                                  MHD_HTTP_SERVICE_UNAVAILABLE);
    }

    if (!conduit_script_is_enabled()) {
        api_free_post_buffer(con_cls);
        return send_webhook_error(connection, "scripting_disabled",
                                  "Scripting subsystem is not enabled",
                                  MHD_HTTP_SERVICE_UNAVAILABLE);
    }

    hook = webhooks_find_hook(&app_config->webhooks, hook_name);
    if (!hook) {
        api_free_post_buffer(con_cls);
        return send_webhook_error(connection, "hook_not_found",
                                  "Unknown webhook",
                                  MHD_HTTP_NOT_FOUND);
    }

    sig_header = hook->SignatureHeader ? hook->SignatureHeader : "";
    secret = hook->SecretEnv ? getenv(hook->SecretEnv) : NULL;
    if (!secret || !secret[0] || !sig_header[0]) {
        api_free_post_buffer(con_cls);
        return send_webhook_error(connection, "invalid_signature",
                                  "Webhook signature verification failed",
                                  MHD_HTTP_UNAUTHORIZED);
    }

    {
        const char *provided = MHD_lookup_connection_value(
            connection, MHD_HEADER_KIND, sig_header);
        const unsigned char *body =
            buffer && buffer->data ? (const unsigned char *)buffer->data : NULL;
        size_t body_len = buffer ? buffer->size : 0;

        if (!provided ||
            !conduit_webhook_verify_hmac(body, body_len, secret, provided,
                                         hook->Hmac)) {
            api_free_post_buffer(con_cls);
            return send_webhook_error(connection, "invalid_signature",
                                      "Webhook signature verification failed",
                                      MHD_HTTP_UNAUTHORIZED);
        }

        content_type = MHD_lookup_connection_value(
            connection, MHD_HEADER_KIND, "Content-Type");
        headers = conduit_webhook_collect_headers(connection, sig_header);
        params_json = conduit_webhook_build_params_json(
            hook_name, body, body_len, content_type, headers);
        json_decref(headers);
    }
    api_free_post_buffer(con_cls);

    if (!params_json) {
        return send_webhook_error(connection, "internal_error",
                                  "Failed to build webhook params",
                                  MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    if (strstr(params_json, "\"_hydrogen\"")) {
        free(params_json);
        return send_webhook_error(connection, "internal_error",
                                  "Reserved params leaked",
                                  MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    log_this(SR_API, "conduit/webhook invoke start hook=%s script=%s",
             LOG_LEVEL_STATE, 2, hook_name, hook->Script);

    if (g_submit_hook) {
        inv_err = g_submit_hook(hook->Script, params_json, NULL, timeout_s,
                                &job_id);
    } else {
        inv_err = scripting_submit_job_from_db(hook->Script, params_json, NULL,
                                               timeout_s, &job_id);
    }
    free(params_json);

    if (inv_err != SCRIPTING_INVOKE_OK || !job_id) {
        unsigned int http_st = MHD_HTTP_INTERNAL_SERVER_ERROR;
        const char *code = NULL;
        const char *msg = NULL;
        conduit_script_map_invoke_error(inv_err, &http_st, &code, &msg);
        free(job_id);
        return send_webhook_error(connection, code, msg, http_st);
    }

    {
        ScoreboardEntry *entry = NULL;
        ScriptingWaitResult wr;
        const char *status_str;
        json_t *resp;

        if (g_wait_hook) {
            wr = g_wait_hook(job_id, timeout_s, &entry);
        } else {
            wr = scripting_wait_job(job_id, timeout_s, &entry);
        }
        status_str = conduit_script_wait_status_name(wr);
        if (wr == SCRIPTING_WAIT_SHUTDOWN) {
            scoreboard_entry_free(entry);
            free(job_id);
            return send_webhook_error(connection, "scripting_shutdown",
                                      "Scripting subsystem is shutting down",
                                      MHD_HTTP_SERVICE_UNAVAILABLE);
        }
        if (wr == SCRIPTING_WAIT_NOT_FOUND || wr == SCRIPTING_WAIT_INTERNAL) {
            scoreboard_entry_free(entry);
            free(job_id);
            return send_webhook_error(
                connection,
                wr == SCRIPTING_WAIT_NOT_FOUND ? "job_not_found" : "internal_error",
                "Webhook script wait failed",
                wr == SCRIPTING_WAIT_NOT_FOUND ? MHD_HTTP_NOT_FOUND
                                               : MHD_HTTP_INTERNAL_SERVER_ERROR);
        }
        resp = conduit_script_build_job_response(status_str, job_id,
                                                 hook->Script, entry, 0);
        scoreboard_entry_free(entry);
        free(job_id);
        if (!resp) {
            return send_webhook_error(connection, "internal_error",
                                      "Failed to build response",
                                      MHD_HTTP_INTERNAL_SERVER_ERROR);
        }
        log_this(SR_API, "conduit/webhook invoke end hook=%s script=%s status=%s",
                 LOG_LEVEL_STATE, 3, hook_name, hook->Script, status_str);
        return conduit_webhook_send_json(connection, resp, MHD_HTTP_OK);
    }
}

enum MHD_Result handle_conduit_webhook_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls,
    const char *path) {
    char *hook = NULL;
    enum MHD_Result r;

    if (!connection || !method) {
        return MHD_NO;
    }

    hook = conduit_webhook_extract_hook(path);
    if (!hook) {
        hook = conduit_webhook_extract_hook_from_url(url);
    }
    if (!hook) {
        return send_webhook_error(connection, "hook_not_found",
                                  "Unknown webhook",
                                  MHD_HTTP_NOT_FOUND);
    }

    if (strcmp(method, "POST") != 0) {
        free(hook);
        return send_webhook_error(connection, "method_not_allowed",
                                  "Use POST for /api/conduit/webhook/{hook}",
                                  MHD_HTTP_METHOD_NOT_ALLOWED);
    }

    if (!upload_data_size || !con_cls) {
        free(hook);
        return MHD_NO;
    }

    r = handle_webhook_post(connection, hook, upload_data, upload_data_size,
                            con_cls);
    free(hook);
    return r;
}
