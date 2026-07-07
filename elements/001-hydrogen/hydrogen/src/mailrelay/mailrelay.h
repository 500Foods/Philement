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

#include <src/config/config_mail_relay.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_queue.h>
#include <src/mailrelay/mailrelay_result.h>
#include <src/mailrelay/mailrelay_test_seams.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_H */
