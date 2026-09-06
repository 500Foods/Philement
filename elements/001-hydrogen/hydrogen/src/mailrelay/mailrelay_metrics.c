/*
 * Mail Relay Prometheus Metrics Implementation
 *
 * Generates Prometheus-compatible metrics from the Mail Relay status counters.
 * Follows the same pattern as chat_metrics_generate_prometheus().
 */

// Project includes
#include <src/hydrogen.h>
#include <string.h>
#include <stdarg.h>

// Local includes
#include "mailrelay_metrics.h"
#include <src/mailrelay/mailrelay.h>

size_t mailrelay_metrics_generate_prometheus(char* buffer, size_t buffer_size) {
    if (buffer_size == 0) {
        return 4096;
    }

    size_t offset = 0;
    MailRelayStatusCounters counters;
    memset(&counters, 0, sizeof(counters));

    bool have_status = mailrelay_get_status(&counters);

    if (!have_status && !counters.enabled) {
        int n = snprintf(buffer + offset, buffer_size - offset,
            "# HELP hydrogen_mailrelay_enabled Whether Mail Relay is enabled\n"
            "# TYPE hydrogen_mailrelay_enabled gauge\n"
            "hydrogen_mailrelay_enabled 0\n");
        if (n < 0) return offset;
        offset += (size_t)n;
        return offset < buffer_size ? offset : buffer_size;
    }

    offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
        "# HELP hydrogen_mailrelay_enabled Whether Mail Relay is enabled\n"
        "# TYPE hydrogen_mailrelay_enabled gauge\n"
        "hydrogen_mailrelay_enabled %.3f\n",
        counters.enabled ? 1.0 : 0.0);

    if (offset >= buffer_size) return buffer_size;

    offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
        "# HELP hydrogen_mailrelay_initialized Whether the Mail Relay runtime is initialized\n"
        "# TYPE hydrogen_mailrelay_initialized gauge\n"
        "hydrogen_mailrelay_initialized %.3f\n",
        counters.initialized ? 1.0 : 0.0);

    if (offset >= buffer_size) return buffer_size;

    offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
        "# HELP hydrogen_mailrelay_queued_total Total messages accepted into the queue\n"
        "# TYPE hydrogen_mailrelay_queued_total counter\n"
        "hydrogen_mailrelay_queued_total %.3f\n",
        (double)counters.queued);

    if (offset >= buffer_size) return buffer_size;

    offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
        "# HELP hydrogen_mailrelay_sending Messages currently being sent\n"
        "# TYPE hydrogen_mailrelay_sending gauge\n"
        "hydrogen_mailrelay_sending %.3f\n",
        (double)counters.sending);

    if (offset >= buffer_size) return buffer_size;

    offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
        "# HELP hydrogen_mailrelay_sent_total Total successfully delivered messages\n"
        "# TYPE hydrogen_mailrelay_sent_total counter\n"
        "hydrogen_mailrelay_sent_total %.3f\n",
        (double)counters.sent);

    if (offset >= buffer_size) return buffer_size;

    offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
        "# HELP hydrogen_mailrelay_failed_total Total failed deliveries\n"
        "# TYPE hydrogen_mailrelay_failed_total counter\n"
        "hydrogen_mailrelay_failed_total %.3f\n",
        (double)counters.failed);

    if (offset >= buffer_size) return buffer_size;

    offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
        "# HELP hydrogen_mailrelay_retrying Messages scheduled for retry\n"
        "# TYPE hydrogen_mailrelay_retrying gauge\n"
        "hydrogen_mailrelay_retrying %.3f\n",
        (double)counters.retrying);

    if (offset >= buffer_size) return buffer_size;

    offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
        "# HELP hydrogen_mailrelay_permanent_failures_total Messages that exhausted retries\n"
        "# TYPE hydrogen_mailrelay_permanent_failures_total counter\n"
        "hydrogen_mailrelay_permanent_failures_total %.3f\n",
        (double)counters.permanent_failures);

    if (offset >= buffer_size) return buffer_size;

    offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
        "# HELP hydrogen_mailrelay_last_success Unix timestamp of last successful delivery\n"
        "# TYPE hydrogen_mailrelay_last_success gauge\n"
        "hydrogen_mailrelay_last_success %.3f\n",
        counters.initialized ? (double)counters.last_success : 0.0);

    if (offset >= buffer_size) return buffer_size;

    offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
        "# HELP hydrogen_mailrelay_last_failure Unix timestamp of last failure\n"
        "# TYPE hydrogen_mailrelay_last_failure gauge\n"
        "hydrogen_mailrelay_last_failure %.3f\n",
        counters.initialized ? (double)counters.last_failure : 0.0);

    if (offset >= buffer_size) return buffer_size;

    offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
        "# HELP hydrogen_mailrelay_worker_count Active mail relay worker threads\n"
        "# TYPE hydrogen_mailrelay_worker_count gauge\n"
        "hydrogen_mailrelay_worker_count %.3f\n",
        counters.initialized ? (double)counters.worker_count : 0.0);

    if (offset >= buffer_size) return buffer_size;

    offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
        "# HELP hydrogen_mailrelay_queue_depth Current in-memory queue depth\n"
        "# TYPE hydrogen_mailrelay_queue_depth gauge\n"
        "hydrogen_mailrelay_queue_depth %.3f\n",
        counters.initialized ? (double)counters.queue_depth : 0.0);

    if (offset >= buffer_size) return buffer_size;

    return offset;
}
