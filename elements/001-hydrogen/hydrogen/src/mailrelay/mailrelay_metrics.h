/*
 * Mail Relay Prometheus Metrics
 *
 * Generates Prometheus-compatible metrics from the Mail Relay status counters.
 * Follows the same pattern as chat_metrics_generate_prometheus(): writes
 * HELP/TYPE header lines followed by metric data lines into a caller-provided
 * buffer, returning the number of bytes written (or needed if the buffer is
 * too small).
 */

#ifndef MAILRELAY_METRICS_H
#define MAILRELAY_METRICS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Generate Prometheus-format metrics for the Mail Relay subsystem.
 *
 * Calls mailrelay_get_status() internally to obtain the current counter
 * snapshot. When Mail Relay is not enabled or not initialized, only the
 * enabled gauge (0) is emitted.
 *
 * @param buffer Output buffer. May be NULL when buffer_size is 0 (capacity
 *               query mode: returns bytes that would be written).
 * @param buffer_size Size of buffer in bytes.
 * @return Number of bytes written (or that would be written if buffer is too
 *         small, excluding the trailing NUL).
 */
size_t mailrelay_metrics_generate_prometheus(char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_METRICS_H */
