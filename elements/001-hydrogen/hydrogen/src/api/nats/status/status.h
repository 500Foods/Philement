/*
 * NATS status API.
 *
 * GET /api/nats/status. JWT matches the MCP status route.
 * Disabled NATS is still HTTP 200.
 */

#ifndef NATS_API_STATUS_H
#define NATS_API_STATUS_H

#include <jansson.h>
#include <microhttpd.h>

#include <src/nats/nats_stats.h>

/*
 * Handle GET /api/nats/status.
 *
 * Requires a valid JWT Bearer token. Returns the link, the design-lock
 * state, registry counts, and the three message counters. No peer ids.
 */
//@ swagger:path /api/nats/status
//@ swagger:method GET
//@ swagger:operationId natsStatus
//@ swagger:tags "NATS Service"
//@ swagger:summary Get NATS status and counters
//@ swagger:description Returns enabled, link, state, presence, singleton, peer counts, self state, and published, received, and reconnect counters. Requires a valid JWT Bearer token with a database claim. Does not include peer ids or payloads.
//@ swagger:security bearerAuth
//@ swagger:response 200 application/json {"type":"object","required":["success","enabled","link","state","presence","singleton","peers","alive","self","published","received","reconnects"],"properties":{"success":{"type":"boolean","example":true},"enabled":{"type":"boolean","example":false},"link":{"type":"string","enum":["down","degraded","up"],"example":"down"},"state":{"type":"string","enum":["down","degraded","up"],"example":"down"},"presence":{"type":"boolean","example":false},"singleton":{"type":"boolean","example":false},"peers":{"type":"integer","example":0},"alive":{"type":"integer","example":0},"self":{"type":"string","example":"none"},"published":{"type":"integer","example":0},"received":{"type":"integer","example":0},"reconnects":{"type":"integer","example":0}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Invalid or expired JWT token"}}}
//@ swagger:response 405 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Method not allowed"},"message":{"type":"string","example":"Only GET requests are supported"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Failed to build status response"}}}
enum MHD_Result handle_nats_status_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls);

json_t *nats_status_build_json(const NatsMetrics *metrics);

#endif /* NATS_API_STATUS_H */
