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

// Function declarations
enum MHD_Result handle_system_recent_request(struct MHD_Connection *connection);

#endif /* HYDROGEN_RECENT_H */