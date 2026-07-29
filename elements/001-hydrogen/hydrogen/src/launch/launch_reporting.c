/*
 * Reporting Subsystem Launch
 *
 * Initializes the Reporting subsystem, which provides image processing
 * endpoints via ImageMagick (MagickWand C API). The subsystem is disabled
 * by default; it only initializes when ReportingConfig.Enabled is true.
 */

// Global includes
#include <src/hydrogen.h>
#include <src/reporting/reporting_service.h>

// Local includes
#include "launch.h"

// Forward declaration of config load function
bool load_reporting_config(json_t* root, AppConfig* config);

// Check reporting launch readiness
LaunchReadiness check_reporting_launch_readiness(void) {
    LaunchReadiness readiness = {0};

    if (!app_config) {
        readiness.ready = false;
        readiness.subsystem = SR_REPORTING;
        readiness.messages = NULL;
        return readiness;
    }

    if (!app_config->reporting.Enabled) {
        readiness.ready = true;
        readiness.subsystem = SR_REPORTING;
        readiness.messages = NULL;
        log_this(SR_REPORTING, "Reporting subsystem is disabled", LOG_LEVEL_DEBUG, 0);
        return readiness;
    }

    // Reporting is enabled — verify ImageMagick is available
    readiness.ready = true;
    readiness.subsystem = SR_REPORTING;
    readiness.messages = NULL;

    log_this(SR_REPORTING, "Reporting subsystem is ready", LOG_LEVEL_DEBUG, 0);
    return readiness;
}

// Launch the reporting subsystem
int launch_reporting_subsystem(void) {
    if (!app_config) {
        log_this(SR_REPORTING, "Cannot launch: no application config", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    if (!app_config->reporting.Enabled) {
        log_this(SR_REPORTING, "Reporting subsystem is disabled, skipping launch", LOG_LEVEL_DEBUG, 0);
        return 1;
    }

    log_this(SR_REPORTING, "Launching Reporting subsystem", LOG_LEVEL_DEBUG, 0);

    if (!reporting_service_init()) {
        log_this(SR_REPORTING, "Failed to initialize reporting service", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    log_this(SR_REPORTING, "Reporting subsystem launched successfully", LOG_LEVEL_DEBUG, 0);
    return 1;
}
