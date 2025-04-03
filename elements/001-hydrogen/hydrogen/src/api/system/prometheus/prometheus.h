/*
 * System Prometheus API endpoint for the Hydrogen Project.
 * Provides system-level information in a Prometheus-compatible format.
 */

#ifndef HYDROGEN_SYSTEM_PROMETHEUS_H
#define HYDROGEN_SYSTEM_PROMETHEUS_H

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
#include "../../../config/config.h"
#include "../../../state/state.h"

/**
 * Handles the /api/system/prometheus endpoint request.
 * Initially returns system information in JSON format (same as /api/system/info)
 * Will be updated in future to return Prometheus-compatible format
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /api/system/prometheus
//@ swagger:method GET
//@ swagger:operationId getSystemPrometheus
//@ swagger:tags "System Service"
//@ swagger:summary System metrics endpoint (Prometheus)
//@ swagger:description Returns system metrics in a format compatible with Prometheus monitoring system
//@ swagger:response 200 application/json {"type":"object","properties":{"hardware":{"type":"object"},"os":{"type":"object"},"runtime":{"type":"object"},"version":{"type":"object"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"Failed to create response"}}}
enum MHD_Result handle_system_prometheus_request(struct MHD_Connection *connection);

#endif /* HYDROGEN_SYSTEM_PROMETHEUS_H */