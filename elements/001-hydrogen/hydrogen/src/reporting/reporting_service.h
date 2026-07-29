/*
 * Reporting Service Header
 *
 * REST API service for image processing and reporting operations.
 * Uses ImageMagick (MagickWand C API) for decode, scale, and encode.
 */

#ifndef REPORTING_SERVICE_H
#define REPORTING_SERVICE_H

//@ swagger:title Reporting Service API
//@ swagger:description REST API for image processing and reporting operations.
//@ swagger:version 1.0.0
//@ swagger:tag "Reporting Service" Provides image processing and conversion services.

#include <stdbool.h>

// Initialize reporting service (MagickWandGenesis + resource limits)
bool reporting_service_init(void);

// Cleanup reporting service (MagickWandTerminus)
void reporting_service_cleanup(void);

// Get service name for logging
const char* reporting_service_name(void);

// Whether MagickWand has been initialized for this process
bool reporting_service_is_initialized(void);

// Apply MagickWand resource caps (memory/map/area/width/height/time).
// Uses AppConfig.reporting when available; otherwise built-in defaults.
// Safe to call after MagickWandGenesis (including re-apply).
void reporting_service_apply_resource_limits(void);

#endif /* REPORTING_SERVICE_H */
