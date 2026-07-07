/*
 * Mail Relay result type returned by all send paths.
 * Uses fixed buffers so callers never allocate; mailrelay_result_free is a no-op.
 */

#ifndef MAILRELAY_RESULT_H
#define MAILRELAY_RESULT_H

#include <stdbool.h>

typedef struct MailRelayResult {
    bool success;
    int smtp_code;          /* last SMTP reply code (0 if transport failed) */
    char smtp_text[256];    /* last SMTP status text or transport error */
    double duration_ms;     /* total send duration in milliseconds */
    char error[256];        /* internal error detail (never secrets) */
    bool retryable;         /* true if the failure is worth retrying */
} MailRelayResult;

/* Reset a result to a clean default state. */
void mailrelay_result_init(MailRelayResult* r);

/* No-op for the fixed-buffer result; provided for API symmetry. */
void mailrelay_result_free(MailRelayResult* r);

#endif /* MAILRELAY_RESULT_H */
