/*
 * Reporting Subsystem Landing
 *
 * Handles shutdown of the Reporting subsystem. Calls MagickWandTerminus()
 * to clean up ImageMagick resources. Idempotent — safe to call multiple times.
 */

// Global includes
#include <src/hydrogen.h>
#include <src/reporting/reporting_service.h>

// Local includes
#include "landing.h"

// Check reporting landing readiness
LaunchReadiness check_reporting_landing_readiness(void) {
    LaunchReadiness readiness = {0};

    if (!app_config || !app_config->reporting.Enabled) {
        readiness.ready = true;
        readiness.subsystem = SR_REPORTING;
        readiness.messages = NULL;
        return readiness;
    }

    readiness.ready = true;
    readiness.subsystem = SR_REPORTING;
    readiness.messages = NULL;

    log_this(SR_REPORTING, "Reporting subsystem is ready for landing", LOG_LEVEL_DEBUG, 0);
    return readiness;
}

// Land the reporting subsystem
int land_reporting_subsystem(void) {
    if (!app_config || !app_config->reporting.Enabled) {
        log_this(SR_REPORTING, "Reporting subsystem is disabled, skipping landing", LOG_LEVEL_DEBUG, 0);
        return 1;
    }

    log_this(SR_REPORTING, "Landing Reporting subsystem", LOG_LEVEL_DEBUG, 0);

    reporting_service_cleanup();

    log_this(SR_REPORTING, "Reporting subsystem landed successfully", LOG_LEVEL_DEBUG, 0);
    return 1;
}
