/*
 * System Configuration API endpoint for the Hydrogen Project.
 * Returns the server's configuration file in JSON format.
 */

#ifndef HYDROGEN_SYSTEM_CONFIG_H
#define HYDROGEN_SYSTEM_CONFIG_H

// Network headers
#include <microhttpd.h>

//@ swagger:path /api/system/config
//@ swagger:method GET
//@ swagger:operationId getSystemConfig
//@ swagger:tags "System Service"
//@ swagger:summary Server configuration endpoint
//@ swagger:description Returns the server's configuration file in JSON format, brotli compressed if the client supports it.
//@ swagger:response 200 application/json {"type":"object","description":"The server's configuration file"}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"Failed to read configuration"}}}
enum MHD_Result handle_system_config_request(struct MHD_Connection *connection,
                                           const char *method,
                                           const char *upload_data,
                                           size_t *upload_data_size,
                                           void **con_cls);

#endif /* HYDROGEN_SYSTEM_CONFIG_H */
