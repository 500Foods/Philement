/*
 * Mail Relay libcurl SMTP/SMTPS send helper implementation.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay_smtp.h>
#include <src/mailrelay/mailrelay_render.h>
#include <src/mailrelay/mailrelay_result.h>
#include <src/mailrelay/mailrelay_retry.h>
#include <src/mailrelay/mailrelay_message.h>

#include <curl/curl.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Forward declarations (cppcheck: every function has a prototype). */
static int resolve_tls_mode(const OutboundServer* server);
static bool build_request(const MailRelayMessage* msg,
                          const OutboundServer* server,
                          const char* default_from,
                          const char* rendered,
                          MailRelaySmtpRequest* req);
static size_t smtp_read_cb(void* ptr, size_t size, size_t nmemb, void* userp);
static size_t smtp_write_cb(const void* ptr, size_t size, size_t nmemb, void* userp);
static long parse_smtp_code(const char* buf, size_t len, char* text_out, size_t text_cap);
static bool mailrelay_smtp_transport_real(const MailRelaySmtpRequest* req, MailRelayResult* out);

/* Active transport (swappable in tests). */
static mailrelay_smtp_transport_fn mailrelay_smtp_transport = mailrelay_smtp_transport_real;

static int resolve_tls_mode(const OutboundServer* server) {
    int mode = server->TLSMode;
    /* Legacy UseTLS boolean is only a backward-compatible hint: NONE + UseTLS
     * means "try STARTTLS", never implicit TLS. */
    if (mode == MAIL_TLS_MODE_NONE && server->UseTLS) {
        mode = MAIL_TLS_MODE_STARTTLS;
    }
    return mode;
}

static bool build_request(const MailRelayMessage* msg,
                          const OutboundServer* server,
                          const char* default_from,
                          const char* rendered,
                          MailRelaySmtpRequest* req) {
    memset(req, 0, sizeof(*req));

    int mode = resolve_tls_mode(server);
    req->tls_mode = mode;
    if (mode == MAIL_TLS_MODE_STARTTLS) {
        req->use_ssl = 1;
    } else if (mode == MAIL_TLS_MODE_STARTTLS_REQUIRED) {
        req->use_ssl = 2;
    }

    const char* scheme = (mode == MAIL_TLS_MODE_SMTPS) ? "smtps" : "smtp";
    const char* port = (server->Port && *server->Port) ? server->Port : "25";
    snprintf(req->url, sizeof(req->url), "%s://%s:%s",
             scheme, server->Host ? server->Host : "", port);

    const char* from = msg->from ? msg->from : (default_from ? default_from : "");
    strncpy(req->mail_from, from, sizeof(req->mail_from) - 1);
    req->mail_from[sizeof(req->mail_from) - 1] = '\0';

    int n = 0;
    for (int i = 0; i < msg->to_count && n < MV_MAX_RECIPIENTS; i++) {
        if (msg->to[i] && msg->to[i][0]) req->recipients[n++] = msg->to[i];
    }
    for (int i = 0; i < msg->cc_count && n < MV_MAX_RECIPIENTS; i++) {
        if (msg->cc[i] && msg->cc[i][0]) req->recipients[n++] = msg->cc[i];
    }
    for (int i = 0; i < msg->bcc_count && n < MV_MAX_RECIPIENTS; i++) {
        if (msg->bcc[i] && msg->bcc[i][0]) req->recipients[n++] = msg->bcc[i];
    }
    req->recipient_count = n;

    if (server->Username && *server->Username) {
        strncpy(req->username, server->Username, sizeof(req->username) - 1);
        req->username[sizeof(req->username) - 1] = '\0';
    }
    if (server->Password && *server->Password) {
        strncpy(req->password, server->Password, sizeof(req->password) - 1);
        req->password[sizeof(req->password) - 1] = '\0';
    }
    req->ca_path = server->CAPath;
    req->timeout_seconds = (server->TimeoutSeconds > 0) ? server->TimeoutSeconds : 30;
    req->auth_mode = server->AuthMode;
    req->payload = rendered;
    req->payload_len = rendered ? strlen(rendered) : 0;
    return true;
}

typedef struct {
    const char* data;
    size_t len;
    size_t pos;
} smtp_payload;

static size_t smtp_read_cb(void* ptr, size_t size, size_t nmemb, void* userp) {
    smtp_payload* p = (smtp_payload*)userp;
    size_t max = size * nmemb;
    if (p->pos >= p->len || max == 0) {
        return 0;
    }
    size_t n = p->len - p->pos;
    if (n > max) n = max;
    memcpy(ptr, p->data + p->pos, n);
    p->pos += n;
    return n;
}

typedef struct {
    char buf[8192];
    size_t len;
} smtp_resp;

static size_t smtp_write_cb(const void* ptr, size_t size, size_t nmemb, void* userp) {
    smtp_resp* r = (smtp_resp*)userp;
    size_t realsize = size * nmemb;
    if (r->len + realsize < sizeof(r->buf)) {
        memcpy(r->buf + r->len, ptr, realsize);
        r->len += realsize;
        r->buf[r->len] = '\0';
    }
    return realsize;
}

/* Find the last SMTP reply code (3 digits at the start of a line) and copy the
 * remainder of that line into text_out. Returns 0 if none found. */
static long parse_smtp_code(const char* buf, size_t len, char* text_out, size_t text_cap) {
    long code = 0;
    const char* line_start = buf;
    for (size_t i = 0; i <= len; i++) {
        bool eol = (i == len) || (buf[i] == '\n');
        if (eol) {
            size_t llen = i - (size_t)(line_start - buf);
            if (llen >= 3 &&
                isdigit((unsigned char)line_start[0]) &&
                isdigit((unsigned char)line_start[1]) &&
                isdigit((unsigned char)line_start[2])) {
                code = (line_start[0] - '0') * 100 +
                       (line_start[1] - '0') * 10 +
                       (line_start[2] - '0');
                if (text_out && llen > 4) {
                    size_t tl = llen - 4;
                    if (tl >= text_cap) tl = text_cap - 1;
                    memcpy(text_out, line_start + 4, tl);
                    text_out[tl] = '\0';
                }
            }
            line_start = buf + i + 1;
        }
    }
    return code;
}

static bool mailrelay_smtp_transport_real(const MailRelaySmtpRequest* req, MailRelayResult* out) {
    if (!req || !out) return false;
    mailrelay_result_init(out);

    CURL* curl = curl_easy_init();
    if (!curl) {
        snprintf(out->error, sizeof(out->error), "MAIL_SMTP_CURL_INIT_FAILED");
        return false;
    }

    struct curl_slist* rcpts = NULL;
    for (int i = 0; i < req->recipient_count; i++) {
        rcpts = curl_slist_append(rcpts, req->recipients[i]);
    }

    struct curl_slist* login = NULL;
    if (req->auth_mode == MAIL_AUTH_MODE_CRAM_MD5) {
        login = curl_slist_append(login, "CRAM-MD5");
    } else if (req->auth_mode == MAIL_AUTH_MODE_PLAIN) {
        login = curl_slist_append(login, "PLAIN");
    } else if (req->auth_mode == MAIL_AUTH_MODE_NTLM) {
        login = curl_slist_append(login, "NTLM");
    } else if (req->auth_mode == MAIL_AUTH_MODE_OAUTH2) {
        login = curl_slist_append(login, "XOAUTH2");
    } else {
        /* MAIL_AUTH_MODE_NONE: no SASL login options */
    }

    smtp_payload payload = { req->payload ? req->payload : "", req->payload_len, 0 };
    smtp_resp resp = {0};

    curl_easy_setopt(curl, CURLOPT_URL, req->url);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, req->mail_from);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, rcpts);
    if (req->username[0]) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, req->username);
        if (req->password[0]) curl_easy_setopt(curl, CURLOPT_PASSWORD, req->password);
    }
    if (login) curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, login->data);
    if (req->use_ssl == 1) {
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
    } else if (req->use_ssl == 2) {
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    }
    if (req->ca_path && *req->ca_path) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, req->ca_path);
    }
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, smtp_read_cb);
    curl_easy_setopt(curl, CURLOPT_READDATA, &payload);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE, (long)req->payload_len);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)req->timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)req->timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    /* SMTP server replies (220/250/354/221) are delivered to the header
     * callback, not the body write callback; capture both into resp. */
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, smtp_write_cb);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &resp);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, smtp_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    CURLcode res = curl_easy_perform(curl);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    out->duration_ms = (double)((t1.tv_sec - t0.tv_sec) * 1000) +
                       (double)(t1.tv_nsec - t0.tv_nsec) / 1e6;

    log_this(SR_MAIL_RELAY,
             "SMTP send %s from=%s recipients=%d tls=%d auth=%d -> curl=%d",
             LOG_LEVEL_STATE, 6,
             req->url, req->mail_from, req->recipient_count,
             req->tls_mode, req->auth_mode, (int)res);

    if (res != CURLE_OK) {
        out->success = false;
        snprintf(out->error, sizeof(out->error), "MAIL_SMTP_TRANSPORT: %s",
                 curl_easy_strerror(res));
        out->retryable = (res == CURLE_COULDNT_RESOLVE_HOST ||
                          res == CURLE_COULDNT_CONNECT ||
                          res == CURLE_OPERATION_TIMEDOUT ||
                          res == CURLE_SEND_ERROR ||
                          res == CURLE_RECV_ERROR ||
                          res == CURLE_GOT_NOTHING ||
                          res == CURLE_SSL_CONNECT_ERROR ||
                          res == CURLE_PARTIAL_FILE ||
                          res == CURLE_WRITE_ERROR ||
                          res == CURLE_READ_ERROR);
    } else {
        long code = parse_smtp_code(resp.buf, resp.len, out->smtp_text,
                                    sizeof(out->smtp_text));
        out->smtp_code = (int)code;
        out->success = (code >= 200 && code < 300);
        if (!out->success && out->smtp_text[0] == '\0') {
            snprintf(out->smtp_text, sizeof(out->smtp_text), "curl=%s",
                     curl_easy_strerror(res));
        }
        out->retryable = (code >= 400 && code < 500);
    }

    if (rcpts) curl_slist_free_all(rcpts);
    if (login) curl_slist_free_all(login);
    curl_easy_cleanup(curl);
    return out->success;
}

bool mailrelay_smtp_send(const MailRelayMessage* msg,
                         const OutboundServer* server,
                         const char* default_from,
                         const char* app_name,
                         MailRelayResult* out) {
    if (out) mailrelay_result_init(out);
    if (!msg || !server || !out) {
        if (out) snprintf(out->error, sizeof(out->error), "MAIL_SMTP_INVALID_ARGS");
        return false;
    }

    char* rendered = NULL;
    if (!mailrelay_render_message(msg, default_from, app_name, &rendered)) {
        snprintf(out->error, sizeof(out->error), "MAIL_SMTP_RENDER_FAILED");
        return false;
    }

    MailRelaySmtpRequest req;
    build_request(msg, server, default_from, rendered, &req);

    bool ok = mailrelay_smtp_transport(&req, out);

    free(rendered);
    return ok;
}

void mailrelay_smtp_set_transport(mailrelay_smtp_transport_fn fn) {
    mailrelay_smtp_transport = fn ? fn : mailrelay_smtp_transport_real;
}

void mailrelay_smtp_reset_transport(void) {
    mailrelay_smtp_transport = mailrelay_smtp_transport_real;
}
