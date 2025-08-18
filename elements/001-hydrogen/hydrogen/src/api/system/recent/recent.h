/*
 * System Recent API Endpoint Header
 * 
 * Declares the /api/system/recent endpoint that provides access
 * to the most recent log messages in reverse chronological order.
 */

#ifndef HYDROGEN_RECENT_H
#define HYDROGEN_RECENT_H

// Network headers
#include <microhttpd.h>

/**
 * Handles the /api/system/recent endpoint request.
 * Returns the most recent log messages in reverse chronological order.
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /api/system/recent
//@ swagger:method GET
//@ swagger:operationId getSystemRecent
//@ swagger:tags "System Service"
//@ swagger:summary Recent log messages endpoint
//@ swagger:description Returns the most recent log messages from the system in reverse chronological order
//@ swagger:response 200 application/json {"type":"object","properties":{"messages":{"type":"array","items":{"type":"object","properties":{"timestamp":{"type":"string"},"level":{"type":"string"},"component":{"type":"string"},"message":{"type":"string"}}}}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string"}}}

// Function declarations
enum MHD_Result handle_system_recent_request(struct MHD_Connection *connection);

#endif /* HYDROGEN_RECENT_H */
