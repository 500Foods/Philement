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
//@ swagger:path /api/mailrelay/status
//@ swagger:method GET
//@ swagger:operationId mailrelayStatus
//@ swagger:tags "Mail Service"
//@ swagger:summary Get Mail Relay status counters
//@ swagger:description Returns a snapshot of Mail Relay queue and delivery counters. Requires a valid JWT Bearer token with a database claim. Does not require the mail_send role. last_success and last_failure are Unix epoch seconds (0 when never set).
//@ swagger:security bearerAuth
//@ swagger:response 200 application/json {"type":"object","required":["success","enabled","initialized","queued","sending","sent","failed","retrying","permanent_failures","last_success","last_failure","worker_count","queue_depth"],"properties":{"success":{"type":"boolean","example":true},"enabled":{"type":"boolean","description":"Whether Mail Relay is enabled in config","example":true},"initialized":{"type":"boolean","description":"Whether the runtime has been initialized","example":true},"queued":{"type":"integer","description":"Messages accepted into the queue","example":12},"sending":{"type":"integer","description":"Messages currently being sent","example":1},"sent":{"type":"integer","description":"Successfully delivered messages","example":10},"failed":{"type":"integer","description":"Failed deliveries (including permanent)","example":1},"retrying":{"type":"integer","description":"Messages scheduled for retry","example":0},"permanent_failures":{"type":"integer","description":"Non-retryable failures","example":1},"last_success":{"type":"integer","description":"Unix epoch of last successful delivery (0 if none)","example":1720612800},"last_failure":{"type":"integer","description":"Unix epoch of last failure (0 if none)","example":0},"worker_count":{"type":"integer","description":"Configured worker threads","example":2},"queue_depth":{"type":"integer","description":"Current in-memory queue depth","example":0}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"JWT validation failure","example":"Invalid or expired JWT token"}}}
//@ swagger:response 405 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Method not allowed"},"message":{"type":"string","example":"Only GET requests are supported"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Failed to build status response"}}}
enum MHD_Result handle_mailrelay_status_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls);

#endif /* MAILRELAY_API_STATUS_H */
