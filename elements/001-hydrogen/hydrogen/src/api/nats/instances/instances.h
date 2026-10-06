/*
 * NATS instances API.
 *
 * GET /api/nats/instances. Presence disabled still returns self
 * and an empty peer list. Peer ids are not logged.
 */

#ifndef NATS_API_INSTANCES_H
#define NATS_API_INSTANCES_H

#include <jansson.h>
#include <microhttpd.h>

/*
 * Handle GET /api/nats/instances.
 *
 * Requires a valid JWT Bearer token. Returns singleton, self, and
 * up to 64 peers. Each peer is id, state, and websocket_connections.
 */
//@ swagger:path /api/nats/instances
//@ swagger:method GET
//@ swagger:operationId natsInstances
//@ swagger:tags "NATS Service"
//@ swagger:summary Get NATS peer instances
//@ swagger:description Returns singleton, self state, and the peer snapshot. Presence disabled returns self and an empty peers array. Requires a valid JWT Bearer token with a database claim.
//@ swagger:security bearerAuth
//@ swagger:response 200 application/json {"type":"object","required":["success","singleton","self","peers"],"properties":{"success":{"type":"boolean","example":true},"singleton":{"type":"boolean","example":false},"self":{"type":"string","example":"none"},"peers":{"type":"array","items":{"type":"object","required":["id","state","websocket_connections"],"properties":{"id":{"type":"string"},"state":{"type":"string","enum":["Starting","Alive"]},"websocket_connections":{"type":"integer","example":0}}}}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Invalid or expired JWT token"}}}
//@ swagger:response 405 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Method not allowed"},"message":{"type":"string","example":"Only GET requests are supported"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"Failed to build instances response"}}}
enum MHD_Result handle_nats_instances_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls);

json_t *nats_instances_build_json(void);

#endif /* NATS_API_INSTANCES_H */
