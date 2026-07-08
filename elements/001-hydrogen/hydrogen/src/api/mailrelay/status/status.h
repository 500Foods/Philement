/*
 * Mail Relay Status API Endpoint
 *
 * Implements GET /api/mailrelay/status returning subsystem counters and health.
 */

#ifndef MAILRELAY_API_STATUS_H
#define MAILRELAY_API_STATUS_H

#include <microhttpd.h>

/*
 * Handle GET /api/mailrelay/status requests.
 *
 * Requires a valid JWT Bearer token. Returns a JSON snapshot of Mail Relay
 * counters (queued, sending, sent, failed, retrying, permanent_failures,
 * queue_depth, worker_count, last_success, last_failure) plus enabled state.
 */
enum MHD_Result handle_mailrelay_status_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls);

#endif /* MAILRELAY_API_STATUS_H */
