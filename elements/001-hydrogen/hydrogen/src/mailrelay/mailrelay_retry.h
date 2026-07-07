/*
 * Mail Relay retry policy.
 *
 * Decides whether a failed send should be retried and computes the next
 * attempt time using exponential backoff. This module is policy-only; the
 * actual transport sets MailRelayResult.retryable based on the underlying
 * SMTP/curl error.
 */

#ifndef MAILRELAY_RETRY_H
#define MAILRELAY_RETRY_H

#include <stdbool.h>
#include <time.h>

#include <src/mailrelay/mailrelay_result.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Failure classification used for diagnostics and policy.
 */
typedef enum MailRelayFailureClass {
    MAIL_FAILURE_TRANSIENT = 0,
    MAIL_FAILURE_PERMANENT,
    MAIL_FAILURE_UNKNOWN
} MailRelayFailureClass;

/*
 * Classify a result. Prefers the explicit retryable flag when set; otherwise
 * falls back to the SMTP response code.
 */
MailRelayFailureClass mailrelay_retry_classify(const MailRelayResult* result);

/*
 * True if the failure represented by result is transient and eligible for
 * retry.
 */
bool mailrelay_retry_is_transient(const MailRelayResult* result);

/*
 * Compute exponential backoff delay for the next attempt.
 *
 * @param attempt       Number of attempts already made (0 = first retry).
 * @param base_seconds  InitialDelaySeconds from config.
 * @param max_seconds   MaxDelaySeconds from config.
 * @return Delay in seconds, capped at max_seconds. Base <= 0 yields 0.
 */
int mailrelay_retry_compute_delay(int attempt, int base_seconds, int max_seconds);

/*
 * True if the item should be retried given the current result and attempt
 * count.
 */
bool mailrelay_retry_should_retry(const MailRelayResult* result,
                                  int attempts,
                                  int max_attempts);

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_RETRY_H */
