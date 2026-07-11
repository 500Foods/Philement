/*
 * Mail Relay OTP primitives: generate, hash, constant-time compare,
 * generate_and_send, verify.
 */

#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <openssl/crypto.h>
#include <openssl/sha.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_otp.h>
#include <src/mailrelay/mailrelay_repository.h>
#include <src/utils/utils_crypto.h>
#include <src/utils/utils_time.h>

static mailrelay_otp_random_fn g_otp_random_fn = NULL;

void mailrelay_otp_set_random_fn(mailrelay_otp_random_fn fn) {
    g_otp_random_fn = fn;
}

void mailrelay_otp_wipe(void* buf, size_t len) {
    if (buf != NULL && len > 0U) {
        OPENSSL_cleanse(buf, len);
    }
}

bool mailrelay_otp_generate_code(int digits, char* out, size_t out_size) {
    if (out == NULL || out_size == 0U) {
        return false;
    }
    if (digits < MAIL_OTP_MIN_DIGITS || digits > MAIL_OTP_MAX_DIGITS) {
        return false;
    }
    if (out_size < (size_t)digits + 1U) {
        return false;
    }

    unsigned char raw[MAIL_OTP_MAX_DIGITS];
    mailrelay_otp_random_fn random_fn = g_otp_random_fn;
    if (random_fn == NULL) {
        random_fn = utils_random_bytes;
    }
    if (!random_fn(raw, (size_t)digits)) {
        mailrelay_otp_wipe(raw, sizeof(raw));
        return false;
    }

    for (int i = 0; i < digits; i++) {
        out[i] = (char)('0' + (raw[i] % 10U));
    }
    out[digits] = '\0';
    mailrelay_otp_wipe(raw, sizeof(raw));
    return true;
}

bool mailrelay_otp_hash_code(const char* code, char* out, size_t out_size) {
    if (code == NULL || *code == '\0' || out == NULL) {
        return false;
    }
    if (out_size < MAIL_OTP_HASH_HEX_LEN) {
        return false;
    }

    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)code, strlen(code), digest);

    static const char hex[] = "0123456789abcdef";
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        out[i * 2]     = hex[(digest[i] >> 4) & 0x0f];
        out[i * 2 + 1] = hex[digest[i] & 0x0f];
    }
    out[SHA256_DIGEST_LENGTH * 2] = '\0';
    mailrelay_otp_wipe(digest, sizeof(digest));
    return true;
}

bool mailrelay_otp_constant_time_equal(const char* a, const char* b) {
    if (a == NULL || b == NULL) {
        return false;
    }

    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    if (len_a != len_b || len_a == 0U) {
        return false;
    }

    return CRYPTO_memcmp(a, b, len_a) == 0;
}

void mailrelay_otp_send_response_init(MailRelayOtpSendResponse* resp) {
    if (resp == NULL) {
        return;
    }
    resp->otp_id = 0;
    resp->message_id = NULL;
    resp->status = NULL;
}

void mailrelay_otp_send_response_free(MailRelayOtpSendResponse* resp) {
    if (resp == NULL) {
        return;
    }
    free(resp->message_id);
    free(resp->status);
    resp->message_id = NULL;
    resp->status = NULL;
    resp->otp_id = 0;
}

typedef struct OtpInsertCtx {
    bool ok;
    long long otp_id;
    char error[256];
} OtpInsertCtx;

static void otp_insert_callback(MailRelayRepoResult* result, void* user_data) {
    OtpInsertCtx* ctx = (OtpInsertCtx*)user_data;
    ctx->ok = false;
    ctx->otp_id = 0;
    ctx->error[0] = '\0';

    if (result == NULL || result->status != MAILRELAY_REPO_OK) {
        const char* msg = (result != NULL && result->error_message != NULL)
                              ? result->error_message
                              : "otp insert failed";
        snprintf(ctx->error, sizeof(ctx->error), "%s", msg);
        return;
    }

    ctx->ok = true;
    if (result->data != NULL && json_is_array(result->data) && json_array_size(result->data) > 0) {
        json_t* row = json_array_get(result->data, 0);
        if (json_is_object(row)) {
            json_t* id_json = json_object_get(row, "otp_id");
            if (json_is_integer(id_json)) {
                ctx->otp_id = json_integer_value(id_json);
            }
        }
    }
}

static int otp_resolve_digits(const MailRelayOtpSendRequest* req) {
    if (req->digits > 0) {
        return req->digits;
    }
    if (app_config != NULL && app_config->mail_relay.Otp.Digits > 0) {
        return app_config->mail_relay.Otp.Digits;
    }
    return MAIL_OTP_DEFAULT_DIGITS;
}

static int otp_resolve_expiry(const MailRelayOtpSendRequest* req) {
    if (req->expiry_seconds > 0) {
        return req->expiry_seconds;
    }
    if (app_config != NULL && app_config->mail_relay.Otp.ExpirySeconds > 0) {
        return app_config->mail_relay.Otp.ExpirySeconds;
    }
    return MAIL_OTP_DEFAULT_EXPIRY_SECONDS;
}

static int otp_resolve_max_attempts(const MailRelayOtpSendRequest* req) {
    if (req->max_attempts > 0) {
        return req->max_attempts;
    }
    if (app_config != NULL && app_config->mail_relay.Otp.MaxAttempts > 0) {
        return app_config->mail_relay.Otp.MaxAttempts;
    }
    return MAIL_OTP_DEFAULT_MAX_ATTEMPTS;
}

static bool otp_purpose_valid(int purpose) {
    return purpose == MAIL_OTP_PURPOSE_LOGIN_MFA
        || purpose == MAIL_OTP_PURPOSE_EMAIL_VERIFY
        || purpose == MAIL_OTP_PURPOSE_PASSWORD_RESET;
}

MailRelayStatus mailrelay_otp_generate_and_send(const MailRelayOtpSendRequest* req,
                                                MailRelayOtpSendResponse* resp,
                                                char* err,
                                                size_t err_cap) {
    if (err != NULL && err_cap > 0U) {
        err[0] = '\0';
    }
    if (req == NULL || resp == NULL) {
        if (err != NULL && err_cap > 0U) {
            snprintf(err, err_cap, "MAIL_PARAM_MISSING: null request or response");
        }
        return MAILRELAY_INVALID_ARGS;
    }

    mailrelay_otp_send_response_init(resp);

    if (app_config != NULL && !app_config->mail_relay.Enabled) {
        if (err != NULL && err_cap > 0U) {
            snprintf(err, err_cap, "MAIL_DISABLED: Mail Relay is disabled");
        }
        return MAILRELAY_DISABLED;
    }

    if (req->email == NULL || req->email[0] == '\0' || !mailrelay_is_valid_email(req->email)) {
        if (err != NULL && err_cap > 0U) {
            snprintf(err, err_cap, "MAIL_RECIPIENT_INVALID: invalid OTP email");
        }
        return MAILRELAY_INVALID_ARGS;
    }

    if (!otp_purpose_valid(req->purpose_a66)) {
        if (err != NULL && err_cap > 0U) {
            snprintf(err, err_cap, "MAIL_PARAM_MISSING: invalid OTP purpose");
        }
        return MAILRELAY_INVALID_ARGS;
    }

    int digits = otp_resolve_digits(req);
    int expiry_seconds = otp_resolve_expiry(req);
    int max_attempts = otp_resolve_max_attempts(req);

    if (digits < MAIL_OTP_MIN_DIGITS || digits > MAIL_OTP_MAX_DIGITS) {
        if (err != NULL && err_cap > 0U) {
            snprintf(err, err_cap, "MAIL_PARAM_MISSING: invalid OTP digits");
        }
        return MAILRELAY_INVALID_ARGS;
    }
    if (expiry_seconds < 1 || max_attempts < 1) {
        if (err != NULL && err_cap > 0U) {
            snprintf(err, err_cap, "MAIL_PARAM_MISSING: invalid OTP expiry or max_attempts");
        }
        return MAILRELAY_INVALID_ARGS;
    }

    char plaintext[MAIL_OTP_MAX_DIGITS + 1];
    memset(plaintext, 0, sizeof(plaintext));
    if (!mailrelay_otp_generate_code(digits, plaintext, sizeof(plaintext))) {
        if (err != NULL && err_cap > 0U) {
            snprintf(err, err_cap, "MAIL_INTERNAL: OTP generation failed");
        }
        mailrelay_otp_wipe(plaintext, sizeof(plaintext));
        return MAILRELAY_INVALID_ARGS;
    }

    char code_hash[MAIL_OTP_HASH_HEX_LEN];
    if (!mailrelay_otp_hash_code(plaintext, code_hash, sizeof(code_hash))) {
        if (err != NULL && err_cap > 0U) {
            snprintf(err, err_cap, "MAIL_INTERNAL: OTP hash failed");
        }
        mailrelay_otp_wipe(plaintext, sizeof(plaintext));
        mailrelay_otp_wipe(code_hash, sizeof(code_hash));
        return MAILRELAY_INVALID_ARGS;
    }

    time_t now = time(NULL);
    char expiry_at[64];
    format_iso_time(now + (time_t)expiry_seconds, expiry_at, sizeof(expiry_at));

    MailRelayRepoOtpInsert insert_params = {
        .code_hash = code_hash,
        .email = req->email,
        .account_id = req->account_id,
        .purpose_a66 = req->purpose_a66,
        .status_a67 = MAIL_OTP_STATUS_ACTIVE,
        .expiry_at = expiry_at,
        .max_attempts = max_attempts,
    };

    OtpInsertCtx insert_ctx;
    memset(&insert_ctx, 0, sizeof(insert_ctx));
    if (!mailrelay_repo_otp_insert(&insert_params, otp_insert_callback, &insert_ctx) || !insert_ctx.ok) {
        if (err != NULL && err_cap > 0U) {
            if (insert_ctx.error[0] != '\0') {
                snprintf(err, err_cap, "MAIL_PERSIST_FAILED: %s", insert_ctx.error);
            } else {
                snprintf(err, err_cap, "MAIL_PERSIST_FAILED: OTP insert failed");
            }
        }
        mailrelay_otp_wipe(plaintext, sizeof(plaintext));
        mailrelay_otp_wipe(code_hash, sizeof(code_hash));
        return MAILRELAY_PERSIST_FAILED;
    }

    const char* to_list[1];
    to_list[0] = req->email;

    char timestamp[64];
    format_iso_time(now, timestamp, sizeof(timestamp));

    const char* server_name = req->server_name;
    if ((server_name == NULL || server_name[0] == '\0') && app_config != NULL
        && app_config->server.server_name != NULL) {
        server_name = app_config->server.server_name;
    }

    MailRelaySendTemplateRequest send_req;
    memset(&send_req, 0, sizeof(send_req));
    send_req.template_key = MAIL_OTP_TEMPLATE_KEY;
    send_req.to = to_list;
    send_req.to_count = 1;
    send_req.from = req->from;
    send_req.params = NULL;
    send_req.priority = req->priority;
    send_req.app_name = req->app_name;
    send_req.server_name = server_name;
    send_req.timestamp = timestamp;
    send_req.request_id = req->request_id;
    send_req.user_email = req->email;
    send_req.otp_code = plaintext;

    MailRelaySendTemplateResponse send_resp;
    mailrelay_send_template_response_init(&send_resp);
    char send_err[256];
    send_err[0] = '\0';
    MailRelayStatus send_status = mailrelay_send_template(&send_req, &send_resp, send_err, sizeof(send_err));

    mailrelay_otp_wipe(plaintext, sizeof(plaintext));
    mailrelay_otp_wipe(code_hash, sizeof(code_hash));

    if (send_status != MAILRELAY_OK) {
        if (err != NULL && err_cap > 0U) {
            if (send_err[0] != '\0') {
                snprintf(err, err_cap, "%s", send_err);
            } else {
                snprintf(err, err_cap, "MAIL_INTERNAL: OTP mail enqueue failed");
            }
        }
        mailrelay_send_template_response_free(&send_resp);
        return send_status;
    }

    resp->otp_id = insert_ctx.otp_id;
    resp->message_id = send_resp.message_id;
    resp->status = send_resp.status;
    send_resp.message_id = NULL;
    send_resp.status = NULL;
    mailrelay_send_template_response_free(&send_resp);

    log_this(SR_MAIL_RELAY, "OTP queued for delivery purpose=%d", LOG_LEVEL_STATE, 1,
             req->purpose_a66);

    return MAILRELAY_OK;
}

void mailrelay_otp_verify_response_init(MailRelayOtpVerifyResponse* resp) {
    if (resp == NULL) {
        return;
    }
    resp->otp_id = 0;
    resp->account_id = 0;
}

typedef struct OtpGetActiveCtx {
    bool ok;
    bool found;
    long long otp_id;
    long long account_id;
    int attempts;
    int max_attempts;
    char code_hash[MAIL_OTP_HASH_HEX_LEN];
    char expiry_at[64];
    char error[256];
} OtpGetActiveCtx;

static long long otp_json_int64(json_t* obj, const char* key) {
    if (obj == NULL || key == NULL) {
        return 0;
    }
    json_t* v = json_object_get(obj, key);
    if (json_is_integer(v)) {
        return json_integer_value(v);
    }
    if (json_is_string(v)) {
        return atoll(json_string_value(v));
    }
    return 0;
}

static void otp_json_copy_string(json_t* obj, const char* key, char* out, size_t out_size) {
    if (out == NULL || out_size == 0U) {
        return;
    }
    out[0] = '\0';
    if (obj == NULL || key == NULL) {
        return;
    }
    json_t* v = json_object_get(obj, key);
    if (json_is_string(v)) {
        snprintf(out, out_size, "%s", json_string_value(v));
    }
}

static void otp_get_active_callback(MailRelayRepoResult* result, void* user_data) {
    OtpGetActiveCtx* ctx = (OtpGetActiveCtx*)user_data;
    ctx->ok = false;
    ctx->found = false;
    ctx->otp_id = 0;
    ctx->account_id = 0;
    ctx->attempts = 0;
    ctx->max_attempts = 0;
    ctx->code_hash[0] = '\0';
    ctx->expiry_at[0] = '\0';
    ctx->error[0] = '\0';

    if (result == NULL || result->status != MAILRELAY_REPO_OK) {
        const char* msg = (result != NULL && result->error_message != NULL)
                              ? result->error_message
                              : "otp get_active failed";
        snprintf(ctx->error, sizeof(ctx->error), "%s", msg);
        return;
    }

    ctx->ok = true;
    if (result->data == NULL || !json_is_array(result->data) || json_array_size(result->data) == 0) {
        return;
    }

    json_t* row = json_array_get(result->data, 0);
    if (!json_is_object(row)) {
        return;
    }

    ctx->found = true;
    ctx->otp_id = otp_json_int64(row, "otp_id");
    ctx->account_id = otp_json_int64(row, "account_id");
    ctx->attempts = (int)otp_json_int64(row, "attempts");
    ctx->max_attempts = (int)otp_json_int64(row, "max_attempts");
    otp_json_copy_string(row, "code_hash", ctx->code_hash, sizeof(ctx->code_hash));
    otp_json_copy_string(row, "expiry_at", ctx->expiry_at, sizeof(ctx->expiry_at));
}

typedef struct OtpWriteCtx {
    bool ok;
    char error[256];
} OtpWriteCtx;

static void otp_write_callback(MailRelayRepoResult* result, void* user_data) {
    OtpWriteCtx* ctx = (OtpWriteCtx*)user_data;
    ctx->ok = false;
    ctx->error[0] = '\0';

    if (result == NULL || result->status != MAILRELAY_REPO_OK) {
        const char* msg = (result != NULL && result->error_message != NULL)
                              ? result->error_message
                              : "otp write failed";
        snprintf(ctx->error, sizeof(ctx->error), "%s", msg);
        return;
    }
    ctx->ok = true;
}

/* Parse ISO-like timestamps written by format_iso_time (%Y-%m-%dT%H:%M:%SZ). */
static bool otp_parse_iso_time(const char* s, time_t* out) {
    if (s == NULL || *s == '\0' || out == NULL) {
        return false;
    }

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    if (sscanf(s, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) < 6
        && sscanf(s, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) < 6) {
        return false;
    }

    struct tm tm_utc;
    memset(&tm_utc, 0, sizeof(tm_utc));
    tm_utc.tm_year = year - 1900;
    tm_utc.tm_mon = month - 1;
    tm_utc.tm_mday = day;
    tm_utc.tm_hour = hour;
    tm_utc.tm_min = minute;
    tm_utc.tm_sec = second;
    tm_utc.tm_isdst = 0;

    time_t parsed = timegm(&tm_utc);
    if (parsed == (time_t)-1) {
        return false;
    }
    *out = parsed;
    return true;
}

static void otp_set_err(char* err, size_t err_cap, const char* msg) {
    if (err != NULL && err_cap > 0U && msg != NULL) {
        snprintf(err, err_cap, "%s", msg);
    }
}

MailRelayStatus mailrelay_otp_verify(const MailRelayOtpVerifyRequest* req,
                                     MailRelayOtpVerifyResponse* resp,
                                     char* err,
                                     size_t err_cap) {
    if (err != NULL && err_cap > 0U) {
        err[0] = '\0';
    }
    if (req == NULL || resp == NULL) {
        otp_set_err(err, err_cap, "MAIL_PARAM_MISSING: null request or response");
        return MAILRELAY_INVALID_ARGS;
    }

    mailrelay_otp_verify_response_init(resp);

    if (app_config != NULL && !app_config->mail_relay.Enabled) {
        otp_set_err(err, err_cap, "MAIL_DISABLED: Mail Relay is disabled");
        return MAILRELAY_DISABLED;
    }

    if (req->email == NULL || req->email[0] == '\0' || !mailrelay_is_valid_email(req->email)) {
        otp_set_err(err, err_cap, "MAIL_RECIPIENT_INVALID: invalid OTP email");
        return MAILRELAY_INVALID_ARGS;
    }

    if (!otp_purpose_valid(req->purpose_a66)) {
        otp_set_err(err, err_cap, "MAIL_PARAM_MISSING: invalid OTP purpose");
        return MAILRELAY_INVALID_ARGS;
    }

    if (req->code == NULL || req->code[0] == '\0') {
        otp_set_err(err, err_cap, "MAIL_PARAM_MISSING: OTP code required");
        return MAILRELAY_INVALID_ARGS;
    }

    MailRelayRepoOtpGetActive get_params = {
        .email = req->email,
        .purpose_a66 = req->purpose_a66,
    };
    OtpGetActiveCtx get_ctx;
    memset(&get_ctx, 0, sizeof(get_ctx));
    if (!mailrelay_repo_otp_get_active(&get_params, otp_get_active_callback, &get_ctx) || !get_ctx.ok) {
        if (err != NULL && err_cap > 0U) {
            if (get_ctx.error[0] != '\0') {
                snprintf(err, err_cap, "MAIL_PERSIST_FAILED: %s", get_ctx.error);
            } else {
                snprintf(err, err_cap, "MAIL_PERSIST_FAILED: OTP lookup failed");
            }
        }
        return MAILRELAY_PERSIST_FAILED;
    }

    if (!get_ctx.found) {
        otp_set_err(err, err_cap, "MAIL_OTP_NOT_FOUND: no active OTP");
        return MAILRELAY_INVALID_ARGS;
    }

    time_t expiry_ts = 0;
    if (get_ctx.expiry_at[0] != '\0' && otp_parse_iso_time(get_ctx.expiry_at, &expiry_ts)) {
        if (time(NULL) > expiry_ts) {
            mailrelay_otp_wipe(get_ctx.code_hash, sizeof(get_ctx.code_hash));
            otp_set_err(err, err_cap, "MAIL_OTP_EXPIRED: OTP has expired");
            return MAILRELAY_INVALID_ARGS;
        }
    }

    if (get_ctx.max_attempts > 0 && get_ctx.attempts >= get_ctx.max_attempts) {
        mailrelay_otp_wipe(get_ctx.code_hash, sizeof(get_ctx.code_hash));
        otp_set_err(err, err_cap, "MAIL_OTP_MAX_ATTEMPTS: too many attempts");
        return MAILRELAY_INVALID_ARGS;
    }

    char submitted_hash[MAIL_OTP_HASH_HEX_LEN];
    memset(submitted_hash, 0, sizeof(submitted_hash));
    if (!mailrelay_otp_hash_code(req->code, submitted_hash, sizeof(submitted_hash))) {
        mailrelay_otp_wipe(get_ctx.code_hash, sizeof(get_ctx.code_hash));
        mailrelay_otp_wipe(submitted_hash, sizeof(submitted_hash));
        otp_set_err(err, err_cap, "MAIL_PARAM_MISSING: invalid OTP code");
        return MAILRELAY_INVALID_ARGS;
    }

    bool match = mailrelay_otp_constant_time_equal(submitted_hash, get_ctx.code_hash);
    mailrelay_otp_wipe(submitted_hash, sizeof(submitted_hash));
    mailrelay_otp_wipe(get_ctx.code_hash, sizeof(get_ctx.code_hash));

    if (match) {
        MailRelayRepoOtpConsume consume_params = {
            .otp_id = get_ctx.otp_id,
        };
        OtpWriteCtx write_ctx;
        memset(&write_ctx, 0, sizeof(write_ctx));
        if (!mailrelay_repo_otp_consume(&consume_params, otp_write_callback, &write_ctx)
            || !write_ctx.ok) {
            if (err != NULL && err_cap > 0U) {
                if (write_ctx.error[0] != '\0') {
                    snprintf(err, err_cap, "MAIL_PERSIST_FAILED: %s", write_ctx.error);
                } else {
                    snprintf(err, err_cap, "MAIL_PERSIST_FAILED: OTP consume failed");
                }
            }
            return MAILRELAY_PERSIST_FAILED;
        }

        resp->otp_id = get_ctx.otp_id;
        resp->account_id = get_ctx.account_id;
        log_this(SR_MAIL_RELAY, "OTP verified purpose=%d", LOG_LEVEL_STATE, 1, req->purpose_a66);
        return MAILRELAY_OK;
    }

    int next_attempts = get_ctx.attempts + 1;
    bool at_max = (get_ctx.max_attempts > 0 && next_attempts >= get_ctx.max_attempts);
    OtpWriteCtx write_ctx;
    memset(&write_ctx, 0, sizeof(write_ctx));

    if (at_max) {
        MailRelayRepoOtpMarkMaxAttempts mark_params = {
            .otp_id = get_ctx.otp_id,
        };
        if (!mailrelay_repo_otp_mark_max_attempts(&mark_params, otp_write_callback, &write_ctx)
            || !write_ctx.ok) {
            if (err != NULL && err_cap > 0U) {
                if (write_ctx.error[0] != '\0') {
                    snprintf(err, err_cap, "MAIL_PERSIST_FAILED: %s", write_ctx.error);
                } else {
                    snprintf(err, err_cap, "MAIL_PERSIST_FAILED: OTP max-attempts update failed");
                }
            }
            return MAILRELAY_PERSIST_FAILED;
        }
        otp_set_err(err, err_cap, "MAIL_OTP_MAX_ATTEMPTS: too many attempts");
        return MAILRELAY_INVALID_ARGS;
    }

    MailRelayRepoOtpIncrementAttempts inc_params = {
        .otp_id = get_ctx.otp_id,
    };
    if (!mailrelay_repo_otp_increment_attempts(&inc_params, otp_write_callback, &write_ctx)
        || !write_ctx.ok) {
        if (err != NULL && err_cap > 0U) {
            if (write_ctx.error[0] != '\0') {
                snprintf(err, err_cap, "MAIL_PERSIST_FAILED: %s", write_ctx.error);
            } else {
                snprintf(err, err_cap, "MAIL_PERSIST_FAILED: OTP attempt increment failed");
            }
        }
        return MAILRELAY_PERSIST_FAILED;
    }

    otp_set_err(err, err_cap, "MAIL_OTP_INVALID: code mismatch");
    return MAILRELAY_INVALID_ARGS;
}
