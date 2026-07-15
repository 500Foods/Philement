/*
 * Mail Relay producer implementation.
 *
 * Templated send (mailrelay_send_template) and freeform send
 * (mailrelay_send_direct) share enqueue/idempotency/default-from handling.
 * Used by REST (template only), Lua H.mail (template or freeform), Notify
 * compatibility, and system events. No synchronous SMTP delivery.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_internal.h>
#include <src/mailrelay/mailrelay_repository.h>

#include <stdlib.h>
#include <string.h>

#include <src/utils/utils_uuid.h>

void mailrelay_send_template_response_init(MailRelaySendTemplateResponse* resp) {
    if (!resp) {
        return;
    }
    resp->message_id = NULL;
    resp->status = NULL;
}

void mailrelay_send_template_response_free(MailRelaySendTemplateResponse* resp) {
    if (!resp) {
        return;
    }
    free(resp->message_id);
    free(resp->status);
    resp->message_id = NULL;
    resp->status = NULL;
}

/* Context shared between mailrelay_send_template and the idempotency callback. */
typedef struct IdempotencyContext {
    bool found;
    char* message_id;
    char* status;
    char error[256];
} IdempotencyContext;

/* Forward declaration for the idempotency callback. */
void idempotency_callback(MailRelayRepoResult* result, void* user_data);

/* Map a status_a63 value to a stable status string for callers. */
const char* status_from_a63(int status_a63) {
    switch (status_a63) {
        case 2:
            return "sent";
        case 3:
            return "failed";
        case 0:
        case 1:
        case 4:
        default:
            return "queued";
    }
}

/* Repository callback: capture an existing message for an idempotency key. */
void idempotency_callback(MailRelayRepoResult* result, void* user_data) {
    IdempotencyContext* ctx = (IdempotencyContext*)user_data;
    ctx->found = false;
    ctx->message_id = NULL;
    ctx->status = NULL;
    ctx->error[0] = '\0';

    if (!result || result->status != MAILRELAY_REPO_OK) {
        const char* msg = (result && result->error_message) ? result->error_message : "idempotency lookup failed";
        snprintf(ctx->error, sizeof(ctx->error), "%s", msg);
        return;
    }

    if (!result->data || !json_is_array(result->data) || json_array_size(result->data) == 0) {
        return; /* Not found is not an error; the caller will proceed normally. */
    }

    json_t* row = json_array_get(result->data, 0);
    if (!json_is_object(row)) {
        snprintf(ctx->error, sizeof(ctx->error), "repository returned unexpected idempotency data");
        return;
    }

    json_t* uuid_json = json_object_get(row, "message_uuid");
    json_t* status_json = json_object_get(row, "status_a63");
    if (!json_is_string(uuid_json) || !json_is_integer(status_json)) {
        snprintf(ctx->error, sizeof(ctx->error), "repository returned unexpected idempotency fields");
        return;
    }

    const char* uuid = json_string_value(uuid_json);
    int status_a63 = (int)json_integer_value(status_json);

    ctx->message_id = strdup(uuid);
    ctx->status = strdup(status_from_a63(status_a63));
    if (!ctx->message_id || !ctx->status) {
        free(ctx->message_id);
        free(ctx->status);
        ctx->message_id = NULL;
        ctx->status = NULL;
        snprintf(ctx->error, sizeof(ctx->error), "memory allocation failed");
        return;
    }
    ctx->found = true;
}

/* Add To/Cc/Bcc recipient arrays to the message, validating emails. */
bool add_recipients_arrays(MailRelayMessage* msg,
                                  const char* const* to, int to_count,
                                  const char* const* cc, int cc_count,
                                  const char* const* bcc, int bcc_count,
                                  char* err,
                                  size_t err_cap) {
    for (int i = 0; i < to_count; i++) {
        if (!mailrelay_is_valid_email(to[i])) {
            snprintf(err, err_cap, "MAIL_RECIPIENT_INVALID: invalid To address '%s'", to[i]);
            return false;
        }
        if (!mailrelay_message_add_to(msg, to[i])) {
            snprintf(err, err_cap, "MAIL_RECIPIENT_INVALID: failed to add To address '%s'", to[i]);
            return false;
        }
    }

    for (int i = 0; i < cc_count; i++) {
        if (!mailrelay_is_valid_email(cc[i])) {
            snprintf(err, err_cap, "MAIL_RECIPIENT_INVALID: invalid Cc address '%s'", cc[i]);
            return false;
        }
        if (!mailrelay_message_add_cc(msg, cc[i])) {
            snprintf(err, err_cap, "MAIL_RECIPIENT_INVALID: failed to add Cc address '%s'", cc[i]);
            return false;
        }
    }

    for (int i = 0; i < bcc_count; i++) {
        if (!mailrelay_is_valid_email(bcc[i])) {
            snprintf(err, err_cap, "MAIL_RECIPIENT_INVALID: invalid Bcc address '%s'", bcc[i]);
            return false;
        }
        if (!mailrelay_message_add_bcc(msg, bcc[i])) {
            snprintf(err, err_cap, "MAIL_RECIPIENT_INVALID: failed to add Bcc address '%s'", bcc[i]);
            return false;
        }
    }

    return true;
}

/* Shared runtime/enabled gate for producers. */
MailRelayStatus producer_check_runtime(char* err, size_t err_cap) {
    if (!mailrelay_runtime_is_initialized() ||
        (mailrelay_runtime && mailrelay_runtime->shutdown_requested)) {
        snprintf(err, err_cap, "MAIL_DISABLED: mail relay is not running");
        return MAILRELAY_SHUTDOWN;
    }
    if (app_config && !app_config->mail_relay.Enabled) {
        snprintf(err, err_cap, "MAIL_DISABLED: mail relay is disabled");
        return MAILRELAY_DISABLED;
    }
    return MAILRELAY_OK;
}

/* Shared idempotency short-circuit. Returns true if handled (found or error). */
bool producer_try_idempotency(const char* idempotency_key,
                                     MailRelaySendTemplateResponse* resp,
                                     char* err,
                                     size_t err_cap,
                                     MailRelayStatus* out_status) {
    if (!idempotency_key || idempotency_key[0] == '\0') {
        return false;
    }

    IdempotencyContext idem_ctx = { 0 };
    MailRelayRepoQueueGetByIdempotency idem_req = { idempotency_key };
    if (!mailrelay_repo_queue_get_by_idempotency(&idem_req, idempotency_callback, &idem_ctx)) {
        snprintf(err, err_cap, "idempotency lookup failed");
        *out_status = MAILRELAY_INVALID_ARGS;
        return true;
    }
    if (idem_ctx.error[0] != '\0') {
        snprintf(err, err_cap, "%s", idem_ctx.error);
        *out_status = MAILRELAY_INVALID_ARGS;
        return true;
    }
    if (idem_ctx.found) {
        resp->message_id = idem_ctx.message_id;
        resp->status = idem_ctx.status;
        *out_status = MAILRELAY_OK;
        return true;
    }
    return false;
}

/* Resolve From / Reply-To with config defaults. */
bool producer_resolve_from_reply(const char* req_from,
                                        const char* req_reply_to,
                                        const char** out_from,
                                        const char** out_reply_to,
                                        char* err,
                                        size_t err_cap) {
    const char* from = req_from;
    if (!from || from[0] == '\0') {
        if (app_config && app_config->mail_relay.DefaultFrom &&
            app_config->mail_relay.DefaultFrom[0] != '\0') {
            from = app_config->mail_relay.DefaultFrom;
        }
    }
    if (!from || from[0] == '\0') {
        snprintf(err, err_cap, "MAIL_PARAM_MISSING: from address is required");
        return false;
    }
    *out_from = from;

    const char* reply_to = req_reply_to;
    if (!reply_to || reply_to[0] == '\0') {
        if (app_config && app_config->mail_relay.DefaultReplyTo &&
            app_config->mail_relay.DefaultReplyTo[0] != '\0') {
            reply_to = app_config->mail_relay.DefaultReplyTo;
        }
    }
    *out_reply_to = reply_to;
    return true;
}

/* Enqueue a fully built message and fill the response. */
MailRelayStatus producer_enqueue_message(MailRelayMessage* msg,
                                                int priority,
                                                MailRelaySendTemplateResponse* resp,
                                                char* err,
                                                size_t err_cap) {
    char validate_err[256];
    if (!mailrelay_validate_message(msg, validate_err, sizeof(validate_err))) {
        mailrelay_message_free(msg);
        snprintf(err, err_cap, "MAIL_INVALID: %s", validate_err);
        return MAILRELAY_INVALID_ARGS;
    }

    MailRelayStatus status = mailrelay_enqueue(msg, priority);
    if (status != MAILRELAY_OK) {
        mailrelay_message_free(msg);
        if (status == MAILRELAY_QUEUE_FULL) {
            snprintf(err, err_cap, "MAIL_QUEUE_FULL: queue is at capacity");
        } else if (status == MAILRELAY_DISABLED) {
            snprintf(err, err_cap, "MAIL_DISABLED: mail relay is disabled");
        } else {
            snprintf(err, err_cap, "MAIL_QUEUE_ERROR: enqueue failed (status=%d)", (int)status);
        }
        return status;
    }

    resp->message_id = msg->message_id;
    msg->message_id = NULL;
    resp->status = strdup("queued");
    if (!resp->status) {
        free(resp->message_id);
        resp->message_id = NULL;
        mailrelay_message_free(msg);
        snprintf(err, err_cap, "memory allocation failed");
        return MAILRELAY_INVALID_ARGS;
    }

    mailrelay_message_free(msg);
    return MAILRELAY_OK;
}

/* Default producer implementation (bypasses the test seam). */
MailRelayStatus mailrelay_send_template_default(const MailRelaySendTemplateRequest* req,
                                                       MailRelaySendTemplateResponse* resp,
                                                       char* err,
                                                       size_t err_cap) {
    if (!req || !resp || !err || err_cap == 0) {
        if (err && err_cap > 0) {
            snprintf(err, err_cap, "invalid arguments");
        }
        return MAILRELAY_INVALID_ARGS;
    }
    mailrelay_send_template_response_init(resp);
    err[0] = '\0';

    if (!req->template_key || req->template_key[0] == '\0') {
        snprintf(err, err_cap, "MAIL_PARAM_MISSING: template_key is required");
        return MAILRELAY_INVALID_ARGS;
    }

    bool has_recipient = (req->to_count > 0 && req->to) ||
                         (req->cc_count > 0 && req->cc) ||
                         (req->bcc_count > 0 && req->bcc);
    if (!has_recipient) {
        snprintf(err, err_cap, "MAIL_RECIPIENT_INVALID: at least one recipient is required");
        return MAILRELAY_INVALID_ARGS;
    }

    MailRelayStatus runtime_status = producer_check_runtime(err, err_cap);
    if (runtime_status != MAILRELAY_OK) {
        return runtime_status;
    }

    MailRelayStatus idem_status = MAILRELAY_OK;
    if (producer_try_idempotency(req->idempotency_key, resp, err, err_cap, &idem_status)) {
        return idem_status;
    }

    char* subject = NULL;
    char* text_body = NULL;
    char* html_body = NULL;
    if (!mailrelay_template_preview(req->template_key, req->params,
                                    req->app_name, req->server_name, req->timestamp,
                                    req->request_id, req->user_email, req->otp_code,
                                    &subject, &text_body, &html_body, err, err_cap)) {
        return MAILRELAY_INVALID_ARGS;
    }

    const char* from = NULL;
    const char* reply_to = NULL;
    if (!producer_resolve_from_reply(req->from, req->reply_to, &from, &reply_to, err, err_cap)) {
        free(subject);
        free(text_body);
        free(html_body);
        return MAILRELAY_INVALID_ARGS;
    }

    MailRelayMessage msg;
    mailrelay_message_init(&msg);

    char uuid_str[UUID_STR_LEN];
    generate_uuid(uuid_str);
    msg.message_id = strdup(uuid_str);
    msg.subject = subject;
    msg.text_body = text_body;
    msg.html_body = html_body;
    msg.template_key = strdup(req->template_key);
    if (req->idempotency_key && req->idempotency_key[0] != '\0') {
        msg.idempotency_key = strdup(req->idempotency_key);
    }
    msg.priority = req->priority;

    if (!msg.message_id || !msg.template_key) {
        mailrelay_message_free(&msg);
        snprintf(err, err_cap, "memory allocation failed");
        return MAILRELAY_INVALID_ARGS;
    }

    if (!mailrelay_message_set_from(&msg, from)) {
        mailrelay_message_free(&msg);
        snprintf(err, err_cap, "MAIL_RECIPIENT_INVALID: from address is invalid");
        return MAILRELAY_INVALID_ARGS;
    }

    if (reply_to && reply_to[0] != '\0') {
        if (!mailrelay_message_set_reply_to(&msg, reply_to)) {
            mailrelay_message_free(&msg);
            snprintf(err, err_cap, "MAIL_RECIPIENT_INVALID: reply_to address is invalid");
            return MAILRELAY_INVALID_ARGS;
        }
    }

    if (!add_recipients_arrays(&msg, req->to, req->to_count, req->cc, req->cc_count,
                               req->bcc, req->bcc_count, err, err_cap)) {
        mailrelay_message_free(&msg);
        return MAILRELAY_INVALID_ARGS;
    }

    return producer_enqueue_message(&msg, req->priority, resp, err, err_cap);
}

static mailrelay_send_template_fn send_template_override = NULL;
static mailrelay_send_direct_fn send_direct_override = NULL;

void mailrelay_send_template_set_fn(mailrelay_send_template_fn fn) {
    send_template_override = fn;
}

MailRelayStatus mailrelay_send_template(const MailRelaySendTemplateRequest* req,
                                        MailRelaySendTemplateResponse* resp,
                                        char* err,
                                        size_t err_cap) {
    mailrelay_send_template_fn fn = send_template_override;
    if (!fn) {
        fn = mailrelay_send_template_default;
    }
    return fn(req, resp, err, err_cap);
}

/* Default freeform producer implementation (bypasses the test seam). */
MailRelayStatus mailrelay_send_direct_default(const MailRelaySendDirectRequest* req,
                                                     MailRelaySendTemplateResponse* resp,
                                                     char* err,
                                                     size_t err_cap) {
    if (!req || !resp || !err || err_cap == 0) {
        if (err && err_cap > 0) {
            snprintf(err, err_cap, "invalid arguments");
        }
        return MAILRELAY_INVALID_ARGS;
    }
    mailrelay_send_template_response_init(resp);
    err[0] = '\0';

    bool has_recipient = (req->to_count > 0 && req->to) ||
                         (req->cc_count > 0 && req->cc) ||
                         (req->bcc_count > 0 && req->bcc);
    if (!has_recipient) {
        snprintf(err, err_cap, "MAIL_RECIPIENT_INVALID: at least one recipient is required");
        return MAILRELAY_INVALID_ARGS;
    }

    if (!req->subject || req->subject[0] == '\0') {
        snprintf(err, err_cap, "MAIL_PARAM_MISSING: subject is required");
        return MAILRELAY_INVALID_ARGS;
    }

    bool has_text = (req->text_body && req->text_body[0] != '\0');
    bool has_html = (req->html_body && req->html_body[0] != '\0');
    if (!has_text && !has_html) {
        snprintf(err, err_cap, "MAIL_PARAM_MISSING: text_body or html_body is required");
        return MAILRELAY_INVALID_ARGS;
    }

    MailRelayStatus runtime_status = producer_check_runtime(err, err_cap);
    if (runtime_status != MAILRELAY_OK) {
        return runtime_status;
    }

    MailRelayStatus idem_status = MAILRELAY_OK;
    if (producer_try_idempotency(req->idempotency_key, resp, err, err_cap, &idem_status)) {
        return idem_status;
    }

    const char* from = NULL;
    const char* reply_to = NULL;
    if (!producer_resolve_from_reply(req->from, req->reply_to, &from, &reply_to, err, err_cap)) {
        return MAILRELAY_INVALID_ARGS;
    }

    MailRelayMessage msg;
    mailrelay_message_init(&msg);

    char uuid_str[UUID_STR_LEN];
    generate_uuid(uuid_str);
    msg.message_id = strdup(uuid_str);
    msg.subject = strdup(req->subject);
    if (has_text) {
        msg.text_body = strdup(req->text_body);
    }
    if (has_html) {
        msg.html_body = strdup(req->html_body);
    }
    if (req->idempotency_key && req->idempotency_key[0] != '\0') {
        msg.idempotency_key = strdup(req->idempotency_key);
    }
    msg.priority = req->priority;

    if (!msg.message_id || !msg.subject ||
        (has_text && !msg.text_body) ||
        (has_html && !msg.html_body)) {
        mailrelay_message_free(&msg);
        snprintf(err, err_cap, "memory allocation failed");
        return MAILRELAY_INVALID_ARGS;
    }

    if (!mailrelay_message_set_from(&msg, from)) {
        mailrelay_message_free(&msg);
        snprintf(err, err_cap, "MAIL_RECIPIENT_INVALID: from address is invalid");
        return MAILRELAY_INVALID_ARGS;
    }

    if (reply_to && reply_to[0] != '\0') {
        if (!mailrelay_message_set_reply_to(&msg, reply_to)) {
            mailrelay_message_free(&msg);
            snprintf(err, err_cap, "MAIL_RECIPIENT_INVALID: reply_to address is invalid");
            return MAILRELAY_INVALID_ARGS;
        }
    }

    if (!add_recipients_arrays(&msg, req->to, req->to_count, req->cc, req->cc_count,
                               req->bcc, req->bcc_count, err, err_cap)) {
        mailrelay_message_free(&msg);
        return MAILRELAY_INVALID_ARGS;
    }

    return producer_enqueue_message(&msg, req->priority, resp, err, err_cap);
}

void mailrelay_send_direct_set_fn(mailrelay_send_direct_fn fn) {
    send_direct_override = fn;
}

MailRelayStatus mailrelay_send_direct(const MailRelaySendDirectRequest* req,
                                      MailRelaySendTemplateResponse* resp,
                                      char* err,
                                      size_t err_cap) {
    mailrelay_send_direct_fn fn = send_direct_override;
    if (!fn) {
        fn = mailrelay_send_direct_default;
    }
    return fn(req, resp, err, err_cap);
}
