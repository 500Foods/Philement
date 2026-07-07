/*
 * Mail Relay result implementation.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay_result.h>

void mailrelay_result_init(MailRelayResult* r) {
    if (!r) return;
    r->success = false;
    r->smtp_code = 0;
    r->smtp_text[0] = '\0';
    r->duration_ms = 0.0;
    r->error[0] = '\0';
    r->retryable = false;
}

void mailrelay_result_free(MailRelayResult* r) {
    (void)r;
}
