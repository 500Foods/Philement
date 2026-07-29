/*
 * Reporting Configuration
 *
 * Defines the configuration structure and handlers for the Reporting subsystem.
 * The Reporting subsystem provides image processing endpoints (image scaling,
 * format conversion) using ImageMagick via the MagickWand C API. It is
 * disabled by default; enabling requires explicit configuration.
 */

#ifndef HYDROGEN_CONFIG_REPORTING_H
#define HYDROGEN_CONFIG_REPORTING_H

#include <src/globals.h>

// System headers
#include <stddef.h>
#include <stdbool.h>

// Third-party headers
#include <jansson.h>

// Project headers
#include "config_forward.h"  // For AppConfig forward declaration

// Main reporting configuration structure
typedef struct ReportingConfig {
    bool Enabled;               // Whether the Reporting subsystem is enabled
    int MaxImageSize;           // Maximum output dimension in pixels (default 8192)
    int MaxInputBytes;          // Maximum base64 input size in bytes (default 50 MB)
    int MaxOutputBytes;         // Maximum base64 output size in bytes (default 50 MB)
    int DefaultDPI;             // Default DPI for pt-to-px conversion (default 72)
    char* AllowedFormats;       // Comma-separated list of allowed formats, or NULL for all
} ReportingConfig;

/*
 * Load reporting configuration from JSON
 *
 * Loads and processes reporting configuration from JSON, setting defaults and
 * handling environment variable overrides. The reporting subsystem is disabled
 * by default; enabling requires an explicit "Enabled": true in the JSON.
 *
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on error
 */
bool load_reporting_config(json_t* root, AppConfig* config);

/*
 * Dump reporting configuration for debugging
 *
 * Outputs the current reporting configuration state in a structured format.
 *
 * @param config The reporting configuration to dump
 */
void dump_reporting_config(const ReportingConfig* config);

/*
 * Clean up reporting configuration
 *
 * Frees all resources allocated for the reporting configuration. After cleanup,
 * the structure is zeroed to prevent use-after-free.
 *
 * @param config The reporting configuration to clean up
 */
void cleanup_reporting_config(ReportingConfig* config);

#endif /* HYDROGEN_CONFIG_REPORTING_H */
