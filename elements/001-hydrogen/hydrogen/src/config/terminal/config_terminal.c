/*
 * Terminal Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "config_terminal.h"

int config_terminal_init(TerminalConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize update intervals
    config->status_update_ms = DEFAULT_STATUS_UPDATE_MS;
    config->resource_check_ms = DEFAULT_RESOURCE_CHECK_MS;
    config->metrics_update_ms = DEFAULT_METRICS_UPDATE_MS;

    // Initialize warning thresholds
    config->memory_warning_percent = DEFAULT_MEMORY_WARNING_PERCENT;
    config->disk_warning_percent = DEFAULT_DISK_WARNING_PERCENT;
    config->load_warning = DEFAULT_LOAD_WARNING;

    return 0;
}

void config_terminal_cleanup(TerminalConfig* config) {
    if (!config) {
        return;
    }

    // Zero out the structure
    memset(config, 0, sizeof(TerminalConfig));
}

static int validate_interval(size_t interval) {
    return interval >= MIN_UPDATE_INTERVAL_MS &&
           interval <= MAX_UPDATE_INTERVAL_MS;
}

static int validate_percent(int percent) {
    return percent >= MIN_WARNING_PERCENT &&
           percent <= MAX_WARNING_PERCENT;
}

static int validate_load(double load) {
    return load >= MIN_LOAD_WARNING &&
           load <= MAX_LOAD_WARNING;
}

int config_terminal_validate(const TerminalConfig* config) {
    if (!config) {
        return -1;
    }

    // Validate update intervals
    if (!validate_interval(config->status_update_ms) ||
        !validate_interval(config->resource_check_ms) ||
        !validate_interval(config->metrics_update_ms)) {
        return -1;
    }

    // Validate warning thresholds
    if (!validate_percent(config->memory_warning_percent) ||
        !validate_percent(config->disk_warning_percent) ||
        !validate_load(config->load_warning)) {
        return -1;
    }

    // Validate timing relationships
    
    // Resource check interval should be longer than status update
    // to avoid overwhelming the system
    if (config->resource_check_ms < config->status_update_ms) {
        return -1;
    }

    // Metrics update should not be more frequent than status update
    if (config->metrics_update_ms < config->status_update_ms) {
        return -1;
    }

    // Resource check interval should be a multiple of status update
    // to maintain consistent timing
    if (config->resource_check_ms % config->status_update_ms != 0) {
        return -1;
    }

    // Metrics update should be a multiple of status update
    if (config->metrics_update_ms % config->status_update_ms != 0) {
        return -1;
    }

    return 0;
}