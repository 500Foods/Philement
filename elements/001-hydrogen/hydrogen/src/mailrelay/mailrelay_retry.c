/*
 * Mail Relay retry policy implementation.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay_retry.h>

#include <string.h>

MailRelayFailureClass mailrelay_retry_classify(const MailRelayResult* result) {
    if (!result) {
        return MAIL_FAILURE_UNKNOWN;
    }
    if (result->success) {
        return MAIL_FAILURE_UNKNOWN;
    }
    if (result->retryable) {
        return MAIL_FAILURE_TRANSIENT;
    }
    if (result->smtp_code >= 400 && result->smtp_code < 500) {
        return MAIL_FAILURE_TRANSIENT;
    }
    if (result->smtp_code >= 500 && result->smtp_code < 600) {
        return MAIL_FAILURE_PERMANENT;
    }
    return MAIL_FAILURE_PERMANENT;
}

bool mailrelay_retry_is_transient(const MailRelayResult* result) {
    return mailrelay_retry_classify(result) == MAIL_FAILURE_TRANSIENT;
}

int mailrelay_retry_compute_delay(int attempt, int base_seconds, int max_seconds) {
    if (base_seconds < 0) {
        base_seconds = 0;
    }
    if (max_seconds < 0) {
        max_seconds = 0;
    }
    if (attempt < 0) {
        attempt = 0;
    }
    if (base_seconds == 0) {
        return 0;
    }
    if (max_seconds > 0 && base_seconds > max_seconds) {
        base_seconds = max_seconds;
    }

    long delay = base_seconds;
    for (int i = 0; i < attempt; i++) {
        delay *= 2;
        if (max_seconds > 0 && delay >= max_seconds) {
            return max_seconds;
        }
    }
    if (max_seconds > 0 && delay > max_seconds) {
        delay = max_seconds;
    }
    return (int)delay;
}

bool mailrelay_retry_should_retry(const MailRelayResult* result,
                                  int attempts,
                                  int max_attempts) {
    if (!mailrelay_retry_is_transient(result)) {
        return false;
    }
    if (attempts < 0) {
        attempts = 0;
    }
    return attempts < max_attempts;
}
