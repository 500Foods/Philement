/*
 * Mail Relay message model and validation.
 */

#ifndef MAILRELAY_MESSAGE_H
#define MAILRELAY_MESSAGE_H

#include <stdbool.h>

#define MV_MAX_RECIPIENTS 256
#define MV_ADDR_LEN 256
#define MV_MAX_SUBJECT_LEN 998
#define MV_MAX_BODY_LEN (1024 * 1024)

typedef struct MailRelayMessage {
    char* message_id;          /**< Stable message identifier (UUID). */
    char* from;
    char* reply_to;
    char* to[MV_MAX_RECIPIENTS];
    int to_count;
    char* cc[MV_MAX_RECIPIENTS];
    int cc_count;
    char* bcc[MV_MAX_RECIPIENTS];
    int bcc_count;
    char* subject;
    char* text_body;
    char* html_body;
    char* template_key;        /**< Template used to render subject/body. */
    char* headers;             /**< Optional JSON string of extra headers. */
    char* idempotency_key;
    char* debounce_key;
    int priority;
    long long queue_id;            /**< Persisted mail_queue.queue_id (0 if none). */
} MailRelayMessage;

/* Initialize all fields to empty. */
void mailrelay_message_init(MailRelayMessage* m);

/* Free all owned strings and reset the message. */
void mailrelay_message_free(MailRelayMessage* m);

/* Deep-copy a message. Returns false if allocation fails. */
bool mailrelay_message_copy(MailRelayMessage* dst, const MailRelayMessage* src);

/* Dedicated mailrelay email-address validator (decoupled from auth). */
bool mailrelay_is_valid_email(const char* email);

/* Extract the domain portion (after '@') of an email address. Returns a
 * malloc'd string the caller must free, or NULL on failure. */
char* mailrelay_extract_domain(const char* email);

/* Check whether a sender domain matches a policy entry. Matches exact,
 * subdomain, and wildcard patterns (e.g. "example.com", "*.example.com"). */
bool mailrelay_domain_matches(const char* domain, const char* entry);

/* Reject header-injection: returns false if value contains CR, LF, or NUL. */
bool mailrelay_is_safe_header_value(const char* value);

/* Set the From address (copied). Returns false if the address is invalid. */
bool mailrelay_message_set_from(MailRelayMessage* m, const char* from);

/* Set the Reply-To address (copied). Returns false if invalid/empty. */
bool mailrelay_message_set_reply_to(MailRelayMessage* m, const char* reply_to);

/* Append a To/Cc/Bcc recipient (copied). Returns false on invalid or full. */
bool mailrelay_message_add_to(MailRelayMessage* m, const char* addr);
bool mailrelay_message_add_cc(MailRelayMessage* m, const char* addr);
bool mailrelay_message_add_bcc(MailRelayMessage* m, const char* addr);

/* Validate the message for sending. Writes a human-readable error into err. */
bool mailrelay_validate_message(const MailRelayMessage* m, char* err, size_t err_cap);

/* Count of all envelope recipients (to + cc + bcc). */
int mailrelay_message_recipient_count(const MailRelayMessage* m);

/* Validate the message's sender domain against a configured policy.
 * Returns true if the sender is permitted by the policy.
 * err, if provided, receives a human-readable error on rejection.
 */
bool mailrelay_validate_sender_domain(const MailRelayMessage* m,
                                       const MailRelaySecurity* security,
                                       char* err, size_t err_cap);

/* Append a recipient to the given envelope array. Exposed (non-static) so the
 * Unity test suite can exercise the shared recipient-append logic directly.
 * Not part of the stable public API. */
bool add_recipient(char* arr[], int* count, const char* addr);

/* Build a JSON array string containing all envelope recipients. Caller frees. */
char* mailrelay_message_recipients_to_json(const MailRelayMessage* m);

#endif /* MAILRELAY_MESSAGE_H */
