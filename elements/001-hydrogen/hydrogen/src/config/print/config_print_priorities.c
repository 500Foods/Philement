/*
 * Print Queue Priorities Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "config_print_priorities.h"

int config_print_priorities_init(PrintQueuePrioritiesConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize with default priority values
    config->default_priority = DEFAULT_PRIORITY;
    config->emergency_priority = DEFAULT_EMERGENCY_PRIORITY;
    config->maintenance_priority = DEFAULT_MAINTENANCE_PRIORITY;
    config->system_priority = DEFAULT_SYSTEM_PRIORITY;

    return 0;
}

void config_print_priorities_cleanup(PrintQueuePrioritiesConfig* config) {
    if (!config) {
        return;
    }

    // Zero out the structure
    memset(config, 0, sizeof(PrintQueuePrioritiesConfig));
}

static int validate_priority(int priority) {
    return priority >= MIN_PRIORITY && priority <= MAX_PRIORITY;
}

static int validate_priority_spread(int priority1, int priority2) {
    return abs(priority1 - priority2) >= MIN_PRIORITY_SPREAD;
}

int config_print_priorities_validate(const PrintQueuePrioritiesConfig* config) {
    if (!config) {
        return -1;
    }

    // Validate individual priority values
    if (!validate_priority(config->default_priority) ||
        !validate_priority(config->emergency_priority) ||
        !validate_priority(config->maintenance_priority) ||
        !validate_priority(config->system_priority)) {
        return -1;
    }

    // Validate priority hierarchy
    // Emergency should be highest
    if (config->emergency_priority <= config->system_priority ||
        config->emergency_priority <= config->maintenance_priority ||
        config->emergency_priority <= config->default_priority) {
        return -1;
    }

    // System priority should be next highest
    if (config->system_priority <= config->maintenance_priority ||
        config->system_priority <= config->default_priority) {
        return -1;
    }

    // Maintenance should be higher than default
    if (config->maintenance_priority <= config->default_priority) {
        return -1;
    }

    // Validate priority spreads
    if (!validate_priority_spread(config->emergency_priority,
                                config->system_priority) ||
        !validate_priority_spread(config->system_priority,
                                config->maintenance_priority) ||
        !validate_priority_spread(config->maintenance_priority,
                                config->default_priority)) {
        return -1;
    }

    return 0;
}