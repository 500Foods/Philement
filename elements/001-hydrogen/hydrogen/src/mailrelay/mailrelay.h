/*
 * Mail Relay public internal API.
 *
 * This is the stable producer surface used by the launch trigger, REST
 * endpoints, Lua H.mail, Notify compatibility, and system events. Phase 2
 * implements a thin wrapper around the SMTP transport; Phase 3+ re-routes
 * this through the queue/worker path without changing the signature.
 */

#ifndef MAILRELAY_H
#define MAILRELAY_H

#include <stdbool.h>
#include <time.h>

#include <src/config/config_mail_relay.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_queue.h>
#include <src/mailrelay/mailrelay_result.h>
#include <src/mailrelay/mailrelay_template.h>
#include <src/mailrelay/mailrelay_test_seams.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Snapshot of Mail Relay subsystem counters and health.
 */
typedef struct MailRelayStatusCounters {
    bool enabled;                  /**< Mail Relay enabled in configuration. */
    bool initialized;              /**< Runtime is initialized. */
    size_t queued;                 /**< Lifetime queued messages. */
    size_t sending;                /**< Messages currently being sent. */
    size_t sent;                   /**< Lifetime successful sends. */
    size_t failed;                 /**< Lifetime failed sends. */
    size_t retrying;               /**< Messages scheduled for future retry. */
    size_t permanent_failures;     /**< Messages that exhausted retry attempts. */
    time_t last_success;           /**< Unix timestamp of last successful send. */
    time_t last_failure;           /**< Unix timestamp of last failed send. */
    int worker_count;              /**< Active worker threads. */
    int queue_depth;               /**< Current in-memory queue size. */
} MailRelayStatusCounters;

/*
 * Fill a counter snapshot from the current Mail Relay runtime.
 *
 * @param counters Out: snapshot of current counters. Must not be NULL.
 * @return true if the runtime is initialized and counters were captured.
 */
bool mailrelay_get_status(MailRelayStatusCounters* counters);

/*
 * Send one message to a single outbound server without queueing.
 *
 * This is the seed producer API. Phase 3 will add queue/retry/debounce
 * around this call site while preserving the signature so callers do not
 * change.
 */
bool mailrelay_send_raw(const MailRelayMessage* msg,
                        const OutboundServer* server,
                        const char* default_from,
                        const char* app_name,
                        MailRelayResult* out);

/*
 * Enqueue a message for asynchronous delivery by worker threads.
 *
 * This is the stable internal producer API used by launch triggers, REST
 * endpoints, Lua H.mail, Notify compatibility, and system events. It
 * validates the message, deep-copies it into the queue, and returns a status
 * code. Callers do not block on SMTP.
 */
MailRelayStatus mailrelay_enqueue(const MailRelayMessage* msg, int priority);

/*
 * Minimal lifecycle initializer. Phase 2 only resets seams; Phases 3+ add
 * queue/worker/thread tracking.
 */
bool mailrelay_init(void);

/*
 * Shutdown the Mail Relay runtime.
 *
 * Signals any worker threads to stop, drains tracked threads with a timeout,
 * and frees the runtime instance. Safe to call when not initialized.
 */
void mailrelay_shutdown(void);

/*
 * Request to send a templated email through the queue.
 */
typedef struct MailRelaySendTemplateRequest {
    const char* template_key;             /**< Template key to render and send. */
    const char* const* to;                /**< Array of To recipient addresses. */
    int to_count;                         /**< Number of To addresses. */
    const char* const* cc;                /**< Array of Cc recipient addresses. */
    int cc_count;                         /**< Number of Cc addresses. */
    const char* const* bcc;               /**< Array of Bcc recipient addresses. */
    int bcc_count;                        /**< Number of Bcc addresses. */
    const char* from;                     /**< Override From; NULL uses MailRelay.DefaultFrom. */
    const char* reply_to;                 /**< Override Reply-To; NULL uses MailRelay.DefaultReplyTo. */
    const MailRelayTemplateParams* params; /**< User macro parameters. */
    const char* idempotency_key;          /**< Optional idempotency key. */
    int priority;                         /**< Queue priority (higher dequeues first). */
    const char* app_name;                 /**< Value for %APP_NAME%. */
    const char* server_name;              /**< Value for %SERVER_NAME%. */
    const char* timestamp;                /**< Value for %TIMESTAMP%. */
    const char* request_id;               /**< Value for %REQUEST_ID%. */
    const char* user_email;               /**< Value for %USER_EMAIL%. */
    const char* otp_code;                 /**< Value for %OTP_CODE%. */
} MailRelaySendTemplateRequest;

/*
 * Response from a successful templated send.
 */
typedef struct MailRelaySendTemplateResponse {
    char* message_id;  /**< Caller-owned stable message identifier. */
    char* status;      /**< Caller-owned status string (e.g. "queued"). */
} MailRelaySendTemplateResponse;

/* Initialize a response to a clean default state. */
void mailrelay_send_template_response_init(MailRelaySendTemplateResponse* resp);

/* Free a response and reset it to a clean default state. */
void mailrelay_send_template_response_free(MailRelaySendTemplateResponse* resp);

/*
 * Resolve a template, render it, build a message, and enqueue it for
 * asynchronous delivery. This is the common internal producer API used by
 * REST, Lua, Notify, and system event callers.
 *
 * @param req  Send request.
 * @param resp Out: message_id and status on success. Must be initialized and
 *             freed by the caller.
 * @param err  Buffer for error message on failure.
 * @param err_cap Capacity of err buffer.
 * @return MAILRELAY_OK on success, MAILRELAY_INVALID_ARGS on validation/render
 *         failure, MAILRELAY_DISABLED if Mail Relay is disabled, or other
 *         MAILRELAY_* status codes.
 */
MailRelayStatus mailrelay_send_template(const MailRelaySendTemplateRequest* req,
                                        MailRelaySendTemplateResponse* resp,
                                        char* err,
                                        size_t err_cap);

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_H */
