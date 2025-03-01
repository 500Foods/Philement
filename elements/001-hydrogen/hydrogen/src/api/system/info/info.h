/*
 * System Info API endpoint for the Hydrogen Project.
 * Provides system-level information.
 */

#ifndef HYDROGEN_SYSTEM_INFO_H
#define HYDROGEN_SYSTEM_INFO_H

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <sys/utsname.h>

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

// Project headers
#include "../../../config/configuration.h"
#include "../../../state/state.h"

/**
 * Handles the /api/system/info endpoint request.
 * Returns system information in JSON format including:
 * - Hardware information
 * - Operating system details
 * - Runtime statistics
 * - Version information
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_system_info_request(struct MHD_Connection *connection);

#endif /* HYDROGEN_SYSTEM_INFO_H */