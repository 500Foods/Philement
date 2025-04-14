/*
 * System AppConfig API Endpoint Header
 * 
 * Declares the /api/system/appconfig endpoint that provides the current
 * application configuration in plain text format.
 */

#ifndef HYDROGEN_APPCONFIG_H
#define HYDROGEN_APPCONFIG_H

// Network headers
#include <microhttpd.h>

// Function declarations
enum MHD_Result handle_system_appconfig_request(struct MHD_Connection *connection);

#endif /* HYDROGEN_APPCONFIG_H */