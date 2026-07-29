/*
 * Reporting Configuration Implementation
 *
 * Implements the configuration handlers for the Reporting subsystem,
 * including JSON parsing and environment variable handling. The subsystem
 * is disabled by default; enabling requires explicit configuration.
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "config_reporting.h"
#include "config_utils.h"

// Load reporting configuration from JSON
bool load_reporting_config(json_t* root, AppConfig* config) {
    bool success = true;
    ReportingConfig* reporting = &config->reporting;

    // Zero out the config structure (sets Enabled=false by default)
    memset(reporting, 0, sizeof(ReportingConfig));

    // Set baseline defaults
    reporting->MaxImageSize = 8192;
    reporting->MaxInputBytes = 52428800;
    reporting->MaxOutputBytes = 52428800;
    reporting->DefaultDPI = 72;
    reporting->AllowedFormats = NULL;

    // Process main reporting section
    success = PROCESS_SECTION(root, "Reporting");
    success = success && PROCESS_BOOL(root, reporting, Enabled, "Reporting.Enabled", "Reporting");
    success = success && PROCESS_INT(root, reporting, MaxImageSize, "Reporting.MaxImageSize", "Reporting");
    success = success && PROCESS_INT(root, reporting, MaxInputBytes, "Reporting.MaxInputBytes", "Reporting");
    success = success && PROCESS_INT(root, reporting, MaxOutputBytes, "Reporting.MaxOutputBytes", "Reporting");
    success = success && PROCESS_INT(root, reporting, DefaultDPI, "Reporting.DefaultDPI", "Reporting");
    success = success && PROCESS_STRING(root, reporting, AllowedFormats, "Reporting.AllowedFormats", "Reporting");

    if (success) {
        log_this(SR_CONFIG, "― Reporting configuration loaded successfully", LOG_LEVEL_DEBUG, 0);
    }

    return success;
}

// Dump reporting configuration for debugging
void dump_reporting_config(const ReportingConfig* config) {
    if (!config) return;

    log_this(SR_CONFIG_CURRENT, "Reporting Configuration:", LOG_LEVEL_DEBUG, 0);
    log_this(SR_CONFIG_CURRENT, "  Enabled: %s", LOG_LEVEL_DEBUG, 1, config->Enabled ? "true" : "false");
    log_this(SR_CONFIG_CURRENT, "  MaxImageSize: %d", LOG_LEVEL_DEBUG, 1, config->MaxImageSize);
    log_this(SR_CONFIG_CURRENT, "  MaxInputBytes: %d", LOG_LEVEL_DEBUG, 1, config->MaxInputBytes);
    log_this(SR_CONFIG_CURRENT, "  MaxOutputBytes: %d", LOG_LEVEL_DEBUG, 1, config->MaxOutputBytes);
    log_this(SR_CONFIG_CURRENT, "  DefaultDPI: %d", LOG_LEVEL_DEBUG, 1, config->DefaultDPI);
    log_this(SR_CONFIG_CURRENT, "  AllowedFormats: %s", LOG_LEVEL_DEBUG, 1,
             config->AllowedFormats ? config->AllowedFormats : "(all)");
}

// Clean up reporting configuration
void cleanup_reporting_config(ReportingConfig* config) {
    if (!config) return;

    if (config->AllowedFormats) {
        free(config->AllowedFormats);
        config->AllowedFormats = NULL;
    }

    memset(config, 0, sizeof(ReportingConfig));
}
